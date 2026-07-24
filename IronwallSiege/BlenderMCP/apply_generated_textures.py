from pathlib import Path

import bpy


SOURCE_DIR = Path(bpy.path.abspath("//"))
TEXTURE_DIR = (SOURCE_DIR.parent / "Textures").resolve()
OUTPUT_BLEND = SOURCE_DIR / "IronwallSiege_Assets_Textured.blend"


def clear_nodes(material):
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    output.location = (560, 0)
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    shader.location = (280, 0)
    material.node_tree.links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return nodes, material.node_tree.links, shader


def set_textured_material(
    material_name,
    texture_name,
    *,
    scale=1.0,
    roughness=0.72,
    bump_strength=0.18,
    bump_distance=0.1,
    value=1.0,
    saturation=1.0,
):
    material = bpy.data.materials.get(material_name)
    if material is None:
        return

    nodes, links, shader = clear_nodes(material)
    shader.inputs["Roughness"].default_value = roughness

    texcoord = nodes.new("ShaderNodeTexCoord")
    texcoord.location = (-760, 80)
    mapping = nodes.new("ShaderNodeMapping")
    mapping.location = (-570, 80)
    mapping.inputs["Scale"].default_value = (scale, scale, scale)
    image_node = nodes.new("ShaderNodeTexImage")
    image_node.location = (-350, 80)
    image_path = TEXTURE_DIR / texture_name
    image = bpy.data.images.load(str(image_path), check_existing=True)
    image.filepath = bpy.path.relpath(str(image_path))
    image.colorspace_settings.name = "sRGB"
    image_node.image = image
    image_node.interpolation = "Linear"
    image_node.extension = "REPEAT"

    color_adjust = nodes.new("ShaderNodeHueSaturation")
    color_adjust.location = (-50, 100)
    color_adjust.inputs["Saturation"].default_value = saturation
    color_adjust.inputs["Value"].default_value = value

    bump = nodes.new("ShaderNodeBump")
    bump.location = (40, -150)
    bump.inputs["Strength"].default_value = bump_strength
    bump.inputs["Distance"].default_value = bump_distance

    links.new(texcoord.outputs["UV"], mapping.inputs["Vector"])
    links.new(mapping.outputs["Vector"], image_node.inputs["Vector"])
    links.new(image_node.outputs["Color"], color_adjust.inputs["Color"])
    links.new(color_adjust.outputs["Color"], shader.inputs["Base Color"])
    links.new(image_node.outputs["Color"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], shader.inputs["Normal"])


def set_procedural_material(
    material_name,
    color_a,
    color_b,
    *,
    scale,
    detail,
    roughness,
    metallic=0.0,
    bump_strength=0.12,
    bump_distance=0.04,
):
    material = bpy.data.materials.get(material_name)
    if material is None:
        return

    nodes, links, shader = clear_nodes(material)
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["Metallic"].default_value = metallic

    texcoord = nodes.new("ShaderNodeTexCoord")
    texcoord.location = (-700, 80)
    mapping = nodes.new("ShaderNodeMapping")
    mapping.location = (-520, 80)
    noise = nodes.new("ShaderNodeTexNoise")
    noise.location = (-330, 80)
    noise.noise_dimensions = "3D"
    noise.inputs["Scale"].default_value = scale
    noise.inputs["Detail"].default_value = detail
    noise.inputs["Roughness"].default_value = 0.65

    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.location = (-80, 110)
    ramp.color_ramp.elements[0].color = (*color_a, 1.0)
    ramp.color_ramp.elements[1].color = (*color_b, 1.0)

    bump = nodes.new("ShaderNodeBump")
    bump.location = (30, -150)
    bump.inputs["Strength"].default_value = bump_strength
    bump.inputs["Distance"].default_value = bump_distance

    links.new(texcoord.outputs["Generated"], mapping.inputs["Vector"])
    links.new(mapping.outputs["Vector"], noise.inputs["Vector"])
    links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], shader.inputs["Base Color"])
    links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], shader.inputs["Normal"])


set_textured_material(
    "M_Stone",
    "T_FortressStone_BaseColor.png",
    roughness=0.88,
    bump_strength=0.28,
    bump_distance=0.12,
)
set_textured_material(
    "M_Stone_Dark",
    "T_FortressStone_BaseColor.png",
    roughness=0.92,
    bump_strength=0.32,
    bump_distance=0.13,
    value=0.62,
    saturation=0.72,
)
set_textured_material(
    "M_Wood",
    "T_AgedOak_BaseColor.png",
    roughness=0.72,
    bump_strength=0.22,
    bump_distance=0.08,
)
set_textured_material(
    "M_Wood_Dark",
    "T_AgedOak_BaseColor.png",
    roughness=0.78,
    bump_strength=0.25,
    bump_distance=0.09,
    value=0.68,
    saturation=0.78,
)
set_textured_material(
    "M_Ground",
    "T_SiegeGround_BaseColor.png",
    scale=7.0,
    roughness=0.98,
    bump_strength=0.24,
    bump_distance=0.16,
)
set_textured_material(
    "M_Cloth",
    "T_CrimsonWool_BaseColor.png",
    scale=2.2,
    roughness=0.96,
    bump_strength=0.18,
    bump_distance=0.025,
)

set_procedural_material(
    "M_Iron",
    (0.045, 0.052, 0.060),
    (0.25, 0.28, 0.30),
    scale=9.0,
    detail=5.0,
    roughness=0.48,
    metallic=0.82,
    bump_strength=0.2,
    bump_distance=0.035,
)
set_procedural_material(
    "M_Leather",
    (0.07, 0.025, 0.012),
    (0.24, 0.095, 0.032),
    scale=5.5,
    detail=3.5,
    roughness=0.8,
    bump_strength=0.16,
    bump_distance=0.025,
)
set_procedural_material(
    "M_Skin",
    (0.27, 0.105, 0.055),
    (0.62, 0.31, 0.18),
    scale=7.0,
    detail=2.0,
    roughness=0.62,
    bump_strength=0.04,
    bump_distance=0.01,
)

for obj in bpy.data.objects:
    if obj.name.startswith("UCX_"):
        obj.hide_render = True
        obj.display_type = "WIRE"

bpy.context.scene.render.engine = "BLENDER_EEVEE"
bpy.context.scene.view_settings.look = "AgX - Medium High Contrast"
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND), check_existing=False)
print(f"CODEX_TEXTURED_BLEND={OUTPUT_BLEND}")
