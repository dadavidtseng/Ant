#pragma once

//-----------------------------------------------------------------------------------------------
// DebugRenderer.hpp
//
// Wraps DebugInterface for structured debug output and visualization.
// All 5 required debug calls: LogText, SetMoodText, QueueDrawWorldText,
// QueueDrawVertexArray, FlushQueuedDraws.
//-----------------------------------------------------------------------------------------------

#include "Common.hpp"

class Ant;
class GameMap;
class HeatMap;

class DebugRenderer
{
public:
	DebugRenderer() = default;
	void Initialize(DebugInterface* debugInterface, Color8 playerColor);

	// REQ-5: Required debug interface calls
	void LogText(const char* format, ...);
	void SetMoodText(const char* format, ...);
	void DrawWorldText(float x, float y, float height, Color8 color, const char* format, ...);
	void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, Color8 color);
	void DrawTileHighlight(short tileX, short tileY, Color8 color);
	void Flush();

	// Conditional drawing
	bool IsActive() const;

	// High-level debug views
	void DrawAntRoles(const Ant* ants, int maxAnts);
	void DrawFoodLocations(const GameMap& map);
	void DrawExplorationHeat(const HeatMap& heatMap, short mapWidth);
	void DrawTestTriangle();

private:
	DebugInterface* m_debug      = nullptr;
	Color8          m_playerColor;

	static constexpr int BUFFER_SIZE = 512;
};
