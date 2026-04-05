#include "Ant.hpp"
#include "GameMap.hpp"
#include "Colony.hpp"

//-----------------------------------------------------------------------------------------------
void Ant::UpdateFromReport(const AgentReport& report)
{
	m_id                        = report.agentID;
	m_type                      = report.type;
	m_x                         = report.tileX;
	m_y                         = report.tileY;
	m_exhaustion                = report.exhaustion;
	m_state                     = report.state;
	m_lastResult                = report.result;
	m_receivedCombatDamage      = report.receivedCombatDamage;
	m_receivedSuffocationDamage = report.receivedSuffocationDamage;

	if (m_state == STATE_DEAD)
		m_alive = false;
}

//-----------------------------------------------------------------------------------------------
void Ant::AssignRole(AntRole newRole, short tgtX, short tgtY)
{
	m_role    = newRole;
	m_targetX = tgtX;
	m_targetY = tgtY;
}

//-----------------------------------------------------------------------------------------------
// Helper: get the dig order for an adjacent tile
//-----------------------------------------------------------------------------------------------
static eOrderCode GetDigOrderForDirection(short fromX, short fromY, short toX, short toY)
{
	int dx = toX - fromX;
	int dy = toY - fromY;

	if (dx == 1 && dy == 0)  return ORDER_DIG_EAST;
	if (dx == -1 && dy == 0) return ORDER_DIG_WEST;
	if (dx == 0 && dy == 1)  return ORDER_DIG_NORTH;
	if (dx == 0 && dy == -1) return ORDER_DIG_SOUTH;
	if (dx == 0 && dy == 0)  return ORDER_DIG_HERE;

	return ORDER_HOLD;
}

//-----------------------------------------------------------------------------------------------
AgentOrder Ant::DecideOrder(const GameMap& map, const Colony& colony) const
{
	AgentOrder order;
	order.agentID = m_id;
	order.order   = ORDER_HOLD;

	if (IsExhausted())
		return order;

	switch (m_role)
	{
	case AntRole::Queen:     order.order = DecideQueenOrder(map, colony);     break;
	case AntRole::Explorer:  order.order = DecideExplorerOrder(map);          break;
	case AntRole::Gatherer:  order.order = DecideGathererOrder(map);          break;
	case AntRole::Deliverer: order.order = DecideDelivererOrder(map, colony); break;
	case AntRole::Digger:    order.order = DecideDiggerOrder(map);            break;
	case AntRole::Guard:     order.order = DecideGuardOrder(map, colony);     break;
	case AntRole::Idle:
	default:                 order.order = DecideExplorerOrder(map);          break;
	}

	return order;
}

//-----------------------------------------------------------------------------------------------
// Queen: hold position; birth decisions are handled by Colony economy logic
//-----------------------------------------------------------------------------------------------
eOrderCode Ant::DecideQueenOrder(const GameMap& map, const Colony& colony) const
{
	(void)map;
	(void)colony;
	return ORDER_HOLD;
}

//-----------------------------------------------------------------------------------------------
// Explorer: move toward nearest unexplored frontier tile
//-----------------------------------------------------------------------------------------------
eOrderCode Ant::DecideExplorerOrder(const GameMap& map) const
{
	// If carrying dirt from a previous dig, drop it
	if (IsCarryingDirt())
		return ORDER_DROP_CARRIED_OBJECT;

	// If we have a valid target, move toward it
	if (m_targetX >= 0 && m_targetY >= 0)
	{
		eOrderCode move = map.GetNextMoveToward(m_x, m_y, m_targetX, m_targetY, m_type);
		if (move != ORDER_HOLD)
			return move;
	}

	// Find nearest unexplored area
	short ux = -1, uy = -1;
	if (map.FindNearestUnexplored(m_x, m_y, ux, uy, m_type))
	{
		eOrderCode move = map.GetNextMoveToward(m_x, m_y, ux, uy, m_type);
		if (move != ORDER_HOLD)
			return move;
	}

	return ORDER_HOLD;
}

//-----------------------------------------------------------------------------------------------
// Gatherer: move to known food, pick it up
//-----------------------------------------------------------------------------------------------
eOrderCode Ant::DecideGathererOrder(const GameMap& map) const
{
	// If already carrying food, hold (AntManager should reassign to Deliverer)
	if (IsCarryingFood())
		return ORDER_HOLD;

	// If carrying dirt, drop it first
	if (IsCarryingDirt())
		return ORDER_DROP_CARRIED_OBJECT;

	// If standing on food, pick it up
	if (map.HasFood(m_x, m_y))
		return ORDER_PICK_UP_FOOD;

	// If we have a food target, move toward it
	if (m_targetX >= 0 && m_targetY >= 0)
	{
		if (map.HasFood(m_targetX, m_targetY))
		{
			eOrderCode move = map.GetNextMoveToward(m_x, m_y, m_targetX, m_targetY, m_type);
			if (move != ORDER_HOLD)
				return move;
		}
	}

	// Search for nearest food
	short fx = -1, fy = -1;
	if (map.FindNearestFood(m_x, m_y, fx, fy, m_type))
	{
		eOrderCode move = map.GetNextMoveToward(m_x, m_y, fx, fy, m_type);
		if (move != ORDER_HOLD)
			return move;
	}

	// No food found — explore instead
	short ux = -1, uy = -1;
	if (map.FindNearestUnexplored(m_x, m_y, ux, uy, m_type))
	{
		eOrderCode move = map.GetNextMoveToward(m_x, m_y, ux, uy, m_type);
		if (move != ORDER_HOLD)
			return move;
	}

	return ORDER_HOLD;
}

//-----------------------------------------------------------------------------------------------
// Deliverer: carry food back to queen, drop it
//-----------------------------------------------------------------------------------------------
eOrderCode Ant::DecideDelivererOrder(const GameMap& map, const Colony& colony) const
{
	if (!IsCarryingFood())
		return ORDER_HOLD;

	short qx = colony.GetQueenX();
	short qy = colony.GetQueenY();

	if (qx < 0 || qy < 0)
		return ORDER_HOLD;

	// On same tile as queen — drop food
	if (m_x == qx && m_y == qy)
		return ORDER_DROP_CARRIED_OBJECT;

	// Move toward queen
	eOrderCode move = map.GetNextMoveToward(m_x, m_y, qx, qy, m_type);
	if (move != ORDER_HOLD)
		return move;

	return ORDER_HOLD;
}

//-----------------------------------------------------------------------------------------------
// Digger: move to dig target, dig adjacent dirt
//-----------------------------------------------------------------------------------------------
eOrderCode Ant::DecideDiggerOrder(const GameMap& map) const
{
	if (IsCarryingDirt())
		return ORDER_DROP_CARRIED_OBJECT;

	if (m_targetX >= 0 && m_targetY >= 0)
	{
		int dx = abs(m_targetX - m_x);
		int dy = abs(m_targetY - m_y);

		if ((dx + dy) == 1 && map.CanDig(m_targetX, m_targetY, m_type))
			return GetDigOrderForDirection(m_x, m_y, m_targetX, m_targetY);

		if (dx == 0 && dy == 0 && map.CanDig(m_x, m_y, m_type))
			return ORDER_DIG_HERE;

		eOrderCode move = map.GetNextMoveToward(m_x, m_y, m_targetX, m_targetY, m_type);
		if (move != ORDER_HOLD)
			return move;
	}

	short digX = -1, digY = -1;
	if (map.FindNearestDigTarget(m_x, m_y, digX, digY, m_type))
	{
		int dx = abs(digX - m_x);
		int dy = abs(digY - m_y);

		if ((dx + dy) == 1)
			return GetDigOrderForDirection(m_x, m_y, digX, digY);

		eOrderCode move = map.GetNextMoveToward(m_x, m_y, digX, digY, m_type);
		if (move != ORDER_HOLD)
			return move;
	}

	return ORDER_HOLD;
}

//-----------------------------------------------------------------------------------------------
// Guard: stay near queen
//-----------------------------------------------------------------------------------------------
eOrderCode Ant::DecideGuardOrder(const GameMap& map, const Colony& colony) const
{
	short qx = colony.GetQueenX();
	short qy = colony.GetQueenY();

	if (qx < 0 || qy < 0)
		return ORDER_HOLD;

	int dist = abs(m_x - qx) + abs(m_y - qy);
	if (dist <= 1)
		return ORDER_HOLD;

	eOrderCode move = map.GetNextMoveToward(m_x, m_y, qx, qy, m_type);
	if (move != ORDER_HOLD)
		return move;

	return ORDER_HOLD;
}
