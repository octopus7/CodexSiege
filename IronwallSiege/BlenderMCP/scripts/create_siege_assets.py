"""Create the Ironwall Siege Blender asset kit.

This script is intended to run inside Blender through Blender MCP. It only uses
Blender's Python API and writes the requested .blend file. FBX validation and
export remain in ue_asset_pipeline.py.
"""

from __future__ import annotations

import json
import math
from pathlib import Path

import bpy
from mathutils import Vector


PROJECT_ROOT = Path(args["project_root"]).resolve()
BLEND_PATH = PROJECT_ROOT / "BlenderMCP" / "Generated" / "IronwallSiege_Assets.blend"


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.materials):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def make_material(
    name: str,
    color: tuple[float, float, float, float],
    roughness: float = 0.75,
    metallic: float = 0.0,
) -> bpy.types.Material:
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.diffuse_color = color
    material.use_nodes = True
    shader = material.node_tree.nodes.get("Principled BSDF")
    if shader:
        shader.inputs["Base Color"].default_value = color
        shader.inputs["Roughness"].default_value = roughness
        shader.inputs["Metallic"].default_value = metallic
    return material


def finish_mesh(
    obj: bpy.types.Object,
    name: str,
    material: bpy.types.Material,
    parent: bpy.types.Object | None = None,
) -> bpy.types.Object:
    obj.name = name
    obj.data.name = f"{name}_Mesh"
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if not obj.data.uv_layers:
        obj.data.uv_layers.new(name="UVMap")
    obj.data.materials.clear()
    obj.data.materials.append(material)
    triangulate = obj.modifiers.new(name="Triangulate", type="TRIANGULATE")
    bpy.ops.object.modifier_apply(modifier=triangulate.name)
    if parent:
        obj.parent = parent
    obj.select_set(False)
    return obj


def cube(
    name: str,
    dimensions: tuple[float, float, float],
    location: tuple[float, float, float],
    material: bpy.types.Material,
    parent: bpy.types.Object | None = None,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.dimensions = dimensions
    return finish_mesh(obj, name, material, parent)


def cylinder(
    name: str,
    radius: float,
    depth: float,
    location: tuple[float, float, float],
    material: bpy.types.Material,
    parent: bpy.types.Object | None = None,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
    vertices: int = 16,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices,
        radius=radius,
        depth=depth,
        location=location,
        rotation=rotation,
    )
    return finish_mesh(bpy.context.object, name, material, parent)


def sphere(
    name: str,
    radius: float,
    location: tuple[float, float, float],
    material: bpy.types.Material,
    parent: bpy.types.Object | None = None,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=radius, location=location)
    return finish_mesh(bpy.context.object, name, material, parent)


def cone(
    name: str,
    radius1: float,
    radius2: float,
    depth: float,
    location: tuple[float, float, float],
    material: bpy.types.Material,
    parent: bpy.types.Object | None = None,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
    vertices: int = 12,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cone_add(
        vertices=vertices,
        radius1=radius1,
        radius2=radius2,
        depth=depth,
        location=location,
        rotation=rotation,
    )
    return finish_mesh(bpy.context.object, name, material, parent)


def beam(
    name: str,
    start: tuple[float, float, float],
    end: tuple[float, float, float],
    thickness: float,
    material: bpy.types.Material,
    parent: bpy.types.Object,
) -> bpy.types.Object:
    start_v = Vector(start)
    end_v = Vector(end)
    direction = end_v - start_v
    midpoint = (start_v + end_v) * 0.5
    rotation = direction.to_track_quat("X", "Z").to_euler()
    return cube(
        name,
        (direction.length, thickness, thickness),
        tuple(midpoint),
        material,
        parent,
        tuple(rotation),
    )


def collision_box(
    root_name: str,
    dimensions: tuple[float, float, float],
    center_z: float,
    material: bpy.types.Material,
) -> bpy.types.Object:
    obj = cube(
        f"UCX_{root_name}_00",
        dimensions,
        (0.0, 0.0, center_z),
        material,
    )
    obj.display_type = "WIRE"
    obj.hide_render = True
    return obj


def add_crenellations(
    root: bpy.types.Object,
    prefix: str,
    span: float,
    y: float,
    z: float,
    count: int,
    block_size: tuple[float, float, float],
    material: bpy.types.Material,
) -> None:
    for index in range(count):
        x = -span * 0.5 + span * index / max(count - 1, 1)
        cube(
            f"{prefix}_Crenel_{index:02d}",
            block_size,
            (x, y, z),
            material,
            root,
        )


def build_ground(materials: dict[str, bpy.types.Material]) -> None:
    name = "SM_Ground_SiegeField"
    root = cube(name, (90.0, 84.0, 0.5), (0.0, 0.0, 0.25), materials["ground"])
    collision_box(name, (90.0, 84.0, 0.5), 0.25, materials["collision"])
    root["asset_slot"] = "Ground"


def build_wall(materials: dict[str, bpy.types.Material]) -> None:
    name = "SM_Fortress_Wall_A"
    root = cube(name, (6.0, 1.6, 6.0), (0.0, 0.0, 3.0), materials["stone"])
    add_crenellations(root, name, 5.4, 0.0, 6.55, 7, (0.55, 1.6, 0.7), materials["stone_dark"])
    collision_box(name, (6.0, 1.6, 6.9), 3.45, materials["collision"])
    root["asset_slot"] = "Wall"


def build_gatehouse(materials: dict[str, bpy.types.Material]) -> None:
    name = "SM_Fortress_Gatehouse_A"
    root = cube(name, (2.0, 2.0, 6.6), (-2.4, 0.0, 3.3), materials["stone"])
    cube(f"{name}_RightTower", (2.0, 2.0, 6.6), (2.4, 0.0, 3.3), materials["stone"], root)
    cube(f"{name}_Lintel", (3.2, 2.0, 1.4), (0.0, 0.0, 5.9), materials["stone_dark"], root)
    cube(f"{name}_GateLeaf_L", (1.45, 0.18, 4.7), (-0.75, -0.92, 2.35), materials["wood"], root)
    cube(f"{name}_GateLeaf_R", (1.45, 0.18, 4.7), (0.75, -0.92, 2.35), materials["wood"], root)
    add_crenellations(root, name, 6.2, 0.0, 7.25, 8, (0.5, 2.0, 0.7), materials["stone_dark"])
    collision_box(name, (6.8, 2.0, 7.6), 3.8, materials["collision"])
    root["asset_slot"] = "Gate"


def build_tower(materials: dict[str, bpy.types.Material]) -> None:
    name = "SM_Fortress_Tower_A"
    root = cylinder(name, 2.45, 7.8, (0.0, 0.0, 3.9), materials["stone"], vertices=20)
    for index in range(12):
        angle = math.tau * index / 12
        cylinder(
            f"{name}_Crenel_{index:02d}",
            0.3,
            0.75,
            (2.08 * math.cos(angle), 2.08 * math.sin(angle), 8.425),
            materials["stone_dark"],
            root,
            vertices=8,
        )
    collision_box(name, (4.9, 4.9, 8.8), 4.4, materials["collision"])
    root["asset_slot"] = "Tower"


def build_trebuchet(materials: dict[str, bpy.types.Material]) -> None:
    name = "SM_Siege_Trebuchet_A"
    root = beam(name, (-3.6, -1.4, 0.45), (3.6, -1.4, 0.45), 0.32, materials["wood"], None)
    root.parent = None
    beam(f"{name}_Base_R", (-3.6, 1.4, 0.45), (3.6, 1.4, 0.45), 0.32, materials["wood"], root)
    for y in (-1.25, 1.25):
        beam(f"{name}_Frame_L_{y}", (-2.4, y, 0.55), (0.0, y, 5.8), 0.28, materials["wood"], root)
        beam(f"{name}_Frame_R_{y}", (2.4, y, 0.55), (0.0, y, 5.8), 0.28, materials["wood"], root)
    # The throwing arm, counterweight, and stone are separate runtime
    # components in Unreal so they can animate and launch independently.
    for x in (-2.5, 2.5):
        for y in (-1.55, 1.55):
            cylinder(
                f"{name}_Wheel_{x}_{y}",
                0.75,
                0.3,
                (x, y, 0.75),
                materials["wood_dark"],
                root,
                (math.pi * 0.5, 0.0, 0.0),
                14,
            )
    collision_box(name, (10.4, 3.4, 8.4), 4.2, materials["collision"])
    root["asset_slot"] = "Trebuchet"


def build_battering_ram(materials: dict[str, bpy.types.Material]) -> None:
    name = "SM_Siege_BatteringRam_A"
    root = beam(name, (-3.2, -1.45, 0.45), (3.2, -1.45, 0.45), 0.3, materials["wood"], None)
    root.parent = None
    beam(f"{name}_Base_R", (-3.2, 1.45, 0.45), (3.2, 1.45, 0.45), 0.3, materials["wood"], root)
    for x in (-2.4, 2.4):
        for y in (-1.25, 1.25):
            beam(f"{name}_Post_{x}_{y}", (x, y, 0.6), (x * 0.75, y * 0.75, 3.8), 0.25, materials["wood"], root)
    cube(f"{name}_Roof", (6.0, 3.3, 0.3), (0.0, 0.0, 4.0), materials["wood_dark"], root, (0.0, 0.12, 0.0))
    cylinder(
        f"{name}_RamLog",
        0.45,
        6.4,
        (0.0, 0.0, 2.05),
        materials["wood_dark"],
        root,
        (0.0, math.pi * 0.5, 0.0),
        14,
    )
    cone(
        f"{name}_IronTip",
        0.55,
        0.0,
        0.9,
        (3.55, 0.0, 2.05),
        materials["iron"],
        root,
        (0.0, math.pi * 0.5, 0.0),
    )
    for x in (-2.35, 2.35):
        for y in (-1.55, 1.55):
            cylinder(
                f"{name}_Wheel_{x}_{y}",
                0.65,
                0.28,
                (x, y, 0.65),
                materials["wood"],
                root,
                (math.pi * 0.5, 0.0, 0.0),
                14,
            )
    collision_box(name, (7.0, 3.4, 4.6), 2.3, materials["collision"])
    root["asset_slot"] = "BatteringRam"


def build_infantry(materials: dict[str, bpy.types.Material]) -> None:
    name = "SM_Unit_Infantry_A"
    root = cube(name, (0.48, 0.34, 0.72), (0.0, 0.0, 1.02), materials["cloth"])
    sphere(f"{name}_Head", 0.2, (0.0, 0.0, 1.58), materials["skin"], root)
    cone(f"{name}_Helmet", 0.23, 0.05, 0.28, (0.0, 0.0, 1.78), materials["iron"], root)
    for x in (-0.14, 0.14):
        cube(f"{name}_Leg_{x}", (0.16, 0.18, 0.62), (x, 0.0, 0.31), materials["leather"], root)
    cube(f"{name}_Shield", (0.08, 0.68, 0.9), (-0.42, 0.0, 1.05), materials["wood_dark"], root)
    cylinder(
        f"{name}_Spear",
        0.025,
        1.55,
        (0.38, 0.0, 0.95),
        materials["wood"],
        root,
        vertices=8,
    )
    cone(f"{name}_SpearTip", 0.07, 0.0, 0.22, (0.38, 0.0, 1.82), materials["iron"], root)
    collision_box(name, (0.9, 0.7, 1.85), 0.925, materials["collision"])
    root["asset_slot"] = "Infantry"


def main() -> dict:
    clear_scene()
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.length_unit = "METERS"
    scene.unit_settings.scale_length = 1.0

    materials = {
        "stone": make_material("M_Stone", (0.34, 0.36, 0.39, 1.0), 0.9),
        "stone_dark": make_material("M_Stone_Dark", (0.22, 0.24, 0.27, 1.0), 0.95),
        "wood": make_material("M_Wood", (0.24, 0.10, 0.035, 1.0), 0.78),
        "wood_dark": make_material("M_Wood_Dark", (0.12, 0.045, 0.015, 1.0), 0.85),
        "iron": make_material("M_Iron", (0.10, 0.11, 0.13, 1.0), 0.36, 0.72),
        "cloth": make_material("M_Cloth", (0.16, 0.20, 0.30, 1.0), 0.95),
        "leather": make_material("M_Leather", (0.18, 0.07, 0.025, 1.0), 0.88),
        "skin": make_material("M_Skin", (0.43, 0.29, 0.20, 1.0), 0.82),
        "ground": make_material("M_Ground", (0.10, 0.075, 0.04, 1.0), 1.0),
        "collision": make_material("M_Collision", (0.8, 0.05, 0.02, 0.2), 1.0),
    }

    build_ground(materials)
    build_wall(materials)
    build_gatehouse(materials)
    build_tower(materials)
    build_trebuchet(materials)
    build_battering_ram(materials)
    build_infantry(materials)

    BLEND_PATH.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))

    roots = sorted(
        obj.name
        for obj in bpy.data.objects
        if obj.get("asset_slot")
    )
    return {
        "blend_file": str(BLEND_PATH),
        "blender_version": bpy.app.version_string,
        "root_assets": roots,
        "mesh_object_count": sum(1 for obj in bpy.data.objects if obj.type == "MESH"),
    }


__result__ = main()
print(json.dumps(__result__, indent=2))
