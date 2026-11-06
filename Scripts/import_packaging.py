"""Imports the generated packaging PNGs (Art/Packaging) and builds the shared packaging material.

Run (after Scripts/make_packaging.py):
  UnrealEditor-Cmd.exe TechStoreSimulator.uproject -ExecutePythonScript="Scripts/import_packaging.py" -unattended -nosplash -nullrhi

Result: /Game/TSS/Textures/T_Pack_<Name> and /Game/TSS/Materials/M_Packaging (texture parameter "Pack").
Safe to re-run: textures are replaced, the material is rebuilt.
"""
import os
import unreal

ART = os.path.abspath(os.path.join(unreal.Paths.project_dir(), "Art", "Packaging"))
TEX_DIR = "/Game/TSS/Textures"
MAT_DIR = "/Game/TSS/Materials"

tools = unreal.AssetToolsHelpers.get_asset_tools()
unreal.EditorAssetLibrary.make_directory(TEX_DIR)
unreal.EditorAssetLibrary.make_directory(MAT_DIR)

# ---- textures ----------------------------------------------------------------------------------------
tasks = []
for f in sorted(os.listdir(ART)):
    if f.startswith("T_Pack_") and f.endswith(".png"):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(ART, f)
        t.destination_path = TEX_DIR
        t.automated = True
        t.replace_existing = True
        t.replace_existing_settings = True
        t.save = True
        tasks.append(t)
tools.import_asset_tasks(tasks)
imported = []
for t in tasks:
    for path in t.get_editor_property("imported_object_paths"):
        tex = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(tex, unreal.Texture2D):
            tex.set_editor_property("srgb", True)
            tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
            unreal.EditorAssetLibrary.save_loaded_asset(tex)
            imported.append(path)
unreal.log("import_packaging: %d textures imported" % len(imported))

# ---- the material ------------------------------------------------------------------------------------
mat_path = MAT_DIR + "/M_Packaging"
if unreal.EditorAssetLibrary.does_asset_exist(mat_path):
    unreal.EditorAssetLibrary.delete_asset(mat_path)
mat = tools.create_asset("M_Packaging", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
mel = unreal.MaterialEditingLibrary

tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0)
tex.set_editor_property("parameter_name", "Pack")
if imported:
    tex.set_editor_property("texture", unreal.EditorAssetLibrary.load_asset(imported[0]))
mel.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
rough.set_editor_property("r", 0.55)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log("import_packaging: material %s built" % mat_path)
