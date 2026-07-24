"""Reimport only the fixed-frame trebuchet FBX into Unreal Engine."""

from pathlib import Path

import unreal


def main() -> None:
    project_root = Path(unreal.Paths.project_dir()).resolve()
    source = project_root / "BlenderMCP" / "Exports" / "SM_Siege_Trebuchet_A.fbx"
    if not source.is_file():
        raise RuntimeError(f"Missing trebuchet FBX: {source}")

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)

    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("combine_meshes", True)
    static_data.set_editor_property("generate_lightmap_u_vs", True)
    static_data.set_editor_property("auto_generate_collision", False)
    static_data.set_editor_property("import_mesh_lods", False)
    static_data.set_editor_property(
        "normal_import_method",
        unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS,
    )

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", "/Game/Art/Blender")
    task.set_editor_property("destination_name", "SM_Siege_Trebuchet_A")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", False)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset = unreal.load_asset(
        "/Game/Art/Blender/SM_Siege_Trebuchet_A.SM_Siege_Trebuchet_A"
    )
    if not isinstance(asset, unreal.StaticMesh):
        raise RuntimeError("Trebuchet static mesh reimport failed")

    unreal.log(
        "IronwallSiegeImport success asset="
        "/Game/Art/Blender/SM_Siege_Trebuchet_A"
    )


main()
