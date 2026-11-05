"""(Re)places the Register without touching anything else in the map: deletes the old one and spawns a fresh one.
The customer side is the register's local -X, so a register against the back wall needs yaw 180
(customers queue in the room, the owner stands between the counter and the wall).
Entrance points (spawn/door/hub) keep their class defaults unless you change them below.
"""
import unreal
YAW = 180.0
LOC = unreal.Vector(-530, 370, 0)   # hand-placed spot (back-right corner); the origin is on the floor
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
for a in list(eas.get_all_level_actors()):
    if isinstance(a, unreal.TSSRegister):
        eas.destroy_actor(a)
r = eas.spawn_actor_from_class(unreal.TSSRegister, LOC, unreal.Rotator(0, 0, YAW))
r.set_actor_label("Register")
les.save_current_level()
unreal.log("place_register: spawned at %s yaw %.0f" % (r.get_actor_location(), YAW))
