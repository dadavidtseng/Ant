#include "DebugRenderer.hpp"
#include "Ant.hpp"
#include "GameMap.hpp"
#include "HeatMap.hpp"

//-----------------------------------------------------------------------------------------------
void DebugRenderer::Initialize(DebugInterface* debugInterface, Color8 playerColor)
{
	m_debug      = debugInterface;
	m_playerColor = playerColor;
}

//-----------------------------------------------------------------------------------------------
// Core Debug Interface Wrappers
//-----------------------------------------------------------------------------------------------

void DebugRenderer::LogText(char const* format, ...)
{
	if (!m_debug || !m_debug->LogText)
		return;

	char buffer[BUFFER_SIZE];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, BUFFER_SIZE, format, args);
	va_end(args);

	m_debug->LogText("%s", buffer);
}

//-----------------------------------------------------------------------------------------------
void DebugRenderer::SetMoodText(char const* format, ...)
{
	if (!m_debug || !m_debug->SetMoodText)
		return;

	char buffer[BUFFER_SIZE];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, BUFFER_SIZE, format, args);
	va_end(args);

	m_debug->SetMoodText("%s", buffer);
}

//-----------------------------------------------------------------------------------------------
void DebugRenderer::DrawWorldText(float x, float y, float height, Color8 color, char const* format, ...)
{
	if (!m_debug || !m_debug->QueueDrawWorldText)
		return;

	char buffer[BUFFER_SIZE];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, BUFFER_SIZE, format, args);
	va_end(args);

	// anchorU=0.5, anchorV=0.5 centers the text on the position
	m_debug->QueueDrawWorldText(x, y, 0.5f, 0.5f, height, color, "%s", buffer);
}

//-----------------------------------------------------------------------------------------------
void DebugRenderer::DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, Color8 color)
{
	if (!m_debug || !m_debug->QueueDrawVertexArray)
		return;

	VertexPC verts[3];
	verts[0] = { x1, y1, color };
	verts[1] = { x2, y2, color };
	verts[2] = { x3, y3, color };

	m_debug->QueueDrawVertexArray(3, verts);
}

//-----------------------------------------------------------------------------------------------
void DebugRenderer::DrawTileHighlight(short tileX, short tileY, Color8 color)
{
	if (!m_debug || !m_debug->QueueDrawVertexArray)
		return;

	// Tile (x,y) center is at (x.0, y.0), extends from (x-0.5, y-0.5) to (x+0.5, y+0.5)
	float minX = static_cast<float>(tileX) - 0.5f;
	float minY = static_cast<float>(tileY) - 0.5f;
	float maxX = static_cast<float>(tileX) + 0.5f;
	float maxY = static_cast<float>(tileY) + 0.5f;

	// Two triangles forming a quad
	VertexPC verts[6];
	verts[0] = { minX, minY, color };
	verts[1] = { maxX, minY, color };
	verts[2] = { maxX, maxY, color };
	verts[3] = { minX, minY, color };
	verts[4] = { maxX, maxY, color };
	verts[5] = { minX, maxY, color };

	m_debug->QueueDrawVertexArray(6, verts);
}

//-----------------------------------------------------------------------------------------------
void DebugRenderer::Flush()
{
	if (!m_debug || !m_debug->FlushQueuedDraws)
		return;

	m_debug->FlushQueuedDraws();
}

//-----------------------------------------------------------------------------------------------
bool DebugRenderer::IsActive() const
{
	if (!m_debug || !m_debug->IsDebugging)
		return false;

	return m_debug->IsDebugging();
}

//-----------------------------------------------------------------------------------------------
// High-Level Debug Views
//-----------------------------------------------------------------------------------------------

void DebugRenderer::DrawAntRoles(Ant const* ants, int maxAnts)
{
	if (!IsActive())
		return;

	for (int i = 0; i < maxAnts; ++i)
	{
		if (!ants[i].m_alive)
			continue;

		Color8 roleColor;
		char const* roleLabel = "";

		switch (ants[i].m_role)
		{
		case AntRole::Queen:     roleColor = Color8(255, 0, 255, 100); roleLabel = "Q";  break;
		case AntRole::Explorer:  roleColor = Color8(0, 128, 255, 100); roleLabel = "Ex"; break;
		case AntRole::Gatherer:  roleColor = Color8(0, 255, 0, 100);   roleLabel = "G";  break;
		case AntRole::Deliverer: roleColor = Color8(255, 255, 0, 100); roleLabel = "D";  break;
		case AntRole::Digger:    roleColor = Color8(180, 100, 50, 100); roleLabel = "Dg"; break;
		case AntRole::Guard:     roleColor = Color8(255, 0, 0, 100);   roleLabel = "Gd"; break;
		case AntRole::Idle:
		default:                 roleColor = Color8(128, 128, 128, 100); roleLabel = "?"; break;
		}

		DrawTileHighlight(ants[i].m_x, ants[i].m_y, roleColor);
		DrawWorldText(
			static_cast<float>(ants[i].m_x),
			static_cast<float>(ants[i].m_y),
			0.4f, Color8(255, 255, 255), "%s", roleLabel
		);
	}

	Flush();
}

//-----------------------------------------------------------------------------------------------
void DebugRenderer::DrawFoodLocations(GameMap const& map)
{
	if (!IsActive())
		return;

	short width = map.GetWidth();
	Color8 foodColor(0, 255, 0, 80);

	for (short y = 0; y < width; ++y)
	{
		for (short x = 0; x < width; ++x)
		{
			if (map.HasFood(x, y))
				DrawTileHighlight(x, y, foodColor);
		}
	}

	Flush();
}

//-----------------------------------------------------------------------------------------------
void DebugRenderer::DrawExplorationHeat(HeatMap const& heatMap, short mapWidth)
{
	if (!IsActive())
		return;

	for (short y = 0; y < mapWidth; ++y)
	{
		for (short x = 0; x < mapWidth; ++x)
		{
			float val = heatMap.GetValue(x, y);
			if (val > 1.f)
			{
				unsigned char intensity = static_cast<unsigned char>(std::min(val * 25.f, 255.f));
				DrawTileHighlight(x, y, Color8(intensity, 0, 0, 60));
			}
		}
	}

	Flush();
}

//-----------------------------------------------------------------------------------------------
void DebugRenderer::DrawTestTriangle()
{
	if (!IsActive())
		return;

	// Draw a small test triangle near origin to satisfy REQ-5 vertex array requirement
	DrawTriangle(1.f, 1.f, 2.f, 1.f, 1.5f, 2.f, m_playerColor);
	Flush();
}
