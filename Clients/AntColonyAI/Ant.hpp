#pragma once

#include "Common.hpp"

//-----------------------------------------------------------------------------------------------
// Ant.hpp
//
// Per-ant state tracking and role-based order generation.
// Forward declares Colony and GameMap to avoid circular includes.
//-----------------------------------------------------------------------------------------------

class Colony;
class GameMap;

enum class AntRole : unsigned char
{
	Idle,       // No assignment yet
	Explorer,   // Scout/Worker exploring unseen tiles
	Gatherer,   // Worker heading to known food
	Deliverer,  // Worker carrying food back to queen
	Digger,     // Worker digging tunnels toward targets
	Guard,      // Soldier near queen
	Queen       // Queen: stay safe, birth ants
};

class Ant
{
public:
	// Identity (from AgentReport)
	AgentID             m_id    = 0;
	eAgentType          m_type  = AGENT_TYPE_SCOUT;
	bool                m_alive = false;

	// State (updated from AgentReport each turn)
	short               m_x          = 0;
	short               m_y          = 0;
	short               m_exhaustion = 0;
	eAgentState         m_state      = STATE_NORMAL;
	eAgentOrderResult   m_lastResult = AGENT_ORDER_SUCCESS_HELD;
	short               m_receivedCombatDamage      = 0;
	short               m_receivedSuffocationDamage  = 0;

	// AI state
	AntRole             m_role    = AntRole::Idle;
	short               m_targetX = -1;
	short               m_targetY = -1;

	// Methods
	void       UpdateFromReport(const AgentReport& report);
	AgentOrder DecideOrder(const GameMap& map, const Colony& colony) const;
	void       AssignRole(AntRole newRole, short tgtX = -1, short tgtY = -1);

	bool IsExhausted() const    { return m_exhaustion > 0; }
	bool IsCarryingFood() const { return m_state == STATE_HOLDING_FOOD; }
	bool IsCarryingDirt() const { return m_state == STATE_HOLDING_DIRT; }

private:
	// Role-specific order logic (called from DecideOrder)
	eOrderCode DecideQueenOrder(const GameMap& map, const Colony& colony) const;
	eOrderCode DecideExplorerOrder(const GameMap& map) const;
	eOrderCode DecideGathererOrder(const GameMap& map) const;
	eOrderCode DecideDelivererOrder(const GameMap& map, const Colony& colony) const;
	eOrderCode DecideDiggerOrder(const GameMap& map) const;
	eOrderCode DecideGuardOrder(const GameMap& map, const Colony& colony) const;
};
