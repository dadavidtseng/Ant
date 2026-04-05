#include "GameMap.hpp"

//-----------------------------------------------------------------------------------------------
// Construction / Initialization
//-----------------------------------------------------------------------------------------------

GameMap::GameMap()
	: m_width(0)
{
	memset(m_agentTypeInfos, 0, sizeof(m_agentTypeInfos));
}

//-----------------------------------------------------------------------------------------------
void GameMap::Initialize(short mapWidth, AgentTypeInfo const agentTypeInfos[NUM_AGENT_TYPES])
{
	m_width = mapWidth;
	memcpy(m_agentTypeInfos, agentTypeInfos, sizeof(AgentTypeInfo) * NUM_AGENT_TYPES);

	// Reset all tiles to unseen
	for (int i = 0; i < MAX_ARENA_TILES; ++i)
	{
		m_tiles[i].m_type         = TILE_TYPE_UNSEEN;
		m_tiles[i].m_hasFood      = false;
		m_tiles[i].m_lastSeenTurn = -1;
	}

	m_explorationHeat.Initialize(mapWidth);
}

//-----------------------------------------------------------------------------------------------
// State Update
//-----------------------------------------------------------------------------------------------

void GameMap::UpdateFromObservation(
	eTileType const observedTiles[MAX_ARENA_TILES],
	bool const tilesThatHaveFood[MAX_ARENA_TILES],
	int turnNumber)
{
	int totalTiles = m_width * m_width;
	for (int i = 0; i < totalTiles; ++i)
	{
		if (observedTiles[i] != TILE_TYPE_UNSEEN)
		{
			m_tiles[i].m_type         = observedTiles[i];
			m_tiles[i].m_hasFood      = tilesThatHaveFood[i];
			m_tiles[i].m_lastSeenTurn = turnNumber;
		}
		// Fog-of-war: retain last known state for unseen tiles
		// but clear food flag since we can't confirm it's still there
		else if (m_tiles[i].m_lastSeenTurn >= 0)
		{
			m_tiles[i].m_hasFood = false;
		}
	}
}

//-----------------------------------------------------------------------------------------------
// Tile Queries
//-----------------------------------------------------------------------------------------------

eTileType GameMap::GetTile(short x, short y) const
{
	if (!InBounds(x, y))
		return TILE_TYPE_STONE;
	return m_tiles[TileIndex(x, y)].m_type;
}

bool GameMap::HasFood(short x, short y) const
{
	if (!InBounds(x, y))
		return false;
	return m_tiles[TileIndex(x, y)].m_hasFood;
}

int GameMap::GetLastSeenTurn(short x, short y) const
{
	if (!InBounds(x, y))
		return -1;
	return m_tiles[TileIndex(x, y)].m_lastSeenTurn;
}

bool GameMap::IsExplored(short x, short y) const
{
	if (!InBounds(x, y))
		return true; // Out of bounds is "known" (impassable)
	return m_tiles[TileIndex(x, y)].m_type != TILE_TYPE_UNSEEN;
}

//-----------------------------------------------------------------------------------------------
// Movement Queries (runtime AgentTypeInfo, never hardcoded)
//-----------------------------------------------------------------------------------------------

bool GameMap::IsPassable(short x, short y, eAgentType type) const
{
	if (!InBounds(x, y))
		return false;

	eTileType tileType = m_tiles[TileIndex(x, y)].m_type;
	if (tileType == TILE_TYPE_UNSEEN)
		return false;
	if (tileType >= NUM_TILE_TYPES)
		return false;

	// A5 restriction: never enter water
	if (tileType == TILE_TYPE_WATER)
		return false;

	return m_agentTypeInfos[type].moveExhaustPenalties[tileType] != TILE_IMPASSABLE;
}

int GameMap::GetMoveExhaust(short x, short y, eAgentType type) const
{
	if (!InBounds(x, y))
		return TILE_IMPASSABLE;

	eTileType tileType = m_tiles[TileIndex(x, y)].m_type;
	if (tileType == TILE_TYPE_UNSEEN || tileType >= NUM_TILE_TYPES)
		return TILE_IMPASSABLE;

	return m_agentTypeInfos[type].moveExhaustPenalties[tileType];
}

bool GameMap::CanDig(short x, short y, eAgentType type) const
{
	if (!InBounds(x, y))
		return false;

	eTileType tileType = m_tiles[TileIndex(x, y)].m_type;
	if (tileType == TILE_TYPE_UNSEEN || tileType >= NUM_TILE_TYPES)
		return false;

	return m_agentTypeInfos[type].digExhaustPenalties[tileType] != DIG_IMPOSSIBLE;
}

//-----------------------------------------------------------------------------------------------
// BFS Pathfinding
//-----------------------------------------------------------------------------------------------

eOrderCode GameMap::GetNextMoveToward(short fromX, short fromY, short toX, short toY, eAgentType type) const
{
	if (fromX == toX && fromY == toY)
		return ORDER_HOLD;

	if (!InBounds(toX, toY))
		return ORDER_HOLD;

	// BFS from source, tracking the first move direction taken
	bool visited[MAX_ARENA_TILES];
	memset(visited, 0, sizeof(visited));

	BFSNode queue[MAX_ARENA_TILES];
	int head = 0;
	int tail = 0;

	visited[TileIndex(fromX, fromY)] = true;

	// Seed BFS with the 4 neighbors of the start tile
	for (int dir = 0; dir < 4; ++dir)
	{
		short nx = fromX + static_cast<short>(DX[dir]);
		short ny = fromY + static_cast<short>(DY[dir]);

		if (!InBounds(nx, ny))
			continue;

		int ni = TileIndex(nx, ny);
		if (visited[ni])
			continue;

		if (!IsPassable(nx, ny, type))
			continue;

		visited[ni] = true;

		if (nx == toX && ny == toY)
			return MOVE_ORDERS[dir];

		queue[tail++] = { nx, ny, MOVE_ORDERS[dir] };
	}

	// BFS loop
	while (head < tail)
	{
		BFSNode current = queue[head++];

		for (int dir = 0; dir < 4; ++dir)
		{
			short nx = current.m_x + static_cast<short>(DX[dir]);
			short ny = current.m_y + static_cast<short>(DY[dir]);

			if (!InBounds(nx, ny))
				continue;

			int ni = TileIndex(nx, ny);
			if (visited[ni])
				continue;

			if (!IsPassable(nx, ny, type))
				continue;

			visited[ni] = true;

			if (nx == toX && ny == toY)
				return current.m_firstMove;

			queue[tail++] = { nx, ny, current.m_firstMove };
		}
	}

	return ORDER_HOLD; // Unreachable
}

//-----------------------------------------------------------------------------------------------
// Search Queries
//-----------------------------------------------------------------------------------------------

bool GameMap::FindNearestFood(short fromX, short fromY, short& outX, short& outY, eAgentType type) const
{
	bool visited[MAX_ARENA_TILES];
	memset(visited, 0, sizeof(visited));

	BFSNode queue[MAX_ARENA_TILES];
	int head = 0;
	int tail = 0;

	int startIdx = TileIndex(fromX, fromY);
	visited[startIdx] = true;
	queue[tail++] = { fromX, fromY, ORDER_HOLD };

	while (head < tail)
	{
		BFSNode current = queue[head++];

		// Check if this tile has food (skip start tile — we want to move to food)
		if ((current.m_x != fromX || current.m_y != fromY) && HasFood(current.m_x, current.m_y))
		{
			outX = current.m_x;
			outY = current.m_y;
			return true;
		}

		for (int dir = 0; dir < 4; ++dir)
		{
			short nx = current.m_x + static_cast<short>(DX[dir]);
			short ny = current.m_y + static_cast<short>(DY[dir]);

			if (!InBounds(nx, ny))
				continue;

			int ni = TileIndex(nx, ny);
			if (visited[ni])
				continue;

			if (!IsPassable(nx, ny, type))
				continue;

			visited[ni] = true;
			queue[tail++] = { nx, ny, ORDER_HOLD };
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------
bool GameMap::FindNearestUnexplored(short fromX, short fromY, short& outX, short& outY, eAgentType type) const
{
	bool visited[MAX_ARENA_TILES];
	memset(visited, 0, sizeof(visited));

	BFSNode queue[MAX_ARENA_TILES];
	int head = 0;
	int tail = 0;

	int startIdx = TileIndex(fromX, fromY);
	visited[startIdx] = true;
	queue[tail++] = { fromX, fromY, ORDER_HOLD };

	while (head < tail)
	{
		BFSNode current = queue[head++];

		for (int dir = 0; dir < 4; ++dir)
		{
			short nx = current.m_x + static_cast<short>(DX[dir]);
			short ny = current.m_y + static_cast<short>(DY[dir]);

			if (!InBounds(nx, ny))
				continue;

			int ni = TileIndex(nx, ny);
			if (visited[ni])
				continue;

			// If this neighbor is unseen, we found an exploration target
			// Return the last passable tile adjacent to it
			if (m_tiles[ni].m_type == TILE_TYPE_UNSEEN)
			{
				outX = current.m_x;
				outY = current.m_y;
				return true;
			}

			if (!IsPassable(nx, ny, type))
				continue;

			visited[ni] = true;
			queue[tail++] = { nx, ny, ORDER_HOLD };
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------
bool GameMap::FindNearestDigTarget(short fromX, short fromY, short& outX, short& outY, eAgentType type) const
{
	bool visited[MAX_ARENA_TILES];
	memset(visited, 0, sizeof(visited));

	BFSNode queue[MAX_ARENA_TILES];
	int head = 0;
	int tail = 0;

	int startIdx = TileIndex(fromX, fromY);
	visited[startIdx] = true;
	queue[tail++] = { fromX, fromY, ORDER_HOLD };

	while (head < tail)
	{
		BFSNode current = queue[head++];

		for (int dir = 0; dir < 4; ++dir)
		{
			short nx = current.m_x + static_cast<short>(DX[dir]);
			short ny = current.m_y + static_cast<short>(DY[dir]);

			if (!InBounds(nx, ny))
				continue;

			int ni = TileIndex(nx, ny);
			if (visited[ni])
				continue;

			// If this neighbor is diggable, return its position
			if (CanDig(nx, ny, type))
			{
				outX = nx;
				outY = ny;
				return true;
			}

			if (!IsPassable(nx, ny, type))
				continue;

			visited[ni] = true;
			queue[tail++] = { nx, ny, ORDER_HOLD };
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------
// Exploration Heat Map
//-----------------------------------------------------------------------------------------------

void GameMap::UpdateExplorationHeat(int turnNumber)
{
	// Decay existing values so stale targets get deprioritized
	m_explorationHeat.Decay(0.95f);

	int totalTiles = m_width * m_width;
	for (int i = 0; i < totalTiles; ++i)
	{
		short x = static_cast<short>(i % m_width);
		short y = static_cast<short>(i / m_width);

		if (m_tiles[i].m_type == TILE_TYPE_UNSEEN)
		{
			// Unseen tiles get high base value
			m_explorationHeat.SetValue(x, y, 10.f);
		}
		else
		{
			// Frontier bonus: passable tiles adjacent to unseen tiles
			bool nearUnseen = false;
			for (int dir = 0; dir < 4; ++dir)
			{
				short nx = x + static_cast<short>(DX[dir]);
				short ny = y + static_cast<short>(DY[dir]);
				if (InBounds(nx, ny) && m_tiles[TileIndex(nx, ny)].m_type == TILE_TYPE_UNSEEN)
				{
					nearUnseen = true;
					break;
				}
			}

			if (nearUnseen)
			{
				// Frontier tiles get moderate value
				m_explorationHeat.AddValue(x, y, 5.f);
			}
			else
			{
				// Recently seen tiles get low value based on staleness
				int staleness = turnNumber - m_tiles[i].m_lastSeenTurn;
				float staleBonus = std::min(static_cast<float>(staleness) * 0.01f, 1.f);
				m_explorationHeat.SetValue(x, y, staleBonus);
			}
		}
	}
}
