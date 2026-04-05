#include "AntManager.hpp"
#include "Colony.hpp"

//-----------------------------------------------------------------------------------------------
AntManager::AntManager()
	: m_livingCount(0)
	, m_numEnemies(0)
{
	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
		m_ants[i].m_alive = false;
}

//-----------------------------------------------------------------------------------------------
// State Sync
//-----------------------------------------------------------------------------------------------

void AntManager::UpdateFromTurnState(ArenaTurnStateForPlayer const& state)
{
	// Process agent reports
	for (int i = 0; i < state.numReports; ++i)
	{
		AgentReport const& report = state.agentReports[i];

		if (report.result == AGENT_WAS_CREATED)
		{
			// New ant — find a free slot
			int slot = FindFreeSlot();
			if (slot >= 0)
			{
				m_ants[slot].m_alive = true;
				m_ants[slot].UpdateFromReport(report);
			}
			continue;
		}

		// Find existing ant
		int slot = FindSlotByID(report.agentID);
		if (slot < 0)
			continue;

		// Check for death events
		bool killed = (report.result == AGENT_KILLED_BY_ENEMY)
			|| (report.result == AGENT_KILLED_BY_WATER)
			|| (report.result == AGENT_KILLED_BY_SUFFOCATION)
			|| (report.result == AGENT_KILLED_BY_STARVATION)
			|| (report.result == AGENT_KILLED_BY_PENALTY)
			|| (report.result == AGENT_ORDER_SUCCESS_SUICIDE);

		if (killed)
		{
			m_ants[slot].m_alive = false;
			m_ants[slot].m_state = STATE_DEAD;
			continue;
		}

		// Update living ant
		m_ants[slot].UpdateFromReport(report);

		// Auto-fix role if state changed
		if (m_ants[slot].IsCarryingFood() && m_ants[slot].m_role != AntRole::Deliverer)
			m_ants[slot].AssignRole(AntRole::Deliverer);
		else if (!m_ants[slot].IsCarryingFood() && m_ants[slot].m_role == AntRole::Deliverer)
			m_ants[slot].AssignRole(AntRole::Gatherer);
	}

	// Recount living ants
	m_livingCount = 0;
	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (m_ants[i].m_alive)
			++m_livingCount;
	}

	// Rebuild enemy list from observed agents
	m_numEnemies = 0;
	for (int i = 0; i < state.numObservedAgents; ++i)
	{
		if (m_numEnemies < MAX_AGENTS_TOTAL)
			m_enemies[m_numEnemies++] = state.observedAgents[i];
	}
}

//-----------------------------------------------------------------------------------------------
// Queries
//-----------------------------------------------------------------------------------------------

Ant* AntManager::FindAnt(AgentID id)
{
	int slot = FindSlotByID(id);
	return (slot >= 0) ? &m_ants[slot] : nullptr;
}

Ant const* AntManager::FindAnt(AgentID id) const
{
	int slot = FindSlotByID(id);
	return (slot >= 0) ? &m_ants[slot] : nullptr;
}

Ant* AntManager::GetFirstLivingQueen()
{
	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (m_ants[i].m_alive && m_ants[i].m_type == AGENT_TYPE_QUEEN)
			return &m_ants[i];
	}
	return nullptr;
}

int AntManager::GetCountByType(eAgentType type) const
{
	int count = 0;
	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (m_ants[i].m_alive && m_ants[i].m_type == type)
			++count;
	}
	return count;
}

int AntManager::GetTotalUpkeep(AgentTypeInfo const agentTypeInfos[NUM_AGENT_TYPES]) const
{
	int total = 0;
	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (m_ants[i].m_alive)
			total += agentTypeInfos[m_ants[i].m_type].upkeepPerTurn;
	}
	return total;
}

//-----------------------------------------------------------------------------------------------
// Pool Management
//-----------------------------------------------------------------------------------------------

int AntManager::FindSlotByID(AgentID id) const
{
	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (m_ants[i].m_alive && m_ants[i].m_id == id)
			return i;
	}
	return -1;
}

int AntManager::FindFreeSlot() const
{
	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (!m_ants[i].m_alive)
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------------------------
// Order Generation
//-----------------------------------------------------------------------------------------------

void AntManager::GenerateAllOrders(GameMap const& map, Colony const& colony, PlayerTurnOrders* out)
{
	out->numberOfOrders = 0;

	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (!m_ants[i].m_alive)
			continue;

		if (out->numberOfOrders >= MAX_ORDERS_PER_PLAYER)
			break;

		AgentOrder order = m_ants[i].DecideOrder(map, colony);
		out->orders[out->numberOfOrders++] = order;
	}
}

//-----------------------------------------------------------------------------------------------
// Role Assignment
//-----------------------------------------------------------------------------------------------

void AntManager::AssignRoles(GameMap const& map, int currentNutrients, MatchInfo const& matchInfo)
{
	AssignQueenRoles(currentNutrients, matchInfo);
	AssignScoutRoles(map);
	AssignWorkerRoles(map, currentNutrients, matchInfo);
	AssignSoldierRoles(map);
}

//-----------------------------------------------------------------------------------------------
void AntManager::AssignQueenRoles(int nutrients, MatchInfo const& matchInfo)
{
	(void)nutrients;
	(void)matchInfo;

	ForEachLivingAntOfType(AGENT_TYPE_QUEEN, [](Ant& ant)
	{
		if (ant.m_role != AntRole::Queen)
			ant.AssignRole(AntRole::Queen);
	});
}

//-----------------------------------------------------------------------------------------------
void AntManager::AssignScoutRoles(GameMap const& map)
{
	ForEachLivingAntOfType(AGENT_TYPE_SCOUT, [&map](Ant& ant)
	{
		// Scouts always explore
		if (ant.m_role == AntRole::Idle || ant.m_role == AntRole::Explorer)
		{
			short ux = -1, uy = -1;
			if (map.FindNearestUnexplored(ant.m_x, ant.m_y, ux, uy, ant.m_type))
				ant.AssignRole(AntRole::Explorer, ux, uy);
			else
				ant.AssignRole(AntRole::Explorer);
		}
	});
}

//-----------------------------------------------------------------------------------------------
void AntManager::AssignWorkerRoles(GameMap const& map, int nutrients, MatchInfo const& matchInfo)
{
	(void)nutrients;
	(void)matchInfo;

	ForEachLivingAntOfType(AGENT_TYPE_WORKER, [&map](Ant& ant)
	{
		// Workers carrying food should deliver
		if (ant.IsCarryingFood())
		{
			if (ant.m_role != AntRole::Deliverer)
				ant.AssignRole(AntRole::Deliverer);
			return;
		}

		// Workers carrying dirt should drop it (handled in DecideOrder)
		if (ant.IsCarryingDirt())
			return;

		// Idle workers become gatherers
		if (ant.m_role == AntRole::Idle || ant.m_role == AntRole::Deliverer)
		{
			short fx = -1, fy = -1;
			if (map.FindNearestFood(ant.m_x, ant.m_y, fx, fy, ant.m_type))
				ant.AssignRole(AntRole::Gatherer, fx, fy);
			else
				ant.AssignRole(AntRole::Explorer);
		}

		// Gatherers: refresh target if food is gone
		if (ant.m_role == AntRole::Gatherer)
		{
			if (ant.m_targetX >= 0 && ant.m_targetY >= 0 && !map.HasFood(ant.m_targetX, ant.m_targetY))
			{
				short fx = -1, fy = -1;
				if (map.FindNearestFood(ant.m_x, ant.m_y, fx, fy, ant.m_type))
					ant.AssignRole(AntRole::Gatherer, fx, fy);
				else
					ant.AssignRole(AntRole::Explorer);
			}
		}

		// Explorers: refresh target
		if (ant.m_role == AntRole::Explorer)
		{
			short ux = -1, uy = -1;
			if (map.FindNearestUnexplored(ant.m_x, ant.m_y, ux, uy, ant.m_type))
				ant.AssignRole(AntRole::Explorer, ux, uy);
		}
	});
}

//-----------------------------------------------------------------------------------------------
void AntManager::AssignSoldierRoles(GameMap const& map)
{
	(void)map;

	ForEachLivingAntOfType(AGENT_TYPE_SOLDIER, [](Ant& ant)
	{
		// Soldiers guard the queen
		if (ant.m_role != AntRole::Guard)
			ant.AssignRole(AntRole::Guard);
	});
}
