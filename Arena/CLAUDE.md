[Root Directory](../CLAUDE.md) > **Arena**

# Arena Module -- API Contract and Server Runtime

## Changelog

| Date       | Action  | Notes                          |
|------------|---------|--------------------------------|
| 2026-04-05 | Created | Initial scan                   |

## Module Responsibilities

Contains the read-only API contract (`ArenaPlayerInterface.hpp`) that defines the DLL-EXE interface, plus the arena server binary and all data-driven configuration files. Nothing here should be modified by the AI developer except XML configs for testing.

## Entry and Startup

- `Run_Windows/AntArena2_x64.exe` -- the arena server. Loads player DLLs from `Run_Windows/Players/`.
- Runtime keys: P=play/pause, SPACE=step turn, 1-9=debug draw per player, R=restart match

## External Interfaces

### ArenaPlayerInterface.hpp (`Code/Game/`)

Interface version: 12. Defines:

- Type aliases: `TeamID`, `PlayerID`, `AgentID`
- Constants: `MAX_ARENA_WIDTH` (256), `MAX_AGENTS_PER_PLAYER` (256), `MAX_PLAYERS` (32)
- Enums: `eFaultType`, `eAgentType` (Scout/Worker/Soldier/Queen), `eOrderCode` (22 orders), `eAgentOrderResult` (25 results), `eAgentState`, `eTileType` (Air/Dirt/Stone/Water/CorpseBridge/Unseen)
- Structs: `Color8`, `VertexPC`, `AgentTypeInfo`, `MatchInfo`, `PlayerInfo`, `AgentReport`, `ObservedAgent`, `AgentOrder`, `PlayerTurnOrders`, `StartupInfo`, `ArenaTurnStateForPlayer`, `MatchResults`, `DebugInterface`
- DLL exports (7 functions): `GiveCommonInterfaceVersion`, `GivePlayerName`, `GiveAuthorName`, `PreGameStartup`, `PostGameShutdown`, `PlayerThreadEntry`, `ReceiveTurnState`, `TurnOrderRequest`

## Key Configuration Files

| File | Purpose |
|------|---------|
| `Run_Windows/Data/AgentDefinitions.xml` | Ant type stats (cost, upkeep, visibility, combat, movement/dig penalties) |
| `Run_Windows/Data/MatchDefinitions.xml` | Match rules (nutrients, sudden death, population cap, fault penalties) |
| `Run_Windows/Data/MapDefinitions.xml` | Map generation (10 maps: SoloSurvivalTest, Default, Final, Fertile Fields, Big Blue, Mordor, Siberia, Aggression Alley, Estonia, Labyrinth, Gigantor) |

### Default Agent Stats (from AgentDefinitions.xml)

| Type | Cost | Upkeep | Visibility | Combat | Can Carry | Can Birth | Move through Dirt |
|------|------|--------|------------|--------|-----------|-----------|-------------------|
| Scout | 500 | 1 | 8 | 1 | No | No | Yes (0 exhaust) |
| Worker | 500 | 2 | 4 | 1 | Food+Tiles | No | Yes (1 exhaust) |
| Soldier | 500 | 3 | 4 | 2 | No | No | No |
| Queen | 10000 | 10 | 6 | 0 | No | Yes | No (1 exhaust on air/water/bridge) |

## Opponent DLLs

Located in `Run_Windows/Storehouse/`:
- `AI_Benchmark.dll`, `AI_Demo.dll` -- baseline opponents
- `Faculty/` -- AI_Butler.dll, AI_Forseth.dll
- `HallOfFame/` -- top past cohort AIs (C28, C32, C33)
- `C28/` through `C33/` -- student AIs from past cohorts

## Related File List

| File | Role |
|------|------|
| `Code/Game/ArenaPlayerInterface.hpp` | DLL-EXE interface contract (read-only) |
| `Run_Windows/AntArena2_x64.exe` | Arena server binary |
| `Run_Windows/Data/AgentDefinitions.xml` | Ant type configuration |
| `Run_Windows/Data/MatchDefinitions.xml` | Match rules |
| `Run_Windows/Data/MapDefinitions.xml` | Map generation definitions |
| `Run_Windows/Players/` | Drop DLLs here to compete |
| `ReadMe.txt` | Arena version history and instructions |
