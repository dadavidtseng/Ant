#pragma once

//-----------------------------------------------------------------------------------------------
// Colony.hpp
//
// Central AI orchestrator. Owns GameMap, AntManager, DebugRenderer.
// Manages thread-safe data exchange via TripleBuffer, economy decisions,
// and the per-turn AI pipeline.
//-----------------------------------------------------------------------------------------------

#include "Common.hpp"
#include "TripleBuffer.hpp"
#include "GameMap.hpp"
#include "AntManager.hpp"
#include "DebugRenderer.hpp"

class Colony
{
public:
	Colony(StartupInfo const& info);
	~Colony();

	// Called by ArenaPlayerImplementation (EXE thread, must return sub-1ms)
	void OnReceiveTurnState(ArenaTurnStateForPlayer const& state);
	bool OnTurnOrderRequest(int turnNumber, PlayerTurnOrders* out);

	// Called by ArenaPlayerImplementation (worker thread)
	void WorkerLoop(int threadIndex, std::atomic<bool>* isQuitting);

	// Accessors needed by Ant::DecideOrder and AntManager
	short GetQueenX() const { return m_queenX; }
	short GetQueenY() const { return m_queenY; }
	int   GetCurrentNutrients() const { return m_currentNutrients; }
	int   GetCurrentTurn() const { return m_currentTurn; }
	MatchInfo const&     GetMatchInfo() const { return m_matchInfo; }
	AgentTypeInfo const& GetAgentTypeInfo(eAgentType type) const { return m_matchInfo.agentTypeInfos[type]; }

private:
	// Subsystems (owned by value)
	GameMap       m_gameMap;
	AntManager    m_antManager;
	DebugRenderer m_debugRenderer;

	// Match data (from StartupInfo)
	MatchInfo  m_matchInfo;
	PlayerInfo m_playerInfo;

	// Per-turn state
	short m_queenX          = -1;
	short m_queenY          = -1;
	int   m_currentNutrients = 0;
	int   m_currentTurn      = 0;
	int   m_numFaults        = 0;
	int   m_prevNutrients    = 0;   // Nutrients last turn (for income tracking)
	int   m_foodDelivered    = 0;   // Cumulative food delivered
	int   m_turnsWithoutFood = 0;   // Consecutive turns with no food income

	// Thread communication
	TripleBuffer<ArenaTurnStateForPlayer> m_stateBuffer;
	TripleBuffer<PlayerTurnOrders>        m_ordersBuffer;

	// AI pipeline (called from WorkerLoop on thread 0)
	void UpdateGameState(ArenaTurnStateForPlayer const& state);
	void ManageEconomy();
	void AssignRoles();
	void GenerateAntOrders();
	void RenderDebug();

	// Economy helpers
	eOrderCode GetBirthOrder() const;
	AgentID    FindSuicideCandidate() const;
};
