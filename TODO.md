# Tech Store Simulator — TODO

## START HERE after restart (2026-11-01)

**Where we are:** nothing from City Sample is migrated yet (no `Content/CitySampleLite/`). We are at "pick the box" (step 5 below). Brandon wants to fly around himself and pick a street block; Claude then looks up the actor in its dump (scratchpad `descs.json`, 12,102 actors with bounds) and computes the cut box.

**Why it stopped:** the City Sample editor crashed with OutOfMemory on 2026-11-01 ("paging file is too small", only ~0.2 GB virtual left; the editor itself used ~14 of 32 GB RAM). Crash logs: `D:\Unreal Projects\CitySample_Work\Saved\Crashes\`.

**Do first:**
1. [ ] Brandon: raise the Windows page file (Advanced system settings > Performance > Advanced > Virtual memory; custom ~40000 MB initial / ~80000 MB max on D:), then reboot.
2. [ ] Open `D:\Unreal Projects\CitySample_Work\CitySample.uproject` with `/Game/Map/Small_City_LVL` (editor: `D:\Games\Epic Games\UE_5.8.3\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe`). Do NOT use Load All in World Partition; load only a small region around the street being looked at.
3. [ ] Brandon: fly to a street with 2-4 floor buildings + sidewalks, click one block, tell Claude the Outliner actor name (e.g. `BLDG_N154005`) or its Location X/Y.
4. [ ] Claude: derive a ~100 m box from the dump, paste those actors into a fresh NON-World-Partition level, then Migrate into this project under `Content/CitySampleLite/`; add that folder to `.gitignore` first.

**Notes:** `256m_767m_L#_X#_Y#` names are HLOD proxy actors (distant simplified meshes), not real buildings; don't use them. Old candidate box C (x 0..10000, y -40000..-30000 cm, harbour-front brick) is only a fallback; little real shopfront. There are also uncommitted game changes in Source/ (TSSCharacter, TSSCrate, etc.): review and commit those separately.

## Next: the town (looks only, high poly) — City Sample cut-down

**Decision (2026-10-26):** use Epic's free **City Sample** (UE 5.8 version, Aug 2026) as a *source* project, cut most of it away, and bring only a few blocks into this game. Woodland District (Fab, $50 license) is the fallback if the migration is more trouble than it is worth.

**Hard rule from Brandon: nothing big goes on C:.** C: has ~150 GB free. D: is a 12 TB HDD (slow); the Intel 512 GB SSD is C:.

### State of the download
- Downloaded ~104 GB into `D:\EpicVaultCache` (verified-and-unpacking phase when paused). `C:\ProgramData\Epic\EpicGamesLauncher\VaultCache` is a **junction** to that D: folder. **Do not delete the folder or the junction until City Sample is fully installed** — resuming depends on it.
- Final install goes to `D:\Unreal Projects\CitySample` (does not exist until the Launcher finishes unpacking).
- To continue: open Epic Games Launcher -> Downloads -> Resume. Decline the ".uproject not associated" prompt.

### Steps
1. [ ] Launcher finishes -> confirm `D:\Unreal Projects\CitySample\CitySample.uproject` exists; check C: free space is still ~150 GB.
2. [ ] **Copy the project** to `D:\Unreal Projects\CitySample_Work` and work only on the copy (original stays pristine).
3. [ ] Before opening: in the work copy's `Config/DefaultEngine.ini` set `[Zen.AutoLaunch] DataPath=D:/UnrealZenData` so Zen's cache does not grow on C:. (`UE-LocalDataCachePath` is not set; the engine DDC already lives on D: inside the engine folder.) Keep an eye on C: while it opens.
4. [ ] Open the work copy in UE 5.8, let shaders/DDC build (slow on the HDD — expect a long first open). Disable the heavy systems first: Mass traffic + crowds (not needed for a shop).
5. [ ] World Partition: pick a few blocks (a main street, ~4-6 buildings with sidewalks/roads) and note the cell/actor bounds. Delete everything outside. Verify the cut area still loads, lights, and runs at a good frame rate on the RX 7800 XT.
6. [ ] Migrate only the chosen meshes/materials/textures into Tech Store Simulator (Migrate tool, or a Python script like `Scripts/build_store_map.py`). Prefer copying onto the SSD-backed project folder.
7. [ ] New map `Town_Main` (or extend `Store_Main`): street + lots, store entrance matching the shop door (+X wall, 240 cm gap, 1400x1000 room).
8. [ ] Re-test with `-TSSAutoTest`; commit in the game repo (do NOT commit City Sample content without checking its size; add to `.gitignore` / use LFS).
9. [ ] When satisfied: delete `D:\EpicVaultCache` (frees ~100 GB) and optionally `CitySample_Work`.

### License note
City Sample is free to use inside Unreal projects (Epic sample content). Re-read the license line on the Fab listing before shipping/selling.

## After the town
- Customers who walk in, pick items and pay at the register (the counter and register are placeholders).
- Selling, daily cash, save/load.
- Lighting/colour pass on the shop interior (it is still washed out).
