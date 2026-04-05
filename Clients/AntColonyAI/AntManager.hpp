#pragma once

//-----------------------------------------------------------------------------------------------
// AntManager.hpp
//
// Ant object pool management, role assignment, and order generation.
//-----------------------------------------------------------------------------------------------

#include "Common.hpp"
#include "Ant.hpp"
#include "GameMap.hpp"

class Colony;

class AntManager
{
public:
	AntManager();

	// State sync from ArenaTurnStateForPlayer
	void UpdateFromTurnState(const ArenaTurnStateForPlayer& state);

	// Queries
	Ant*       FindAnt(AgentID id);
	const Ant* FindAnt(AgentID id) const;
	Ant*       GetFirstLivingQueen();
	int        GetLivingCount() const { return m_livingCount; }
	int        GetCountByType(eAgentType type) const;
	int        GetTotalUpkeep(const AgentTypeInfo agentTypeInfos[NUM_AGENT_TYPES]) const;

	// Iterators
	template<typename Func> void ForEachLivingAnt(Func&& fn);
	template<typename Func> void ForEachLivingAntOfType(eAgentType type, Func&& fn);

	// Order generation — fills PlayerTurnOrders
	void GenerateAllOrders(const GameMap& map, const Colony& colony, PlayerTurnOrders* out);

	// Role assignment (called by Colony)
	void AssignRoles(const GameMap& map, int currentNutrients, const MatchInfo& matchInfo);

	// Enemy tracking
	int                  GetEnemyCount() const { return m_numEnemies; }
	const ObservedAgent* GetEnemies() const { return m_enemies; }

	// Direct pool access (for iteration)
	const Ant* GetAntPool() const { return m_ants; }

private:
	Ant m_ants[MAX_AGENTS_PER_PLAYER];
	int m_livingCount = 0;

	// Enemy tracking (rebuilt each turn from observedAgents[])
	ObservedAgent m_enemies[MAX_AGENTS_TOTAL];
	int           m_numEnemies = 0;

	// Ant pool management
	int FindSlotByID(AgentID id) const;
	int FindFreeSlot() const;

	// Role assignment helpers
	void AssignScoutRoles(const GameMap& map);
	void AssignWorkerRoles(const GameMap& map, int nutrients, const MatchInfo& matchInfo);
	void AssignSoldierRoles(const GameMap& map);
	void AssignQueenRoles(int nutrients, const MatchInfo& matchInfo);
};

//-----------------------------------------------------------------------------------------------
// Template implementations (must be in header)
//-----------------------------------------------------------------------------------------------

template<typename Func>
void AntManager::ForEachLivingAnt(Func&& fn)
{
	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (m_ants[i].m_alive)
			fn(m_ants[i]);
	}
}

template<typename Func>
void AntManager::ForEachLivingAntOfType(eAgentType type, Func&& fn)
{
	for (int i = 0; i < MAX_AGENTS_PER_PLAYER; ++i)
	{
		if (m_ants[i].m_alive && m_ants[i].m_type == type)
			fn(m_ants[i]);
	}
}
