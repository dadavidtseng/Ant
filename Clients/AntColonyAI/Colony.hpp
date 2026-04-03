#pragma once
#include "Common.hpp"
#include "Ant.hpp"

//-----------------------------------------------------------------------------------------------
class Colony
{
public: 
	Colony(StartupInfo const& startupInfo);

	StartupInfo m_startupInfo;
	eTileType m_mapTiles[MAX_ARENA_TILES];
	Ant m_myAnts[MAX_AGENTS_PER_PLAYER];	// Object Pool
};