[Root Directory](../../CLAUDE.md) > [Clients](..) > **AntColonyAI**

# AntColonyAI Module -- AI DLL Source

## Changelog

| Date       | Action  | Notes                          |
|------------|---------|--------------------------------|
| 2026-04-05 | Created | Initial scan, all files covered |

## Module Responsibilities

This is the entire AI codebase. It compiles to `AI_YuWeiTseng.dll` which the AntArena2 server loads at runtime. The AI controls a colony of ants (queen, scouts, workers, soldiers) through a turn-based pipeline: receive state -> update map -> manage economy -> assign roles -> generate orders -> render debug.

## Entry and Startup

- `ArenaPlayerImplementation.cpp` -- DLL export layer. Implements the 7 required `extern "C"` functions from `ArenaPlayerInterface.hpp`. Creates/destroys a static `Colony*` instance. Zero AI logic here.
- `Colony::Colony(const StartupInfo&)` -- initializes GameMap, DebugRenderer, stores MatchInfo/PlayerInfo
- `Colony::WorkerLoop(int, atomic<bool>*)` -- AI main loop on thread 0; blocks on `TripleBuffer::WaitForNewData()`

## External Interfaces (DLL Exports)

All defined in `ArenaPlayerImplementation.cpp`:

| Export | Purpose |
|--------|---------|
| `GiveCommonInterfaceVersion()` | Returns `COMMON_INTERFACE_VERSION_NUMBER` (12) |
| `GivePlayerName()` | Returns `"Formicidae"` |
| `GiveAuthorName()` | Returns `"Yu-Wei Tseng"` |
| `PreGameStartup(StartupInfo&)` | Creates Colony, stores match config |
| `PostGameShutdown(MatchResults&)` | Signals shutdown, deletes Colony |
| `PlayerThreadEntry(int)` | Worker thread entry; thread 0 runs AI loop |
| `ReceiveTurnState(ArenaTurnStateForPlayer&)` | Copies state into TripleBuffer (sub-1ms) |
| `TurnOrderRequest(int, PlayerTurnOrders*)` | Reads orders from TripleBuffer (sub-1ms) |

## Class Architecture

### Colony (`Colony.hpp/cpp`) -- Central Orchestrator
- Owns: `GameMap`, `AntManager`, `DebugRenderer` (by value)
- Owns: `TripleBuffer<ArenaTurnStateForPlayer>`, `TripleBuffer<PlayerTurnOrders>`
- AI pipeline: `UpdateGameState` -> `ManageEconomy` -> `AssignRoles` -> `GenerateAntOrders` -> `RenderDebug`
- Economy: conservative birthing (needs 20x upkeep reserve), proactive downsizing, sudden death awareness
- Birth composition target: up to 8 workers + 3 scouts + 1 queen = 12 ants, ~29 upkeep/turn

### GameMap (`GameMap.hpp/cpp`) -- Tile Grid and Pathfinding
- Fixed-size `TileInfo[MAX_ARENA_TILES]` array with fog-of-war (retains last-known state for unseen tiles)
- BFS pathfinding: `GetNextMoveToward()` returns the first-step `eOrderCode` toward a target
- BFS search: `FindNearestFood()`, `FindNearestUnexplored()`, `FindNearestDigTarget()`
- Movement validation via `AgentTypeInfo.moveExhaustPenalties[]` -- never hardcoded
- Owns `HeatMap` for exploration scoring (frontier bonus, staleness decay)

### AntManager (`AntManager.hpp/cpp`) -- Ant Pool and Roles
- Fixed pool: `Ant[MAX_AGENTS_PER_PLAYER]` (256 slots)
- Syncs from `AgentReport[]` each turn; tracks births, deaths, state changes
- Role assignment per type: queens->Queen, scouts->Explorer, workers->Gatherer/Deliverer/Explorer, soldiers->Guard
- Auto-promotes workers carrying food to Deliverer, demotes back to Gatherer on drop
- Enemy tracking: rebuilds `ObservedAgent[]` each turn
- Template iterators: `ForEachLivingAnt()`, `ForEachLivingAntOfType()`

### Ant (`Ant.hpp/cpp`) -- Per-Ant State and Orders
- Roles: `Idle`, `Explorer`, `Gatherer`, `Deliverer`, `Digger`, `Guard`, `Queen`
- `DecideOrder()` dispatches to role-specific methods
- Explorer: BFS to nearest unseen frontier
- Gatherer: BFS to nearest food, pick up
- Deliverer: BFS to queen, drop food on her tile
- Digger: BFS to nearest diggable tile, dig adjacent
- Guard: stay within taxicab distance 1 of queen

### HeatMap (`HeatMap.hpp/cpp`) -- Float Scoring Grid
- `float[MAX_ARENA_TILES]` with per-tile set/add/decay
- Used by GameMap for exploration prioritization
- Decay factor 0.95 per turn; unseen=10, frontier=+5, stale tiles scale with age

### TripleBuffer (`TripleBuffer.hpp`) -- Thread-Safe Data Exchange
- Header-only template; 3 buffers with write/middle/read index swapping
- `mutex` + `condition_variable` for blocking wait
- `PublishWrite()` / `TrySwapRead()` / `WaitForNewData()` / `SignalShutdown()`

### DebugRenderer (`DebugRenderer.hpp/cpp`) -- Debug Visualization
- Wraps `DebugInterface*` from arena (5 required calls: LogText, SetMoodText, QueueDrawWorldText, QueueDrawVertexArray, FlushQueuedDraws)
- Gated by `IsDebugging()` for expensive draws
- Views: ant role overlays (color-coded tiles + labels), food locations, exploration heat, test triangle

### Common (`Common.hpp/cpp`) -- Shared Includes
- Includes `ArenaPlayerInterface.hpp` via relative path `../../Arena/Code/Game/`
- Standard library: `<atomic>`, `<mutex>`, `<condition_variable>`, `<thread>`, `<vector>`, `<algorithm>`, `<queue>`, `<cstring>`, `<cstdarg>`, `<cstdio>`

## Key Dependencies and Configuration

- Build: MSVC v143, C++17, x64 DLL, `/W3 /WX /SDL`
- External dependency: only `ArenaPlayerInterface.hpp` (read-only, provided by arena)
- No third-party libraries; STL only
- Post-build: `xcopy` copies DLL to `Arena/Run_Windows/Players/`
- Preprocessor: `AIYUWEITSENG_EXPORTS`, `_WINDOWS`, `_USRDLL`

## Data Models

All data structures are defined in `ArenaPlayerInterface.hpp` (read-only):
- `ArenaTurnStateForPlayer` -- per-turn state (map tiles, food, agent reports, observed enemies)
- `AgentReport` -- per-ant status (position, exhaustion, state, last order result)
- `ObservedAgent` -- visible enemy ant info
- `AgentOrder` / `PlayerTurnOrders` -- order submission
- `MatchInfo` / `PlayerInfo` / `AgentTypeInfo` -- match configuration
- `DebugInterface` -- function pointers for debug draw/log

Internal models:
- `Ant` -- per-ant AI state (role, target, alive flag)
- `GameMap::TileInfo` -- tile type + food flag + last-seen turn
- `HeatMap` -- float grid for exploration scoring

## Testing and Quality

- No unit tests; validation is via arena matches
- `/WX` ensures zero-warning builds
- Debug visualization confirms AI behavior at runtime (role overlays, pathfinding)
- Mood text shows live stats: turn, nutrients, upkeep, runway, population, food drought counter

## FAQ

**Q: Where do I add new ant behavior?**
A: Add a new `AntRole` enum value in `Ant.hpp`, implement `Decide<Role>Order()` in `Ant.cpp`, wire it in `DecideOrder()`, and add assignment logic in `AntManager::AssignRoles()`.

**Q: How do I change the birth composition?**
A: Edit `Colony::GetBirthOrder()` in `Colony.cpp`. Current target: 8 workers, 3 scouts, 0 soldiers.

**Q: Why does the queen just hold?**
A: `DecideQueenOrder()` returns `ORDER_HOLD`. Birth orders are injected by `Colony::ManageEconomy()` via target coordinate overloading (`m_targetX` = birth order code, `m_targetY` = 1 as flag).

**Q: How is thread safety handled?**
A: `TripleBuffer<T>` is the only shared state between EXE thread and AI thread. No raw shared variables.

## Related File List

| File | Role |
|------|------|
| `ArenaPlayerImplementation.cpp` | DLL exports (thin wrappers) |
| `Colony.hpp` / `Colony.cpp` | Central orchestrator, economy, AI pipeline |
| `GameMap.hpp` / `GameMap.cpp` | Tile grid, fog-of-war, BFS pathfinding |
| `AntManager.hpp` / `AntManager.cpp` | Ant pool, role assignment, order generation |
| `Ant.hpp` / `Ant.cpp` | Per-ant state, role-based order decisions |
| `HeatMap.hpp` / `HeatMap.cpp` | Float scoring grid for exploration |
| `TripleBuffer.hpp` | Thread-safe triple buffer (header-only) |
| `DebugRenderer.hpp` / `DebugRenderer.cpp` | Debug draw/log wrapper |
| `Common.hpp` / `Common.cpp` | Shared includes and forward declarations |
| `AntColonyAI.vcxproj` | MSBuild project (x64 DLL, v143) |
| `AntColonyAI.sln` | Visual Studio solution |
