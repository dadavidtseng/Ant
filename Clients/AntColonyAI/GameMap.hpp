#pragma once

//-----------------------------------------------------------------------------------------------
// GameMap.hpp
//
// Tile grid, fog-of-war management, food tracking, and BFS pathfinding.
// All movement validation reads from AgentTypeInfo at runtime — never hardcoded.
//-----------------------------------------------------------------------------------------------

#include "Common.hpp"
#include "HeatMap.hpp"

class GameMap
{
public:
	GameMap();
	void Initialize(short mapWidth, AgentTypeInfo const agentTypeInfos[NUM_AGENT_TYPES]);

	// State update from ArenaTurnStateForPlayer
	void UpdateFromObservation(
		eTileType const observedTiles[MAX_ARENA_TILES],
		bool const tilesThatHaveFood[MAX_ARENA_TILES],
		int turnNumber
	);

	// Tile queries
	eTileType GetTile(short x, short y) const;
	bool      HasFood(short x, short y) const;
	int       GetLastSeenTurn(short x, short y) const;
	bool      IsExplored(short x, short y) const;

	// Movement queries (uses AgentTypeInfo.moveExhaustPenalties)
	bool IsPassable(short x, short y, eAgentType type) const;
	int  GetMoveExhaust(short x, short y, eAgentType type) const;
	bool CanDig(short x, short y, eAgentType type) const;

	// Pathfinding (BFS on tile grid)
	eOrderCode GetNextMoveToward(short fromX, short fromY, short toX, short toY, eAgentType type) const;

	// Search queries
	bool FindNearestFood(short fromX, short fromY, short& outX, short& outY, eAgentType type) const;
	bool FindNearestUnexplored(short fromX, short fromY, short& outX, short& outY, eAgentType type) const;
	bool FindNearestDigTarget(short fromX, short fromY, short& outX, short& outY, eAgentType type) const;

	// Exploration scoring
	HeatMap&       GetExplorationHeatMap() { return m_explorationHeat; }
	HeatMap const& GetExplorationHeatMap() const { return m_explorationHeat; }
	void           UpdateExplorationHeat(int turnNumber);

	short GetWidth() const { return m_width; }

private:
	struct TileInfo
	{
		eTileType m_type         = TILE_TYPE_UNSEEN;
		bool      m_hasFood      = false;
		int       m_lastSeenTurn = -1;
	};

	short         m_width = 0;
	TileInfo      m_tiles[MAX_ARENA_TILES];
	HeatMap       m_explorationHeat;
	AgentTypeInfo m_agentTypeInfos[NUM_AGENT_TYPES];

	int  TileIndex(short x, short y) const { return y * m_width + x; }
	bool InBounds(short x, short y) const { return x >= 0 && x < m_width && y >= 0 && y < m_width; }

	// Direction helpers
	static constexpr int DX[4] = { 1, 0, -1, 0 };  // E, N, W, S
	static constexpr int DY[4] = { 0, 1, 0, -1 };
	static constexpr eOrderCode MOVE_ORDERS[4] = { ORDER_MOVE_EAST, ORDER_MOVE_NORTH, ORDER_MOVE_WEST, ORDER_MOVE_SOUTH };

	// BFS helper
	struct BFSNode
	{
		short      m_x, m_y;
		eOrderCode m_firstMove;
	};
};
