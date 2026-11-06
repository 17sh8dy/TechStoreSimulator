"""Adds the day/night actor to Store_Main WITHOUT touching anything else (idempotent).
Run: UnrealEditor-Cmd.exe TechStoreSimulator.uproject /Game/Maps/Store_Main -ExecutePythonScript="Scripts/place_daynight.py" -unattended -nosplash -nullrhi
"""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
for a in list(eas.get_all_level_actors()):
    if a.get_class().get_name() == "TSSDayNight":
        eas.destroy_actor(a)
d = eas.spawn_actor_from_class(unreal.TSSDayNight, unreal.Vector(0, 0, 0))
d.set_actor_label("DayNight")
les.save_current_level()
unreal.log("place_daynight: placed the DayNight actor")
