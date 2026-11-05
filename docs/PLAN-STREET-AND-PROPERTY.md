# Plan: commercial street + expandable property (Option B)

Status: PLAN ONLY (2026-10-27). Nothing below is built yet. Decisions are Brandon's (see his brief); open questions at the end.

## What City Sample actually gives us (inspected from files, not yet eyeballed in-editor)

Source: `D:\Unreal Projects\CitySample_Work` (copy; original `CitySample` untouched). Building content is ~13 GB, props ~12.5 GB - we migrate only picked pieces, never the project.

- Buildings are **modular wall kits**, not whole meshes: `Wall`, `WallL/R`, `CornerEx/In(L/R)`, `Column`, plus **dedicated `Entrance_NN` modules (192 of them)**. Family prefix = style (CH / NY / NYA / SF...), `_L<n>_` = floors.
- Low-rise is plentiful: ~30 variants at 1-3 floors (e.g. `CHB_L3_A`, `CHE_L1/L2/L3`, `SFD_L2_A`, `NYA_L2_*`). CHE_L1 alone has six entrance variants.
- Shopfront dressing exists as separate props: awnings (NYA 11 variants, `Kit_Prop_BA/BB`), marquee, light signs (`Kit_LightSign` 13), banners, numbers, plaques, fire escapes, AC units, lamps, clocks.
- Street props live in `Content/Prop` and `Content/Environment` (cars, hydrants, mailboxes, lamps, crosswalk decals) - not yet audited in detail.
- NOT found: any mesh named store/shop/retail/storefront. A real glass shopfront is not a ready-made piece. "Entrance" modules are the closest; whether they read as shop doors or apartment stoops must be checked visually (the red NYA corner building has stoops).

## Recommended combination

| Layer | Source |
|---|---|
| Street, sidewalks, crosswalks, lamps, cars, props | City Sample (migrate picked pieces) |
| Neighbouring buildings (low/mid-rise, mixed commercial + residential) | City Sample kits L2-L5, CH/NYA/SF families |
| Distant skyline | a few City Sample towers placed far away (cheap, Nanite) |
| Upper floors of our store, cornice, parapet | City Sample wall/column/corner modules so it matches the street |
| **Store ground floor: glass facade, door, signage, interior** | **OURS** - custom modular shopfront |
| Expansion units next door | OURS (same shopfront module, re-skinned) |

## Architecture for the custom store (data-driven, no rewrite to add units/floors)

- `UTSSPropertyDef` (data asset): list of **units** - id, floor index, footprint (grid rect), price, requires (unit ids), facade style, doorway links.
- `ATSSPropertyUnit`: spawns its own floor/walls/ceiling from small wall segments (HISM). Wall segments between two units carry an **opening** that stays solid until the unit is bought, then is replaced by a doorway.
- `UTSSPropertySubsystem`: owned-unit set, `CanBuy/Buy(unitId)`, broadcasts a delegate; costs go through the existing `TSSStoreSubsystem` cash.
- State is keyed by a **StoreId** (default `"main"`) from day one, so multiple locations later is a second key, not a rewrite.
- Floors are just units with a floor index plus a stair unit (stairs/lift) that links them.
- Interior customising reuses the existing `TSSShelf`/`TSSCrate` actors; add grid-snapped placement later (build mode is a separate milestone).
- Keep: View settings menu, first/third person camera, current `Store_Main` slice. New work goes in a new map `Town_Main`; `Store_Main` stays as the test bed and `-TSSAutoTest` target.

## Milestones

**M0 (this step, done):** inspect assets, write this plan.

**M1 - prototype (first thing to build):**
1. Pick the street pieces in the editor (needs the editor; one short session) and note the list in this file.
2. Migrate ONLY those pieces into this project under `Content/CitySampleLite/` (git-ignored; nothing from City Sample is committed).
3. `Town_Main`: one straight street, one block, 2-3 neighbouring buildings, our single-floor shop (unit 1) with glass front + door + sign + small interior, reusing existing shelves/crates. Streaming/lighting sanity check on the RX 7800 XT.
4. `-TSSAutoTest` still passes; Store_Main still works.

**M2 - expansion core:** `UTSSPropertyDef`, `ATSSPropertyUnit`, subsystem, buy a neighbouring unit (opening becomes a doorway), HUD prompt + price. Saved owned-unit state.

**M3 - floors + storage:** stair unit, upper floor unit, warehouse/storage unit type (stock lives there).

**M4 - customising:** grid-snapped place/rotate/move for shelves, counters, decorations; renovations (floor/wall materials).

**Later:** more property defs, second store location (StoreId), polish.

## Rules / safety
- Work only in this project and `CitySample_Work` (read). Never modify LIFE or other Nova Games projects.
- No paid downloads; no pushes/releases without asking. Don't delete project content without approval.
- Nothing big on C:. Zen cache is on D: (`D:\UnrealZenData`).
- Never commit City Sample content; check size first and use `.gitignore`.

## Open questions
1. Do the CH/NYA `Entrance` modules look like shop doors in-editor, or only apartment entries? (decides how much of the ground floor is ours)
2. Preferred architectural flavour: NY-style brick, Chicago-style, or SF-style? (so store + neighbours match)
3. Store footprint for unit 1 (current test room is 14 x 10 m).
