"""Imports Art/Audio/shop_bell.wav as /Game/TSS/Audio/S_ShopBell (the sound the shop door bell plays).

Run: UnrealEditor-Cmd.exe TechStoreSimulator.uproject -ExecutePythonScript="Scripts/import_bell.py" -unattended -nosplash -nullrhi
"""
import os
import unreal

project = unreal.SystemLibrary.get_project_directory()
wav = os.path.normpath(os.path.join(project, "Art", "Audio", "shop_bell.wav"))
task = unreal.AssetImportTask()
task.set_editor_property("filename", wav)
task.set_editor_property("destination_path", "/Game/TSS/Audio")
task.set_editor_property("destination_name", "S_ShopBell")
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
exists = unreal.EditorAssetLibrary.does_asset_exist("/Game/TSS/Audio/S_ShopBell")
unreal.log("import_bell: %s (asset exists: %s)" % (wav, exists))
