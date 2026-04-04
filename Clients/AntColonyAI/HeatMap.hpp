#pragma once

//-----------------------------------------------------------------------------------------------
// HeatMap.hpp
//
// Fixed-size float grid for scoring tiles by exploration value, food proximity, danger, etc.
// Used by GameMap to prioritize exploration targets.
//-----------------------------------------------------------------------------------------------

#include "Common.hpp"

class HeatMap
{
public:
	HeatMap();

	void Initialize(short mapWidth);
	void Reset();

	// Per-tile scoring
	void  SetValue(short x, short y, float value);
	float GetValue(short x, short y) const;
	void  AddValue(short x, short y, float delta);

	// Decay all values by a factor (e.g. 0.95 reduces all by 5%)
	void Decay(float factor);

	// Find the tile with the highest score
	bool FindMax(short& outX, short& outY, float& outValue) const;

	short GetWidth() const { return m_width; }

private:
	float m_values[MAX_ARENA_TILES];
	short m_width = 0;

	int  TileIndex(short x, short y) const { return y * m_width + x; }
	bool InBounds(short x, short y) const { return x >= 0 && x < m_width && y >= 0 && y < m_width; }
};
