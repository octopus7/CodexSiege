"""Scene validator and controlled FBX exporter for the Ironwall Siege Blender kit.

Review this file before sending it through Blender MCP's execute_blender_code tool.
It uses bpy and ordinary file output only. It performs no network or subprocess operations.
"""

from __future__ import annotations

import json
import math
from pathlib import Path

import bpy
from mathutils import Vector


ASSETS = {
    "SM_Ground_SiegeField": {
        "dimensions": (90.0, 84.0, 0.5),
        "file": "SM_Ground_SiegeField.fbx",
    },
    "SM_Fortress_Wall_A": {
        "dimensions": (6.0, 1.6, 6.9),
        "file": "SM_Fortress_Wall_A.fbx",
    },
    "SM_Fortress_Gatehouse_A": {
        "dimensions": (6.8, 2.0, 7.6),
        "file": "SM_Fortress_Gatehouse_A.fbx",
    },
    "SM_Fortress_Tower_A": {
        "dimensions": (4.9, 4.9, 8.8),
        "file": "SM_Fortress_Tower_A.fbx",
    },
    "SM_Siege_Trebuchet_A": {
        "dimensions": (10.4, 3.4, 8.4),
        "file": "SM_Siege_Trebuchet_A.fbx",
    },
    "SM_Siege_BatteringRam_A": {
        "dimensions": (7.0, 3.4, 4.6),
        "file": "SM_Siege_BatteringRam_A.fbx",
    },
    "SM_Unit_Infantry_A": {
        "dimensions": (0.9, 0.7, 1.85),
        "file": "SM_Unit_Infantry_A.fbx",
    },
}


def _descendants(root: bpy.types.Object) -> list[bpy.types.Object]:
    result: list[bpy.types.Object] = []
    queue = list(root.children)
    while queue:
        child = queue.pop(0)
        result.append(child)
        queue.extend(child.children)
    return result


def _mesh_family(root: bpy.types.Object) -> list[bpy.types.Object]:
    return [obj for obj in [root, *_descendants(root)] if obj.type == "MESH"]


def _collision_for(root_name: str) -> list[bpy.types.Object]:
    prefix = f"UCX_{root_name}_"
    return [obj for obj in bpy.data.objects if obj.name.startswith(prefix)]


def _family_dimensions(root: bpy.types.Object, meshes: list[bpy.types.Object]) -> tuple[float, float, float]:
    """Return the complete render-family bounds in root-local coordinates."""
    inverse_root = root.matrix_world.inverted()
    points = [
        inverse_root @ obj.matrix_world @ Vector(corner)
        for obj in meshes
        for corner in obj.bound_box
    ]
    minimum = Vector((min(point.x for point in points), min(point.y for point in points), min(point.z for point in points)))
    maximum = Vector((max(point.x for point in points), max(point.y for point in points), max(point.z for point in points)))
    dimensions = maximum - minimum
    return tuple(dimensions)


def validate_scene(tolerance: float = 0.05) -> dict:
    report = {"errors": [], "warnings": [], "assets": {}}
    for name, spec in ASSETS.items():
        asset_report = {"errors": [], "warnings": []}
        root = bpy.data.objects.get(name)
        if root is None:
            asset_report["errors"].append("Missing root object.")
            report["errors"].append(f"{name}: missing root object")
            report["assets"][name] = asset_report
            continue

        meshes = _mesh_family(root)
        if not meshes:
            asset_report["errors"].append("Root has no mesh family.")

        for obj in meshes:
            if any(abs(value - 1.0) > 0.001 for value in obj.scale):
                asset_report["errors"].append(f"{obj.name}: scale is not applied ({tuple(obj.scale)}).")
            if any(abs(value) > math.radians(0.05) for value in obj.rotation_euler):
                asset_report["warnings"].append(f"{obj.name}: non-zero object rotation.")
            if obj.data and len(obj.data.uv_layers) == 0:
                asset_report["errors"].append(f"{obj.name}: UV0 is missing.")
            if obj.data and len(obj.data.materials) == 0:
                asset_report["warnings"].append(f"{obj.name}: no material slot.")

        expected = spec["dimensions"]
        actual = _family_dimensions(root, meshes)
        for axis, expected_value, actual_value in zip("XYZ", expected, actual):
            if expected_value > 0 and abs(actual_value - expected_value) / expected_value > tolerance:
                asset_report["warnings"].append(
                    f"Dimension {axis}: expected {expected_value:.3f} m, got {actual_value:.3f} m."
                )

        if abs(root.location.z) > 0.01:
            asset_report["warnings"].append(
                f"Root origin Z is {root.location.z:.3f}; ground-center convention expects 0."
            )

        collisions = _collision_for(name)
        if not collisions:
            asset_report["warnings"].append("No UCX collision object found.")

        if asset_report["errors"]:
            report["errors"].extend(f"{name}: {item}" for item in asset_report["errors"])
        if asset_report["warnings"]:
            report["warnings"].extend(f"{name}: {item}" for item in asset_report["warnings"])
        asset_report["dimensions_m"] = actual
        asset_report["mesh_objects"] = [obj.name for obj in meshes]
        asset_report["collision_objects"] = [obj.name for obj in collisions]
        report["assets"][name] = asset_report

    print(json.dumps(report, indent=2))
    return report


def export_assets(export_root: str) -> list[str]:
    report = validate_scene()
    if report["errors"]:
        raise RuntimeError("Validation contains errors. Fix them before export.")

    destination = Path(export_root).expanduser().resolve()
    destination.mkdir(parents=True, exist_ok=True)
    exported: list[str] = []

    for name, spec in ASSETS.items():
        root = bpy.data.objects.get(name)
        if root is None:
            continue

        export_objects = [root, *_descendants(root), *_collision_for(name)]
        bpy.ops.object.select_all(action="DESELECT")
        for obj in export_objects:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = root

        output = destination / spec["file"]
        bpy.ops.export_scene.fbx(
            filepath=str(output),
            use_selection=True,
            object_types={"MESH", "EMPTY", "ARMATURE"},
            use_mesh_modifiers=True,
            add_leaf_bones=False,
            bake_anim=False,
            apply_unit_scale=True,
            apply_scale_options="FBX_SCALE_UNITS",
            axis_forward="-Y",
            axis_up="Z",
            mesh_smooth_type="FACE",
            use_tspace=True,
            use_custom_props=True,
        )
        exported.append(str(output))

    print(json.dumps({"exported": exported}, indent=2))
    return exported


if __name__ == "__main__":
    validate_scene()
