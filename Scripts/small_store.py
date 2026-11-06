"""Turns Store_Main into the small starting shop and puts the StoreShell (walls, floor, ceiling, lights) in charge.

Run: UnrealEditor-Cmd.exe TechStoreSimulator.uproject /Game/Maps/Store_Main -ExecutePythonScript="Scripts/small_store.py" -unattended -nosplash -nullrhi

Replaces the hand-placed walls/floor/ceiling/lights, shelves, crates, register, computer and dock with the stage-0 layout
(front wall x=700 with the door; shop is 7 x 6 m). The outdoor ground, sky, sun and post-process volume are kept.
Everything that lives against a wall carries an anchor tag (TSSAnchorBack/Left/Right) so StoreDesk expansions slide it.
Stage-0 coordinates (cm): back wall x=0, side walls y=+-300, front wall x=700, door gap y=+-100.
"""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def v(x, y, z=0.0):
    return unreal.Vector(x, y, z)

# ---- clear what the shell and the new layout replace ------------------------------------------------------
STRUCTURE = ("Wall_", "Floor", "Ceiling", "LightPanel_")
for a in list(eas.get_all_level_actors()):
    label = a.get_actor_label()
    if isinstance(a, unreal.TSSBlock) and label.startswith(STRUCTURE):
        eas.destroy_actor(a)
    elif isinstance(a, unreal.PointLight) and label.startswith("StoreLight_"):
        eas.destroy_actor(a)
    elif isinstance(a, (unreal.TSSShelf, unreal.TSSCrate, unreal.TSSRegister, unreal.TSSStoreDeskComputer,
                        unreal.TSSDeliveryDock, unreal.TSSStoreShell)):
        eas.destroy_actor(a)

def tag(actor, name):
    actor.set_editor_property("tags", [unreal.Name(name)])

# ---- the building -----------------------------------------------------------------------------------------
shell = eas.spawn_actor_from_class(unreal.TSSStoreShell, v(0, 0, 0))
shell.set_actor_label("StoreShell")

# ---- two starting shelves along the left (-Y) wall, facing into the shop ----------------------------------
for i, x in enumerate((150, 365)):
    s = eas.spawn_actor_from_class(unreal.TSSShelf, v(x, -274), unreal.Rotator(0, 0, 90))
    s.set_actor_label("Shelf_Start_%d" % i)
    tag(s, "TSSAnchorLeft")

# ---- wholesale crates against the back wall (a 2 x 3 block) ------------------------------------------------
products = [unreal.TSSProduct.PHONE, unreal.TSSProduct.LAPTOP, unreal.TSSProduct.HEADPHONES,
            unreal.TSSProduct.CONSOLE, unreal.TSSProduct.MONITOR, unreal.TSSProduct.KEYBOARD]
for i, p in enumerate(products):
    x = 45 + 100 * (i // 3)
    y = -15 + 95 * (i % 3)
    c = eas.spawn_actor_from_class(unreal.TSSCrate, v(x, y, 30), unreal.Rotator(0, 0, 0))
    c.set_editor_property("product", p)
    c.set_actor_label("Crate_%d" % i)
    c.rebuild()
    tag(c, "TSSAnchorBack")

# ---- checkout counter on the right (+Y) wall by the door; the owner stands between the counter and the wall ----
reg = eas.spawn_actor_from_class(unreal.TSSRegister, v(480, 170), unreal.Rotator(0, 0, 90))
reg.set_actor_label("Register")
reg.set_editor_property("hub_point", v(360, 0))
tag(reg, "TSSAnchorRight")

# ---- StoreDesk computer in the back-left corner --------------------------------------------------------------
desk = eas.spawn_actor_from_class(unreal.TSSStoreDeskComputer, v(40, -170), unreal.Rotator(0, 0, 0))
desk.set_actor_label("StoreDeskComputer")
tag(desk, "TSSAnchorBack")

# ---- delivery dock just inside the door, on the left -------------------------------------------------------------
dock = eas.spawn_actor_from_class(unreal.TSSDeliveryDock, v(600, -210), unreal.Rotator(0, 0, 0))
dock.set_actor_label("DeliveryDock")
tag(dock, "TSSAnchorLeft")

# ---- spawn point -------------------------------------------------------------------------------------------------
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.PlayerStart):
        a.set_actor_location(v(560, 0, 100), False, False)
        a.set_actor_rotation(unreal.Rotator(0, 180, 0), False)

les.save_current_level()
unreal.log("small_store: done (stage-0 layout)")
