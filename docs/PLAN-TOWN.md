# Plan: the town (release 1) - hybrid layers, no City Sample

Status 2026-11-08. Replaces the City Sample route in docs/PLAN-STREET-AND-PROPERTY.md for the first release (the property-expansion architecture in that file still applies). City Sample is shelved, not deleted.

## Why not City Sample (Brandon, 2026-11-08)
Complexity (made for a huge city), performance (heavy render/memory), mostly assets we do not need, more time adapting than building gameplay, and a dense modern-city look that does not suit a small-town tech shop. RX 7800 XT / 16 GB VRAM / 32 GB RAM must not be sized around it.

## Layers
| Layer | Contents | Detail | How |
|---|---|---|---|
| 1 Store | the shop, interior, door | very high | hand-built (existing) |
| 2 Immediate street | road, pavement, parking, neighbours | high | hand-authored layout + street kit |
| 3 Town blocks | offices, houses, restaurants | medium | modular building kit, hand-placed |
| 4 Background | distant buildings, trees, skyline | low | cheap blocks + tree line, no interiors, no windows lights |

## Hybrid rule
Hand-built important places, modular kits for buildings, scatter for decoration (trees, bins, litter), hand-authored roads. Not fully procedural.

## Engine tech: use / defer
- Use now: spline roads (`ATSSRoad`), Lumen + Nanite (project defaults), instanced meshes for repeated parts.
- Use when needed: PCG for scatter (only once hand-placing becomes painful).
- Defer: World Partition + HLOD (only if the town grows beyond one street area).

## Reference (Supermarket Simulator / Waterpark Simulator)
Brick mid-rise (3-5 floors), pale paving slabs with joints, tree-lined kerbs, hydrants, bins, benches, bollards, fire escapes, AC units, awnings + shop signs, parked cars, street signs, lamps with arms.

## Milestones
T1 street kit (sidewalk joints, trees, bins, hydrants, benches, bollards, signs, parked cars).
T2 building kit v2 (cornice, sills, fire escapes, AC units, sign boards, roof props, more styles).
T3 background ring (layer 4).
T4 polish: lighting/colour pass, litter, tuning for frame rate.
Each milestone: build, `-TSSStreetShots`, look at screenshots, `-TSSAutoTest` must pass.
