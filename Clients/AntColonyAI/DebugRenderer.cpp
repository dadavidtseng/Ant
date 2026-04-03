#include "DebugRenderer.hpp"

//-----------------------------------------------------------------------------------------------
void DebugRenderer::Initialize(DebugInterface* debugInterface, Color8 playerColor)
{
	m_debug = debugInterface;
	m_playerColor = playerColor;
}

// TODO: Full implementation in Task 7
