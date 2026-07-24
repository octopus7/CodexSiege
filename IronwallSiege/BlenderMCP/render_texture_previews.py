from pathlib import Path

import bpy
from mathutils import Vector


OUTPUT_DIR = Path(bpy.path.abspath("//")).parent / "Previews"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def ensure_camera():
    camera_data = bpy.data.cameras.new("CodexPreviewCamera")
    camera = bpy.data.objects.new("CodexPreviewCamera", camera_data)
    bpy.context.scene.collection.objects.link(camera)
    camera_data.type = "ORTHO"
    bpy.context.scene.camera = camera
    return camera


def add_area_light(name, energy, size):
    light_data = bpy.data.lights.new(name, "AREA")
    light_data.energy = energy
    light_data.shape = "DISK"
    light_data.size = size
    light = bpy.data.objects.new(name, light_data)
    bpy.context.scene.collection.objects.link(light)
    return light


def visible_bounds(objects):
    points = []
    for obj in objects:
        points.extend(obj.matrix_world @ Vector(corner) for corner in obj.bound_box)
    minimum = Vector((min(p[i] for p in points) for i in range(3)))
    maximum = Vector((max(p[i] for p in points) for i in range(3)))
    return minimum, maximum


scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 768
scene.render.resolution_y = 768
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.film_transparent = False
scene.render.image_settings.color_mode = "RGBA"
scene.view_settings.look = "AgX - Medium High Contrast"
scene.view_settings.exposure = 1.0
scene.world.use_nodes = True
background = scene.world.node_tree.nodes.get("Background")
background.inputs["Color"].default_value = (0.035, 0.045, 0.06, 1.0)
background.inputs["Strength"].default_value = 0.28

camera = ensure_camera()
key = add_area_light("CodexKey", 1350.0, 8.0)
fill = add_area_light("CodexFill", 650.0, 6.0)

all_meshes = [obj for obj in bpy.data.objects if obj.type == "MESH"]
preview_groups = {
    "fortress_gatehouse_textured.png": "SM_Fortress_Gatehouse_A",
    "siege_trebuchet_textured.png": "SM_Siege_Trebuchet_A",
    "infantry_textured.png": "SM_Unit_Infantry_A",
}

for filename, prefix in preview_groups.items():
    render_objects = [
        obj
        for obj in all_meshes
        if obj.name.startswith(prefix) and not obj.name.startswith("UCX_")
    ]
    for obj in all_meshes:
        obj.hide_render = obj not in render_objects

    minimum, maximum = visible_bounds(render_objects)
    center = (minimum + maximum) * 0.5
    size = maximum - minimum
    extent = max(size.x, size.y, size.z)

    camera.location = center + Vector((1.25, -1.55, 1.0)).normalized() * extent * 2.2
    camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.ortho_scale = extent * 1.48

    key.location = center + Vector((-1.2, -1.5, 2.3)).normalized() * extent * 2.0
    key.rotation_euler = (center - key.location).to_track_quat("-Z", "Y").to_euler()
    key.data.size = extent * 0.9

    fill.location = center + Vector((1.8, 0.8, 1.2)).normalized() * extent * 1.7
    fill.rotation_euler = (center - fill.location).to_track_quat("-Z", "Y").to_euler()
    fill.data.size = extent * 0.7

    scene.render.filepath = str(OUTPUT_DIR / filename)
    bpy.ops.render.render(write_still=True)
    print(f"CODEX_PREVIEW={scene.render.filepath}")
