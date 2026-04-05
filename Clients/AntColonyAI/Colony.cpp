#include "Colony.hpp"

//-----------------------------------------------------------------------------------------------
// Construction / Destruction
//-----------------------------------------------------------------------------------------------

Colony::Colony(const StartupInfo& info)
	: m_matchInfo(info.matchInfo)
	, m_playerInfo(info.yourPlayerInfo)
	, m_currentNutrients(info.matchInfo.startingNutrients)
	, m_currentTurn(0)
	, m_numFaults(0)
	, m_queenX(-1)
	, m_queenY(-1)
{
	m_gameMap.Initialize(info.matchInfo.mapWidth, info.matchInfo.agentTypeInfos);
	m_debugRenderer.Initialize(info.debugInterface, info.yourPlayerInfo.color);
}

Colony::~Colony()
{
	m_stateBuffer.SignalShutdown();
	m_ordersBuffer.SignalShutdown();
}

//-----------------------------------------------------------------------------------------------
// EXE Thread Methods (must return sub-1ms)
//-----------------------------------------------------------------------------------------------

void Colony::OnReceiveTurnState(const ArenaTurnStateForPlayer& state)
{
	// memcpy the entire state into the write buffer, then publish
	ArenaTurnStateForPlayer& writeBuffer = m_stateBuffer.GetWriteBuffer();
	memcpy(&writeBuffer, &state, sizeof(ArenaTurnStateForPlayer));
	m_stateBuffer.PublishWrite();
}

bool Colony::OnTurnOrderRequest(int turnNumber, PlayerTurnOrders* out)
{
	(void)turnNumber;

	if (!m_ordersBuffer.TrySwapRead())
		return false; // AI thread hasn't finished yet, server may ask again

	const PlayerTurnOrders& readBuffer = m_ordersBuffer.GetReadBuffer();
	memcpy(out, &readBuffer, sizeof(PlayerTurnOrders));
	return true;
}

//-----------------------------------------------------------------------------------------------
// Worker Thread
//-----------------------------------------------------------------------------------------------

void Colony::WorkerLoop(int threadIndex, std::atomic<bool>* isQuitting)
{
	// Only thread 0 runs the AI loop
	if (threadIndex != 0)
	{
		while (!isQuitting->load())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return;
	}

	// Log startup
	m_debugRenderer.LogText("Colony AI started (map: %dx%d, nutrients: %d)",
		m_matchInfo.mapWidth, m_matchInfo.mapWidth, m_matchInfo.startingNutrients);

	while (!isQuitting->load())
	{
		// Wait for new turn state from EXE thread
		m_stateBuffer.WaitForNewData();

		if (m_stateBuffer.IsShutdown() || isQuitting->load())
			break;

		if (!m_stateBuffer.TrySwapRead())
			continue;

		const ArenaTurnStateForPlayer& state = m_stateBuffer.GetReadBuffer();

		// AI pipeline
		UpdateGameState(state);
		ManageEconomy();
		AssignRoles();
		GenerateAntOrders();
		RenderDebug();
	}
}

//-----------------------------------------------------------------------------------------------
// AI Pipeline
//-----------------------------------------------------------------------------------------------

void Colony::UpdateGameState(const ArenaTurnStateForPlayer& state)
{
	m_currentTurn      = state.turnNumber;
	m_currentNutrients = state.currentNutrients;
	m_numFaults        = state.numFaults;

	// Update map from observations
	m_gameMap.UpdateFromObservation(state.observedTiles, state.tilesThatHaveFood, state.turnNumber);
	m_gameMap.UpdateExplorationHeat(state.turnNumber);

	// Update ant roster
	m_antManager.UpdateFromTurnState(state);

	// Track queen position
	Ant* queen = m_antManager.GetFirstLivingQueen();
	if (queen)
	{
		m_queenX = queen->m_x;
		m_queenY = queen->m_y;
	}
	else
	{
		m_queenX = -1;
		m_queenY = -1;
	}
}

//-----------------------------------------------------------------------------------------------
void Colony::ManageEconomy()
{
	Ant* queen = m_antManager.GetFirstLivingQueen();
	if (!queen)
		return;

	int totalUpkeep = m_antManager.GetTotalUpkeep(m_matchInfo.agentTypeInfos);
	int livingCount = m_antManager.GetLivingCount();
	bool isSuddenDeath = (m_currentTurn >= m_matchInfo.numTurnsBeforeSuddenDeath);

	// Track food income: if nutrients went up (after upkeep deduction), food was delivered
	int netChange = m_currentNutrients - m_prevNutrients;
	if (netChange > 0)
		m_turnsWithoutFood = 0;
	else
		++m_turnsWithoutFood;
	m_prevNutrients = m_currentNutrients;

	// Calculate how many turns of runway we have
	int turnsOfRunway = (totalUpkeep > 0) ? (m_currentNutrients / totalUpkeep) : 9999;

	// PROACTIVE DOWNSIZING: if we have less than 30 turns of runway and no food coming in
	if (turnsOfRunway < 30 && m_turnsWithoutFood > 10 && livingCount > 2)
	{
		AgentID suicideID = FindSuicideCandidate();
		if (suicideID != 0)
		{
			Ant* victim = m_antManager.FindAnt(suicideID);
			if (victim && victim->m_type != AGENT_TYPE_QUEEN)
				victim->AssignRole(AntRole::Idle);
		}
		return;
	}

	// STARVATION PREVENTION: suicide when nutrients are critically low
	if (m_currentNutrients <= totalUpkeep && livingCount > 1)
	{
		AgentID suicideID = FindSuicideCandidate();
		if (suicideID != 0)
		{
			Ant* victim = m_antManager.FindAnt(suicideID);
			if (victim && victim->m_type != AGENT_TYPE_QUEEN)
				victim->AssignRole(AntRole::Idle);
		}
		return;
	}

	// During sudden death, be very conservative
	if (isSuddenDeath)
	{
		int turnsIntoSuddenDeath = m_currentTurn - m_matchInfo.numTurnsBeforeSuddenDeath;
		int extraUpkeep = 0;
		if (m_matchInfo.suddenDeathTurnsPerUpkeepIncrease > 0)
			extraUpkeep = turnsIntoSuddenDeath / m_matchInfo.suddenDeathTurnsPerUpkeepIncrease;

		int effectiveUpkeep = totalUpkeep + extraUpkeep;

		// Proactively downsize during sudden death if runway is short
		if (m_currentNutrients < effectiveUpkeep * 100 && livingCount > 2)
		{
			AgentID suicideID = FindSuicideCandidate();
			if (suicideID != 0)
			{
				Ant* victim = m_antManager.FindAnt(suicideID);
				if (victim && victim->m_type != AGENT_TYPE_QUEEN)
					victim->AssignRole(AntRole::Idle);
			}
			return;
		}

		// Don't birth during sudden death unless we're flush
		if (m_currentNutrients < effectiveUpkeep * 200)
			return;
	}

	// BIRTH LOGIC: very conservative — need lots of runway
	if (livingCount >= m_matchInfo.colonyMaxPopulation)
		return;

	// Need at least 20x upkeep in reserve before birthing
	if (m_currentNutrients < totalUpkeep * 20)
		return;

	// Don't birth if we haven't seen food income recently
	if (m_turnsWithoutFood > 20 && livingCount > 3)
		return;

	eOrderCode birthOrder = GetBirthOrder();
	if (birthOrder == ORDER_HOLD)
		return;

	int birthTypeIndex = -1;
	switch (birthOrder)
	{
	case ORDER_BIRTH_SCOUT:   birthTypeIndex = AGENT_TYPE_SCOUT;   break;
	case ORDER_BIRTH_WORKER:  birthTypeIndex = AGENT_TYPE_WORKER;  break;
	case ORDER_BIRTH_SOLDIER: birthTypeIndex = AGENT_TYPE_SOLDIER; break;
	case ORDER_BIRTH_QUEEN:   birthTypeIndex = AGENT_TYPE_QUEEN;   break;
	default: return;
	}

	if (m_currentNutrients < m_matchInfo.agentTypeInfos[birthTypeIndex].costToBirth)
		return;

	queen->AssignRole(AntRole::Queen);
	queen->m_targetX = static_cast<short>(birthOrder);
	queen->m_targetY = 1;
}

//-----------------------------------------------------------------------------------------------
eOrderCode Colony::GetBirthOrder() const
{
	int numScouts  = m_antManager.GetCountByType(AGENT_TYPE_SCOUT);
	int numWorkers = m_antManager.GetCountByType(AGENT_TYPE_WORKER);

	// Lean composition: workers first (they gather food), then a few scouts
	// No soldiers for solo survival — they cost 3 upkeep and do nothing useful
	if (numWorkers < 4)
		return ORDER_BIRTH_WORKER;
	if (numScouts < 2)
		return ORDER_BIRTH_SCOUT;
	if (numWorkers < 6)
		return ORDER_BIRTH_WORKER;
	if (numScouts < 3)
		return ORDER_BIRTH_SCOUT;
	if (numWorkers < 8)
		return ORDER_BIRTH_WORKER;

	// Cap at 8 workers + 3 scouts = 11 ants + queen = 12 total
	// Upkeep: 8*2 + 3*1 + 10 = 29/turn — manageable
	return ORDER_HOLD;
}

//-----------------------------------------------------------------------------------------------
AgentID Colony::FindSuicideCandidate() const
{
	const Ant* pool = m_antManager.GetAntPool();
	AgentID bestID = 0;
	int bestPriority = -1;

	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (!pool[i].m_alive || pool[i].m_type == AGENT_TYPE_QUEEN)
			continue;

		int priority = m_matchInfo.agentTypeInfos[pool[i].m_type].sacrificePriority;
		if (priority > bestPriority)
		{
			bestPriority = priority;
			bestID = pool[i].m_id;
		}
	}

	return bestID;
}

//-----------------------------------------------------------------------------------------------
void Colony::AssignRoles()
{
	m_antManager.AssignRoles(m_gameMap, m_currentNutrients, m_matchInfo);
}

//-----------------------------------------------------------------------------------------------
void Colony::GenerateAntOrders()
{
	// Check if queen has a birth order queued from ManageEconomy
	Ant* queen = m_antManager.GetFirstLivingQueen();
	bool queenBirthing = false;
	eOrderCode queenBirthOrder = ORDER_HOLD;

	if (queen && queen->m_targetY == 1 && queen->m_targetX > 0)
	{
		queenBirthing = true;
		queenBirthOrder = static_cast<eOrderCode>(queen->m_targetX);
		queen->m_targetX = -1;
		queen->m_targetY = -1;
	}

	// Check if we need a suicide
	bool needSuicide = (m_currentNutrients <= 0 && m_antManager.GetLivingCount() > 1);
	AgentID suicideID = needSuicide ? FindSuicideCandidate() : 0;

	// Generate orders via AntManager
	PlayerTurnOrders& writeBuffer = m_ordersBuffer.GetWriteBuffer();
	m_antManager.GenerateAllOrders(m_gameMap, *this, &writeBuffer);

	// Override queen order with birth if needed
	if (queenBirthing && queen)
	{
		for (int i = 0; i < writeBuffer.numberOfOrders; ++i)
		{
			if (writeBuffer.orders[i].agentID == queen->m_id)
			{
				writeBuffer.orders[i].order = queenBirthOrder;
				break;
			}
		}
	}

	// Override suicide candidate order
	if (suicideID != 0)
	{
		for (int i = 0; i < writeBuffer.numberOfOrders; ++i)
		{
			if (writeBuffer.orders[i].agentID == suicideID)
			{
				writeBuffer.orders[i].order = ORDER_SUICIDE;
				break;
			}
		}
	}

	// Publish orders for EXE thread to read
	m_ordersBuffer.PublishWrite();
}

//-----------------------------------------------------------------------------------------------
void Colony::RenderDebug()
{
	// Update mood text every turn
	int totalUpkeep = m_antManager.GetTotalUpkeep(m_matchInfo.agentTypeInfos);
	int turnsOfRunway = (totalUpkeep > 0) ? (m_currentNutrients / totalUpkeep) : 9999;
	m_debugRenderer.SetMoodText("T%d N:%d U:%d R:%d Pop:%d NoFood:%d",
		m_currentTurn, m_currentNutrients, totalUpkeep, turnsOfRunway,
		m_antManager.GetLivingCount(), m_turnsWithoutFood);

	// Log periodic status (always, not gated by IsActive)
	if (m_currentTurn == 1 || m_currentTurn % 50 == 0)
	{
		m_debugRenderer.LogText("Turn %d: Nutrients=%d, Upkeep=%d, Pop=%d (S:%d W:%d So:%d Q:%d)",
			m_currentTurn, m_currentNutrients, totalUpkeep,
			m_antManager.GetLivingCount(),
			m_antManager.GetCountByType(AGENT_TYPE_SCOUT),
			m_antManager.GetCountByType(AGENT_TYPE_WORKER),
			m_antManager.GetCountByType(AGENT_TYPE_SOLDIER),
			m_antManager.GetCountByType(AGENT_TYPE_QUEEN));
	}

	// Only do expensive debug drawing when actively debugging
	if (!m_debugRenderer.IsActive())
		return;

	// Log on first turn to confirm LogText works
	if (m_currentTurn == 1)
		m_debugRenderer.LogText("=== Formicidae AI active on turn 1 ===");

	// Draw ant roles
	m_debugRenderer.DrawAntRoles(m_antManager.GetAntPool(), MAX_AGENTS_PER_PLAYER);

	// Draw test triangle (REQ-5 requirement)
	m_debugRenderer.DrawTestTriangle();
}
