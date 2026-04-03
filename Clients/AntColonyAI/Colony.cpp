#include "Colony.hpp"


Colony::Colony(StartupInfo const& startupInfo)
	:m_startupInfo(startupInfo)
{
	for (int i = 0 ; i < MAX_ARENA_TILES; i++)
	{
		m_mapTiles[i] = TILE_TYPE_UNSEEN;
	}
}
