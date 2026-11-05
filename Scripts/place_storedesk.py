"""(Re)places the StoreDesk computer and the delivery dock without touching anything else in the map.
Run: UnrealEditor-Cmd.exe TechStoreSimulator.uproject /Game/Maps/Store_Main -ExecutePythonScript="Scripts/place_storedesk.py" -unattended -nosplash -nullrhi
The desk stands against the back wall; you use it from the +X side. Drag the actors in the editor to move them:
the computer is one actor (move it freely); the dock mat is where delivery trucks' couriers drop parcels, and
its TruckStart/TruckStop offsets say where the truck comes from and parks.
"""
import unreal

DESK_LOC = unreal.Vector(-660, 70, 0)     # back wall, left of the register corner
DOCK_LOC = unreal.Vector(620, 260, 0)     # just inside the door, on the left

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
for a in list(eas.get_all_level_actors()):
    if isinstance(a, (unreal.TSSStoreDeskComputer, unreal.TSSDeliveryDock)):
        eas.destroy_actor(a)

desk = eas.spawn_actor_from_class(unreal.TSSStoreDeskComputer, DESK_LOC, unreal.Rotator(0, 0, 0))
desk.set_actor_label("StoreDeskComputer")
dock = eas.spawn_actor_from_class(unreal.TSSDeliveryDock, DOCK_LOC, unreal.Rotator(0, 0, 0))
dock.set_actor_label("DeliveryDock")
les.save_current_level()
unreal.log("place_storedesk: desk at %s, dock at %s" % (desk.get_actor_location(), dock.get_actor_location()))
