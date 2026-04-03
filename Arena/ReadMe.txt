Ant AI Arena 2.0+

Second rewrite of the original version Squirrel Eiserloh created for SMU Guildhall Cohort 15.

===============
INSTRUCTIONS
===============
- Players create DLLs using Code/Game/ArenaPlayerInterface.hpp (public, shared)
- Player DLLs are manually copied into the Run_Windows/Players folder
- Player DLLs are loaded on startup, then cloned into TempPlayers and re-loaded on match start
- Edit Data/GameConfig.txt for game options, including map selection
- Edit Data/MapDefinitions.xml to change or create new procedural map types
- Edit Data/AgentDefinitions.xml to change (within limits) Ant attributes
- Edit Data/MatchDefinitions.xml to change game rules
- Press P at runtime to Play/Pause the action
- Press SPACE to run a single turn then pause
- Press 1-9 for debug draw for player N; press again to cycle modes, 0=clear
- Press R to restart the match (Player DLLs are unloaded then reloaded)
- Launch AntArena2_x64.exe in the Arena/Run_Windows folder

===============
VERSION HISTORY
===============
Version 0: Cohort 15 (2011)
	- Single-threaded (blocking/synchronous Player DLL functions)
	- Basic Ant AI gameplay rules established
	- Stone/Dirt/Air/Food tiles; Queen/Worker/Scout/Soldier ants
	- Player AI DLL interface

Version 1: Cohorts 16-27 (2012-2019)
	- Multi-threaded (one additional thread per Player DLL = 2 threads per player)
	- Added debug draw system, Sudden Death
	- Better data-driving of map & match definitions, game settings, etc.

Version 2: Cohorts 28+ (2020+)
	- Many-threaded (N additional threads per Player DLL)
	- Added the ability to birth new Queens
	- Added Water and Corpse Bridges
	- Added ability for workers to carry & drop dirt
	- Food as tile attribute, not type (food-in-dirt, food-in-water)
	- Added emotes and debug text
	- Dramatically improved UI and debugging features

