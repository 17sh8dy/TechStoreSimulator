"""Creates /Game/TSS/Materials/M_TSSEmissive: a simple glowing material with a "Color" vector parameter and a "Glow"
strength parameter. Used for lit windows at night. Idempotent (recreates the asset).

Run: UnrealEditor-Cmd.exe TechStoreSimulator.uproject -ExecutePythonScript="Scripts/make_emissive_material.py" -unattended -nosplash -nullrhi
"""
import unreal

PATH = "/Game/TSS/Materials"
NAME = "M_TSSEmissive"
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.MaterialEditingLibrary

if unreal.EditorAssetLibrary.does_asset_exist(PATH + "/" + NAME):
    unreal.EditorAssetLibrary.delete_asset(PATH + "/" + NAME)

mat = tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())

color = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, 0)
color.set_editor_property("parameter_name", "Color")
color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.8, 0.45, 1.0))

glow = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 220)
glow.set_editor_property("parameter_name", "Glow")
glow.set_editor_property("default_value", 6.0)

mul = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -220, 80)
lib.connect_material_expressions(color, "", mul, "A")
lib.connect_material_expressions(glow, "", mul, "B")
lib.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(PATH + "/" + NAME)
unreal.log("make_emissive_material: created %s/%s" % (PATH, NAME))
