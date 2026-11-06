"""Places the generated street scene in Store_Main WITHOUT touching anything else: buildings across the street and two
neighbours beside the shop, and refreshes the road (crosswalk + street lamps). Idempotent: replaces its own buildings.

Run: UnrealEditor-Cmd.exe TechStoreSimulator.uproject /Game/Maps/Store_Main -ExecutePythonScript="Scripts/place_street.py" -unattended -nosplash -nullrhi

Layout (cm): the shop front is x = 700 (facing +X); the road centre line is x = 1500 (runs along Y); the far kerb and
sidewalk end at x = 2280. Buildings across the street have their front at x = 2300 and face the shop (yaw 180);
the two neighbours stand in the shop's row (front at x = 700, yaw 0), clear of the shop's expansion stages.
"""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

for a in list(eas.get_all_level_actors()):
    if a.get_class().get_name() == "TSSStreetBuilding":
        eas.destroy_actor(a)

# (x, y, yaw, width, depth, height, style, label)
BUILDINGS = [
    (2300, -2100, 180, 760, 600, 900, 0, "Across_0"),
    (2300, -1200, 180, 820, 600, 1200, 2, "Across_1"),
    (2300, -250, 180, 900, 600, 700, 1, "Across_2"),
    (2300, 800, 180, 800, 600, 1500, 3, "Across_3"),
    (2300, 1750, 180, 760, 600, 900, 4, "Across_4"),
    (700, -1100, 0, 760, 600, 800, 5, "Neighbour_L"),
    (700, 1100, 0, 760, 600, 1000, 6, "Neighbour_R"),
]
for x, y, yaw, w, d, h, style, label in BUILDINGS:
    b = eas.spawn_actor_from_class(unreal.TSSStreetBuilding, unreal.Vector(x, y, 0), unreal.Rotator(0, 0, yaw))
    b.set_actor_label(label)
    b.set_editor_property("width", w)
    b.set_editor_property("depth", d)
    b.set_editor_property("height", h)
    b.set_editor_property("style", style)
    b.rebuild()

for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "TSSRoad":
        a.rebuild()

les.save_current_level()
unreal.log("place_street: placed %d buildings and refreshed the road" % len(BUILDINGS))
