# Tech Store Simulator

Unreal Engine 5.8 (5.8.4) Blueprint project. Open `TechStoreSimulator.uproject`.
Startup map: `/Game/Maps/Store_Main` (floor, sun, sky, PlayerStart). `Scripts/create_main_map.py` regenerates it.

## Gameplay slice (2026-10-26)
Walk the store, buy boxes at the wholesale crates (E), stock them on shelves (E), take them back (E), change a product's shelf price (mouse wheel or [ ]).
- C++ in `Source/TechStoreSimulator/`: `TSSProducts` (catalogue), `TSSStoreSubsystem` (cash, prices), `TSSShelf`, `TSSCrate`, `TSSBlock`, `TSSHudWidget`, interaction trace in `TSSCharacter`.
- `Scripts/build_store_map.py` rebuilds the store in `Store_Main` (idempotent). Headless: `UnrealEditor-Cmd.exe TechStoreSimulator.uproject /Game/Maps/Store_Main -ExecutePythonScript="Scripts/build_store_map.py" -unattended -nosplash`
- Self-test + screenshots: launch the game with `-TSSAutoTest` (logs `TSSTEST PASS/FAIL`, shots in `Saved/Screenshots`). Console: `TSSSelfTest`, `TSSLook X Y Z Yaw Pitch`, `TSSStart`.
- Engine: UE 5.8.4 at `D:\Games\Epic Games\UE_5.8.3\UE_5.8`. Build.bat must be run from PowerShell with the call operator (the space in the path breaks bash/cmd).
