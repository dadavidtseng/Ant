#include "../../Arena/Code/Game/ArenaPlayerInterface.hpp"

#include<atomic>

std::atomic<bool> g_isQuitting = false;
char const* COLONY_NAME = "DEFAULT NAME";
char const* AUTHOR_NAME = "DEFAULT AUTHOR";
DebugInterface const* g_debug = nullptr;

int GiveCommonInterfaceVersion()
{
	return COMMON_INTERFACE_VERSION_NUMBER;
}

const char* GivePlayerName()
{
	return COLONY_NAME;
}

const char* GiveAuthorName()
{
	return AUTHOR_NAME;
}

void PreGameStartup(const StartupInfo& info)
{
	g_debug = info.debugInterface;
}

void PostGameShutdown(const MatchResults& results)
{
	g_isQuitting = true;
}

void PlayerThreadEntry(int yourThreadIdx)
{
	if (yourThreadIdx != 0)
		return;	//#TODO: actually use other threads later.

	while (!g_isQuitting)
	{
		float mouseX, mouseY;
		g_debug->GetMouseWorldPos(mouseX, mouseY);
		bool isMouseButtonDown = g_debug->IsKeyDown("LMB");
		bool isSpaceDown = g_debug->IsKeyDown("Space");
		int score = (10 * isMouseButtonDown) + (1 * isSpaceDown);
		if (isSpaceDown)
		{
			g_debug->RequestPause();
		}
		g_debug->IsKeyDown("ESC");
		g_debug->SetMoodText("Mouse is at (%.2f, %.2f), score=%i", mouseX, mouseY, score);
		g_debug->LogText("PreGameStartup");
		g_debug->QueueDrawWorldText(mouseX, mouseY, 0.5f, 0.5f, 5.f, Color8(255, 255, 0,100), "SCORE: %i", score);
		g_debug->FlushQueuedDraws();			
	}
}

void ReceiveTurnState(const ArenaTurnStateForPlayer& state)
{

}

bool TurnOrderRequest(int turnNumber, PlayerTurnOrders* out_ordersToFill)
{ 
	return false;
}