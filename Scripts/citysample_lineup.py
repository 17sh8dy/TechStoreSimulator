"""Lays CH/NYA low-rise Entrance modules out on a grid and photographs them (CitySample_Work, throwaway level)."""
import unreal, re, math, os
unreal.EditorPythonScripting.set_keep_python_script_alive(True)

OUT_TAG = os.environ.get("LINEUP_TAG", "wallA")
WANT = set("SM_BLDG_CHB_L02_A_Wall_08_N1 SM_BLDG_CHD_L2_A_Wall_01_N1 SM_BLDG_CHD_L2_A_Wall_02_N1 SM_BLDG_CHE_L2_A_Wall_03_N1 SM_BLDG_CHE_L2_A_Wall_04_N1 SM_BLDG_CHE_L2_B_Wall_02_N1 SM_BLDG_CHE_L2_B_Wall_06_N1 SM_BLDG_CHE_L3_A_Wall_03_N1 SM_BLDG_CHE_L3_A_Wall_04_N1 SM_BLDG_CHE_L3_B_Wall_06_N1 SM_BLDG_CHE_L4_A_Wall_04_N1 SM_BLDG_CHE_L4_B_Wall_06_N1 SM_BLDG_CHB_L03_A_Wall_08_N1 SM_BLDG_CHB_L03_A_Wall_06_N1".split())
PER = 8
COLS = 4

ar = unreal.AssetRegistryHelpers.get_asset_registry()
assets = [a for a in ar.get_assets_by_path("/Game/Building", recursive=True)] + \
         [a for a in ar.get_assets_by_path("/CitySamplePCG", recursive=True)]
names = {}
for a in assets:
    n = str(a.asset_name)
    if n in WANT:
        names[n] = a.get_asset()
keys = sorted(names)
unreal.log("lineup: %d entrance meshes" % len(keys))

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.new_level("/Game/Temp/EntranceLineup")

def v(x, y, z): return unreal.Vector(x, y, z)

sun = eas.spawn_actor_from_class(unreal.DirectionalLight, v(0, 0, 1000), unreal.Rotator(0, -45, -50))
sun.get_component_by_class(unreal.DirectionalLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
sky = eas.spawn_actor_from_class(unreal.SkyLight, v(0, 0, 1000))
sky.get_component_by_class(unreal.SkyLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
fl = eas.spawn_actor_from_class(unreal.StaticMeshActor, v(0, 0, -5))
fl.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Plane"))
fl.set_actor_scale3d(v(200, 200, 1))

batches = [keys[i:i + PER] for i in range(0, len(keys), PER)]
state = {"b": 0, "tick": 0, "spawned": [], "phase": "build"}

def clear():
    for a in state["spawned"]:
        eas.destroy_actor(a)
    state["spawned"] = []

def build(batch):
    clear()
    ext = 0
    items = []
    for n in batch:
        m = names[n]
        b = m.get_bounds()
        e = max(b.box_extent.x, b.box_extent.y)
        ext = max(ext, e)
        items.append((n, m, b))
    step = 2 * ext + 150
    for i, (n, m, b) in enumerate(items):
        r, c = divmod(i, COLS)
        x, y = c * step, r * step
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, v(x - b.origin.x, y - b.origin.y, -b.origin.z + b.box_extent.z))
        a.static_mesh_component.set_static_mesh(m)
        a.set_actor_label(n)
        state["spawned"].append(a)
        t = eas.spawn_actor_from_class(unreal.TextRenderActor, v(x, y, 2 * b.box_extent.z + 110), unreal.Rotator(0, 235, 0))
        tc = t.get_component_by_class(unreal.TextRenderComponent)
        tc.set_text(n.replace("SM_BLDG_", "").replace("_N1", ""))
        tc.set_world_size(max(70.0, ext * 0.30))
        tc.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
        state["spawned"].append(t)
    cols = min(COLS, len(items))
    rows = math.ceil(len(items) / COLS)
    cx, cy = (cols - 1) * step / 2, (rows - 1) * step / 2
    dist = max(cols, rows) * step * 0.62 + 150
    cam = v(cx - dist * 0.55, cy - dist * 0.75, dist * 0.42)
    look = unreal.MathLibrary.find_look_at_rotation(cam, v(cx, cy, ext * 0.6))
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(cam, look)

def tick(dt):
    try:
        _tick(dt)
    except Exception as ex:
        unreal.log_error("lineup: EXC %r" % (ex,))
        state["b"] = 10 ** 6


def _tick(dt):
    state["tick"] += 1
    t = state["tick"]
    if state["b"] >= len(batches):
        if t > state.get("done_at", 10 ** 9) + 20:
            if not state.get("said"):
                state["said"] = True
                unreal.log("lineup: DONE")
        state.setdefault("done_at", t)
        return
    if state["phase"] == "build":
        unreal.log("lineup: build batch %d" % state["b"])
        build(batches[state["b"]])
        state["phase"], state["t0"] = "wait", t
    elif state["phase"] == "wait" and t - state["t0"] > 240:
        unreal.SystemLibrary.execute_console_command(None, "HighResShot 2400x1350 filename=%s_%02d" % (OUT_TAG, state["b"]))
        state["phase"], state["t0"] = "shot", t
        unreal.log("lineup: shot %d of %d" % (state["b"] + 1, len(batches)))
    elif state["phase"] == "shot" and t - state["t0"] > 90:
        state["b"] += 1
        state["phase"] = "build"

unreal.register_slate_post_tick_callback(tick)
