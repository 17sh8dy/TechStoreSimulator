"""Builds the store interior in /Game/Maps/Store_Main. Safe to re-run: it clears what it made before.

Run headless (from the project root):
  UnrealEditor-Cmd.exe TechStoreSimulator.uproject /Game/Maps/Store_Main -ExecutePythonScript="Scripts/build_store_map.py" -unattended -nosplash
All units are cm. The shop is 1400 x 1000, door in the +X wall, shelves along the -X and -Y walls,
wholesale crates along the +Y wall, a counter (and a register placeholder) by the door.
"""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

# ---- clear previous run + the old placeholder floor ------------------------------------------
MINE = (unreal.TSSBlock, unreal.TSSShelf, unreal.TSSCrate, unreal.PointLight, unreal.PostProcessVolume)
for a in eas.get_all_level_actors():
    if isinstance(a, MINE):
        eas.destroy_actor(a)
    elif isinstance(a, unreal.StaticMeshActor) and a.get_actor_label().startswith("StaticMeshActor"):
        eas.destroy_actor(a)  # the original 40x40 plane from create_main_map.py


def v(x, y, z):
    return unreal.Vector(x, y, z)


def block(label, loc, size, color):
    b = eas.spawn_actor_from_class(unreal.TSSBlock, v(*loc))
    b.set_actor_label(label)
    b.set_editor_property("size", v(*size))
    b.set_editor_property("color", unreal.LinearColor(*color))
    b.rebuild()
    return b


W_X, W_Y = 700.0, 500.0     # half extents of the room
H = 320.0                   # wall height
T = 20.0                    # wall thickness
WALL = (0.93, 0.93, 0.95, 1)

# Ground outside, then the shop floor sitting on it.
block("Ground", (0, 0, -12), (6000, 6000, 20), (0.16, 0.2, 0.17, 1))
block("Floor", (0, 0, -3), (2 * W_X, 2 * W_Y, 6), (0.55, 0.57, 0.6, 1))

# Walls: back (-X), left (-Y), right (+Y), front (+X) with a 240 wide door gap.
block("Wall_Back", (-W_X - T / 2, 0, H / 2), (T, 2 * W_Y + 2 * T, H), WALL)
block("Wall_Left", (0, -W_Y - T / 2, H / 2), (2 * W_X, T, H), WALL)
block("Wall_Right", (0, W_Y + T / 2, H / 2), (2 * W_X, T, H), WALL)
door = 240.0
seg = (W_Y - door / 2)
block("Wall_FrontA", (W_X + T / 2, -(door / 2 + seg / 2), H / 2), (T, seg, H), WALL)
block("Wall_FrontB", (W_X + T / 2, (door / 2 + seg / 2), H / 2), (T, seg, H), WALL)
block("Wall_FrontLintel", (W_X + T / 2, 0, H - 40), (T, door, 80), WALL)
block("Ceiling", (0, 0, H + 10), (2 * W_X + 2 * T, 2 * W_Y + 2 * T, 20), (0.97, 0.97, 0.98, 1))

# Ceiling lights (emissive-looking panels + real lights so the room is lit under the roof).
for ix, x in enumerate((-450, -150, 150, 450)):
    for iy, y in enumerate((-250, 250)):
        block("LightPanel_%d_%d" % (ix, iy), (x, y, H - 1), (120, 60, 3), (1, 1, 0.9, 1))
        pl = eas.spawn_actor_from_class(unreal.PointLight, v(x, y, H - 40))
        pl.set_actor_label("StoreLight_%d_%d" % (ix, iy))
        comp = pl.get_component_by_class(unreal.PointLightComponent)
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)   # static lights need a bake; movable work in-editor and in game
        comp.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
        comp.set_editor_property("intensity", 1500.0)
        comp.set_editor_property("attenuation_radius", 700.0)
        comp.set_editor_property("light_color", unreal.Color(255, 244, 228, 255))

# ---- exposure: the shop is lit by its own lights, so don't let eye adaptation chase the sun ----
ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, v(0, 0, 150))
ppv.set_actor_label("StorePostProcess")
ppv.set_editor_property("unbound", True)
pp = ppv.get_editor_property("settings")
pp.set_editor_property("override_auto_exposure_min_brightness", True)
pp.set_editor_property("override_auto_exposure_max_brightness", True)
pp.set_editor_property("auto_exposure_min_brightness", 1.0)
pp.set_editor_property("auto_exposure_max_brightness", 1.0)
ppv.set_editor_property("settings", pp)

# Sky/sun are for the outside only; keep their ambient from flooding the interior.
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.DirectionalLight):
        a.get_component_by_class(unreal.DirectionalLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    if isinstance(a, unreal.SkyLight):
        a.get_component_by_class(unreal.SkyLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
        a.get_component_by_class(unreal.SkyLightComponent).set_editor_property("intensity", 0.4)

# ---- shelves --------------------------------------------------------------------------------
# Back wall (-X), facing +X.
for i, y in enumerate((-375, -125, 125, 375)):
    s = eas.spawn_actor_from_class(unreal.TSSShelf, v(-W_X + 26, y, 0), unreal.Rotator(0, 0, 0))
    s.set_actor_label("Shelf_Back_%d" % i)
# Left wall (-Y), facing +Y (yaw 90).
for i, x in enumerate((-300, 0, 300)):
    s = eas.spawn_actor_from_class(unreal.TSSShelf, v(x, -W_Y + 26, 0), unreal.Rotator(0, 0, 90))
    s.set_actor_label("Shelf_Left_%d" % i)

# ---- wholesale crates along the right wall (+Y) --------------------------------------------
products = [unreal.TSSProduct.PHONE, unreal.TSSProduct.LAPTOP, unreal.TSSProduct.HEADPHONES,
            unreal.TSSProduct.CONSOLE, unreal.TSSProduct.MONITOR, unreal.TSSProduct.KEYBOARD]
for i, p in enumerate(products):
    c = eas.spawn_actor_from_class(unreal.TSSCrate, v(-300 + i * 130, W_Y - 60, 30), unreal.Rotator(0, 0, 0))
    c.set_editor_property("product", p)
    c.set_actor_label("Crate_%d" % i)
    c.rebuild()

# ---- counter + register placeholder near the door ---------------------------------------------
block("Counter", (480, -250, 50), (70, 260, 100), (0.35, 0.25, 0.18, 1))
block("Register", (480, -300, 112), (30, 35, 24), (0.1, 0.1, 0.12, 1))

# ---- spawn point: just inside the door, looking into the shop ----------------------------------
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.PlayerStart):
        a.set_actor_location(v(560, 0, 100), False, False)
        a.set_actor_rotation(unreal.Rotator(0, 180, 0), False)

les.save_current_level()
unreal.log("build_store_map: done")
