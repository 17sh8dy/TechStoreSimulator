"""Places the customer spawner just outside the shop door, facing the shop, WITHOUT touching anything else in the map (idempotent).
Also points the register's SpawnPoint at the same spot so shoppers who leave walk back to it.
Run: UnrealEditor-Cmd.exe TechStoreSimulator.uproject /Game/Maps/Store_Main -ExecutePythonScript="Scripts/place_customer_spawner.py" -unattended -nosplash -nullrhi
"""
import unreal

LOC = unreal.Vector(880, 0, 0)   # the door is at x=700 (front wall 700..740); +X is the street side
YAW = 180.0                      # facing -X, toward the shop

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
for a in list(eas.get_all_level_actors()):
    if a.get_class().get_name() == "TSSCustomerSpawner":
        eas.destroy_actor(a)
sp = eas.spawn_actor_from_class(unreal.TSSCustomerSpawner, LOC, unreal.Rotator(0, 0, YAW))
sp.set_actor_label("CustomerSpawner")
sp.set_editor_property("use_own_location", True)
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.TSSRegister):
        a.set_editor_property("spawn_point", LOC)
les.save_current_level()
unreal.log("place_customer_spawner: placed at %s yaw %.0f" % (LOC, YAW))
