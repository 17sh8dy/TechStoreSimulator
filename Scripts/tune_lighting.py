"""Re-tunes the shop lighting in place (does NOT touch the layout, so hand-edits in Store_Main survive).
Run: UnrealEditor-Cmd.exe TechStoreSimulator.uproject /Game/Maps/Store_Main -ExecutePythonScript="Scripts/tune_lighting.py" -unattended -nosplash -nullrhi
Edit the three numbers below, re-run, look at the screenshots from -TSSAutoTest.
"""
import unreal

LIGHT_LUMENS = 300.0     # per ceiling light (was 1500: blown out)
SKY_INTENSITY = 0.15     # ambient from the sky leaking in through the door
EXPOSURE_BIAS = -0.6     # EV offset on top of the pinned exposure (negative = darker)
EXPOSURE = 1.0           # pinned exposure brightness (the PostProcessVolume stops eye adaptation)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
n = 0
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.PointLight):
        c = a.get_component_by_class(unreal.PointLightComponent)
        c.set_editor_property("intensity", LIGHT_LUMENS)
        n += 1
    elif isinstance(a, unreal.SkyLight):
        a.get_component_by_class(unreal.SkyLightComponent).set_editor_property("intensity", SKY_INTENSITY)
    elif isinstance(a, unreal.PostProcessVolume):
        pp = a.get_editor_property("settings")
        pp.set_editor_property("auto_exposure_min_brightness", EXPOSURE)
        pp.set_editor_property("auto_exposure_max_brightness", EXPOSURE)
        pp.set_editor_property("override_auto_exposure_bias", True)
        pp.set_editor_property("auto_exposure_bias", EXPOSURE_BIAS)
        a.set_editor_property("settings", pp)
les.save_current_level()
unreal.log("tune_lighting: %d lights set to %s lumens" % (n, LIGHT_LUMENS))
