"""Adds the shop door and the delivery road to Store_Main WITHOUT touching anything else, and removes the old wholesale
crates (stock is ordered from the StoreDesk PC now). Idempotent: it replaces its own earlier door/road.

Run: UnrealEditor-Cmd.exe TechStoreSimulator.uproject /Game/Maps/Store_Main -ExecutePythonScript="Scripts/place_door_and_road.py" -unattended -nosplash -nullrhi

Door: in the front-wall doorway (x = 700, y = 0), facing outdoors (+X).
Road: runs along +Y in front of the shop, centre line at x = 1500. Traffic flows +Y, so the truck's right-hand lane
      (200 cm to its right) is x = 1300, on the shop side. The sidewalk runs from the shop wall (x = 720) to the road edge.
      To use a Fab road instead, move the spline points along it and untick "Build Placeholder" on the road actor.
"""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

removed_crates = 0
for a in list(eas.get_all_level_actors()):
    cls = a.get_class().get_name()
    if cls == "TSSCrate":
        eas.destroy_actor(a)
        removed_crates += 1
    elif cls in ("TSSShopDoor", "TSSRoad"):
        eas.destroy_actor(a)

door = eas.spawn_actor_from_class(unreal.TSSShopDoor, unreal.Vector(700, 0, 0), unreal.Rotator(0, 0, 0))
door.set_actor_label("ShopDoor")

road = eas.spawn_actor_from_class(unreal.TSSRoad, unreal.Vector(1500, -3000, 0), unreal.Rotator(0, 0, 90))
road.set_actor_label("DeliveryRoad")
road.rebuild()

les.save_current_level()
unreal.log("place_door_and_road: removed %d crates, placed ShopDoor and DeliveryRoad" % removed_crates)
