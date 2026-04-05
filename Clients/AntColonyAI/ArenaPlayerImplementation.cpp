#include "Colony.hpp"

//-----------------------------------------------------------------------------------------------
// ArenaPlayerImplementation.cpp
//
// Thin DLL export layer. No AI logic — only delegates to Colony.
//-----------------------------------------------------------------------------------------------

static Colony*            g_colony    = nullptr;
static std::atomic<bool>  g_isQuitting = false;

//-----------------------------------------------------------------------------------------------
// Pre-game player information
//-----------------------------------------------------------------------------------------------

int GiveCommonInterfaceVersion()
{
	return COMMON_INTERFACE_VERSION_NUMBER;
}

const char* GivePlayerName()
{
	return "Formicidae";
}

const char* GiveAuthorName()
{
	return "Yu-Wei Tseng";
}

//-----------------------------------------------------------------------------------------------
// Game start & end
//-----------------------------------------------------------------------------------------------

void PreGameStartup(const StartupInfo& info)
{
	g_isQuitting = false;
	g_colony = new Colony(info);
}

void PostGameShutdown(const MatchResults& results)
{
	(void)results;
	g_isQuitting = true;

	if (g_colony)
	{
		delete g_colony;
		g_colony = nullptr;
	}
}

//-----------------------------------------------------------------------------------------------
// Worker thread entry
//-----------------------------------------------------------------------------------------------

void PlayerThreadEntry(int yourThreadIdx)
{
	if (g_colony)
		g_colony->WorkerLoop(yourThreadIdx, &g_isQuitting);
}

//-----------------------------------------------------------------------------------------------
// Per-turn data exchange (EXE thread, must return sub-1ms)
//-----------------------------------------------------------------------------------------------

void ReceiveTurnState(const ArenaTurnStateForPlayer& state)
{
	if (g_colony)
		g_colony->OnReceiveTurnState(state);
}

bool TurnOrderRequest(int turnNumber, PlayerTurnOrders* out_ordersToFill)
{
	if (!g_colony)
		return false;

	return g_colony->OnTurnOrderRequest(turnNumber, out_ordersToFill);
}
