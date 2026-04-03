#pragma once

//-----------------------------------------------------------------------------------------------
// GameMap.hpp
//
// Tile grid, fog-of-war management, food tracking, and BFS pathfinding.
//-----------------------------------------------------------------------------------------------

#include "Common.hpp"
#include "HeatMap.hpp"

class GameMap
{
public:
	GameMap() = default;
	void Initialize(short mapWidth, const AgentTypeInfo agentTypeInfos[NUM_AGENT_TYPES]);

	// TODO: Implementation in Task 4
};
