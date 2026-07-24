"""Import the Blender MCP FBX kit into the expected Unreal Engine paths."""

from __future__ import annotations

import os
from pathlib import Path

import unreal


ASSET_NAMES = (
    "SM_Ground_SiegeField",
    "SM_Fortress_Wall_A",
    "SM_Fortress_Gatehouse_A",
    "SM_Fortress_Tower_A",
    "SM_Siege_Trebuchet_A",
    "SM_Siege_BatteringRam_A",
    "SM_Unit_Infantry_A",
)
DESTINATION = "/Game/Art/Blender"


def make_import_task(source: Path) -> unreal.AssetImportTask:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)

    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("combine_meshes", True)
    static_data.set_editor_property("generate_lightmap_u_vs", True)
    static_data.set_editor_property("auto_generate_collision", False)
    static_data.set_editor_property("import_mesh_lods", False)
    static_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", source.stem)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    return task


def main() -> None:
    project_root = Path(unreal.Paths.project_dir()).resolve()
    export_root = project_root / "BlenderMCP" / "Exports"
    sources = [export_root / f"{name}.fbx" for name in ASSET_NAMES]
    missing_sources = [str(path) for path in sources if not path.is_file()]
    if missing_sources:
        raise RuntimeError(f"Missing Blender exports: {missing_sources}")

    tasks = [make_import_task(path) for path in sources]
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    missing_assets: list[str] = []
    for name, task in zip(ASSET_NAMES, tasks):
        object_path = f"{DESTINATION}/{name}.{name}"
        asset = unreal.load_asset(object_path)
        if not isinstance(asset, unreal.StaticMesh):
            missing_assets.append(object_path)
            continue
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        unreal.log(
            f"IronwallSiegeImport asset={object_path} "
            f"source={os.path.basename(task.get_editor_property('filename'))}"
        )

    if missing_assets:
        raise RuntimeError(f"Imported Static Mesh assets are missing: {missing_assets}")

    unreal.log(f"IronwallSiegeImport success count={len(ASSET_NAMES)} destination={DESTINATION}")


main()
