#pragma once

//-----------------------------------------------------------------------------------------------
// DebugRenderer.hpp
//
// Wraps DebugInterface for structured debug output and visualization.
//-----------------------------------------------------------------------------------------------

#include "Common.hpp"

class DebugRenderer
{
public:
	DebugRenderer() = default;
	void Initialize(DebugInterface* debugInterface, Color8 playerColor);

	// TODO: Implementation in Task 7

private:
	DebugInterface* m_debug = nullptr;
	Color8 m_playerColor;
};
