#include "HeatMap.hpp"

//-----------------------------------------------------------------------------------------------
HeatMap::HeatMap()
	: m_width(0)
{
	memset(m_values, 0, sizeof(m_values));
}

//-----------------------------------------------------------------------------------------------
void HeatMap::Initialize(short mapWidth)
{
	m_width = mapWidth;
	Reset();
}

//-----------------------------------------------------------------------------------------------
void HeatMap::Reset()
{
	memset(m_values, 0, sizeof(m_values));
}

//-----------------------------------------------------------------------------------------------
void HeatMap::SetValue(short x, short y, float value)
{
	if (!InBounds(x, y))
		return;
	m_values[TileIndex(x, y)] = value;
}

//-----------------------------------------------------------------------------------------------
float HeatMap::GetValue(short x, short y) const
{
	if (!InBounds(x, y))
		return 0.f;
	return m_values[TileIndex(x, y)];
}

//-----------------------------------------------------------------------------------------------
void HeatMap::AddValue(short x, short y, float delta)
{
	if (!InBounds(x, y))
		return;
	m_values[TileIndex(x, y)] += delta;
}

//-----------------------------------------------------------------------------------------------
void HeatMap::Decay(float factor)
{
	int totalTiles = m_width * m_width;
	for (int i = 0; i < totalTiles; ++i)
	{
		m_values[i] *= factor;
	}
}

//-----------------------------------------------------------------------------------------------
bool HeatMap::FindMax(short& outX, short& outY, float& outValue) const
{
	if (m_width == 0)
		return false;

	int totalTiles = m_width * m_width;
	float bestValue = -1.f;
	int bestIndex = -1;

	for (int i = 0; i < totalTiles; ++i)
	{
		if (m_values[i] > bestValue)
		{
			bestValue = m_values[i];
			bestIndex = i;
		}
	}

	if (bestIndex < 0 || bestValue <= 0.f)
		return false;

	outX = static_cast<short>(bestIndex % m_width);
	outY = static_cast<short>(bestIndex / m_width);
	outValue = bestValue;
	return true;
}
