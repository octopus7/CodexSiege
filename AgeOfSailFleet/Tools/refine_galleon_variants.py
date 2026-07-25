"""Add a production-detail pass to the Age of Sail Blender ship variants.

Run with Blender in background mode.  The script preserves the original master
scene and writes a new source blend plus FBX files used by the Unreal importer.
The additions are deliberately modular so the ships remain practical real-time
static meshes rather than attempting to reproduce a concept image literally.
"""

from __future__ import annotations

from math import pi, sin
from pathlib import Path

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[1]
SOURCE_BLEND = ROOT / "Art" / "Source" / "AgeOfSailFleetVariants.blend"
OUTPUT_BLEND = ROOT / "Art" / "Source" / "AgeOfSailFleetVariants_GalleonRefined.blend"
RAW = ROOT / "Content" / "Raw"
PREVIEW = ROOT / "Art" / "Preview" / "GalleonRefined_FirstRate.png"


def material(name: str):
    return bpy.data.materials.get(name)


OAK = material("MAT_OakHull")
DECK = material("MAT_WeatheredDeck")
DARK = material("MAT_DarkKeel")
GOLD = material("MAT_FlagshipGold")
IRON = material("MAT_CannonIron")
ROPE = material("MAT_HempRope")
GLASS = material("MAT_GalleryGlass")
VOID = material("MAT_GunportVoid")


def new_collection(name: str):
    collection = bpy.data.collections.get(name)
    if collection is None:
        collection = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(collection)
    return collection


DETAILS = new_collection("Galleon_Refined_Details")


def link_to_details(obj):
    for collection in list(obj.users_collection):
        collection.objects.unlink(obj)
    DETAILS.objects.link(obj)
    return obj


def set_material(obj, mat):
    if mat is not None and hasattr(obj.data, "materials"):
        obj.data.materials.clear()
        obj.data.materials.append(mat)


def cube(name, location, dimensions, mat, bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        modifier = obj.modifiers.new("Softened_Edges", "BEVEL")
        modifier.width = bevel
        modifier.segments = 2
    set_material(obj, mat)
    return link_to_details(obj)


def cylinder(name, location, radius, depth, mat, rotation=(0.0, 0.0, 0.0), vertices=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    set_material(obj, mat)
    bevel = obj.modifiers.new("Edge_Round", "BEVEL")
    bevel.width = min(radius * 0.22, 0.06)
    bevel.segments = 2
    return link_to_details(obj)


def sphere(name, location, scale, mat):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    set_material(obj, mat)
    return link_to_details(obj)


def torus(name, location, major_radius, minor_radius, mat, rotation=(0.0, 0.0, 0.0)):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius,
        minor_radius=minor_radius,
        major_segments=16,
        minor_segments=6,
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    set_material(obj, mat)
    return link_to_details(obj)


def rope_curve(name, points, radius, mat=ROPE, cyclic=False):
    data = bpy.data.curves.new(name, "CURVE")
    data.dimensions = "3D"
    data.resolution_u = 2
    data.bevel_depth = radius
    data.bevel_resolution = 2
    spline = data.splines.new("NURBS")
    spline.points.add(len(points) - 1)
    for point, co in zip(spline.points, points):
        point.co = (co[0], co[1], co[2], 1.0)
    spline.order_u = min(3, len(points))
    spline.use_endpoint_u = True
    spline.use_cyclic_u = cyclic
    obj = bpy.data.objects.new(name, data)
    DETAILS.objects.link(obj)
    set_material(obj, mat)
    return obj


def hull_width(relative_x, half_length, beam):
    """A readable, full-bellied hull profile that still pinches at both ends."""
    t = min(1.0, abs(relative_x) / half_length)
    return max(beam * 0.22, beam * (1.0 - 0.72 * (t ** 1.75)))


def deck_height(relative_x, half_length, base):
    t = min(1.0, abs(relative_x) / half_length)
    return base + 0.62 * (t ** 1.8) + (0.28 if relative_x < -half_length * 0.55 else 0.0)


def side_course(name, cx, half_length, beam, z, offset, mat, radius):
    for side in (-1.0, 1.0):
        points = []
        for index in range(17):
            local_x = -half_length + index * (2.0 * half_length / 16.0)
            width = hull_width(local_x, half_length, beam)
            rise = 0.10 * ((abs(local_x) / half_length) ** 2)
            points.append((cx + local_x, side * (width + offset), z + rise))
        rope_curve(f"{name}_{'Port' if side < 0 else 'Starboard'}", points, radius, mat)


def rail_section(name, start, end, height, mat=GOLD, radius=0.055):
    x0, y0, z0 = start
    x1, y1, z1 = end
    midpoint = ((x0 + x1) / 2.0, (y0 + y1) / 2.0, max(z0, z1) + height)
    rope_curve(name, [start, midpoint, end], radius, mat)


def add_stern_gallery(tag, cx, half_length, beam, base, scale, flagship):
    stern = cx - half_length
    gallery_x = stern - 0.20
    rows = 3 if flagship else 2
    columns = 5 if flagship else 4
    for row in range(rows):
        z = base + 1.9 + row * 0.82 * scale
        width = beam * (0.86 - row * 0.08)
        cube(f"{tag}_GalleryBand_{row}", (gallery_x, 0.0, z + 0.39 * scale), (0.28 * scale, width * 2.05, 0.16 * scale), GOLD, 0.03)
        for column in range(columns):
            y = (column - (columns - 1) / 2.0) * (width * 1.45 / max(1, columns - 1))
            cube(f"{tag}_GalleryWindow_{row}_{column}", (gallery_x - 0.05 * scale, y, z), (0.18 * scale, 0.43 * scale, 0.54 * scale), GLASS, 0.04)
            cube(f"{tag}_GalleryFrameTop_{row}_{column}", (gallery_x - 0.12 * scale, y, z + 0.31 * scale), (0.10 * scale, 0.56 * scale, 0.06 * scale), GOLD, 0.01)
    # A stepped sterncastle is the strongest silhouette cue separating a galleon
    # from a modern box-hull.  It deliberately narrows at every level.
    tiers = 2 if flagship else 1
    for tier in range(tiers):
        tier_scale = 1.0 - tier * 0.18
        cabin_x = stern + (1.35 + tier * 0.22) * scale
        cabin_z = base + (5.05 + tier * 1.15) * scale
        cabin_length = (3.15 - tier * 0.46) * scale
        cabin_width = beam * (1.46 - tier * 0.18)
        cabin_height = 1.02 * scale
        cube(f"{tag}_SterncastleCabin_{tier}", (cabin_x, 0.0, cabin_z), (cabin_length, cabin_width, cabin_height), OAK, 0.12 * scale)
        cube(f"{tag}_SterncastleGoldBand_{tier}", (stern - 0.26 * scale, 0.0, cabin_z + 0.34 * scale), (0.12 * scale, cabin_width * 1.04, 0.12 * scale), GOLD, 0.025 * scale)
        cube(f"{tag}_SterncastleRoof_{tier}", (cabin_x + 0.08 * scale, 0.0, cabin_z + cabin_height * 0.56), (cabin_length * 1.08, cabin_width * 1.08, 0.16 * scale), DARK, 0.08 * scale)
        for side in (-1.0, 1.0):
            for column in range(3 if flagship else 2):
                window_x = cabin_x - cabin_length * 0.22 + column * cabin_length * 0.23
                window_y = side * (cabin_width * 0.5 + 0.025 * scale)
                cube(f"{tag}_SterncastleSideWindow_{tier}_{side}_{column}", (window_x, window_y, cabin_z + 0.03 * scale), (0.52 * scale, 0.07 * scale, 0.50 * scale), GLASS, 0.03 * scale)
    crown_z = base + (7.42 if flagship else 6.2) * scale
    for side in (-1.0, 1.0):
        cylinder(f"{tag}_SterncastleLanternPost_{side}", (stern + 0.05 * scale, side * beam * 0.48, crown_z - 0.22 * scale), 0.055 * scale, 0.50 * scale, GOLD)
        sphere(f"{tag}_SterncastleLantern_{side}", (stern + 0.05 * scale, side * beam * 0.48, crown_z + 0.12 * scale), (0.16 * scale, 0.16 * scale, 0.22 * scale), GLASS)
    for side in (-1.0, 1.0):
        balcony_y = side * (beam * 0.96)
        z = base + 2.15 * scale
        cube(f"{tag}_GalleryBalcony_{side}", (stern + 0.45 * scale, balcony_y, z), (2.3 * scale, 0.58 * scale, 0.16 * scale), DECK, 0.04)
        rail_section(
            f"{tag}_GalleryRail_{side}",
            (stern - 0.45 * scale, balcony_y + side * 0.26 * scale, z + 0.14 * scale),
            (stern + 1.5 * scale, balcony_y + side * 0.26 * scale, z + 0.14 * scale),
            0.35 * scale,
        )
        for lamp_index in range(2):
            lx = stern + (0.2 + lamp_index * 1.0) * scale
            cylinder(f"{tag}_SternLanternPost_{side}_{lamp_index}", (lx, balcony_y + side * 0.18 * scale, z + 0.48 * scale), 0.05 * scale, 0.54 * scale, GOLD)
            sphere(f"{tag}_SternLantern_{side}_{lamp_index}", (lx, balcony_y + side * 0.18 * scale, z + 0.75 * scale), (0.12 * scale, 0.12 * scale, 0.17 * scale), GLASS)


def add_figurehead(tag, cx, half_length, beam, base, scale):
    bow = cx + half_length
    x = bow + 1.25 * scale
    z = base + 1.65 * scale
    cylinder(f"{tag}_BeakheadSpine", (bow + 0.55 * scale, 0.0, base + 2.25 * scale), 0.15 * scale, 3.35 * scale, DECK, rotation=(0.0, pi / 2.0, 0.0))
    cube(f"{tag}_BeakheadPlatform", (bow + 0.86 * scale, 0.0, base + 2.02 * scale), (2.25 * scale, beam * 0.82, 0.16 * scale), DECK, 0.04)
    rail_section(f"{tag}_BeakheadGoldRail_Port", (bow + 0.04 * scale, -beam * 0.46, base + 2.16 * scale), (bow + 1.8 * scale, -beam * 0.36, base + 2.25 * scale), 0.35 * scale)
    rail_section(f"{tag}_BeakheadGoldRail_Starboard", (bow + 0.04 * scale, beam * 0.46, base + 2.16 * scale), (bow + 1.8 * scale, beam * 0.36, base + 2.25 * scale), 0.35 * scale)
    # Stylized lion: compact body, upright chest, mane, head, and a curved tail.
    sphere(f"{tag}_LionBody", (x, 0.0, z), (0.42 * scale, 0.28 * scale, 0.58 * scale), GOLD)
    sphere(f"{tag}_LionHead", (x + 0.22 * scale, 0.0, z + 0.65 * scale), (0.28 * scale, 0.24 * scale, 0.28 * scale), GOLD)
    torus(f"{tag}_LionMane", (x + 0.18 * scale, 0.0, z + 0.63 * scale), 0.29 * scale, 0.07 * scale, GOLD, rotation=(0.0, pi / 2.0, 0.0))
    rope_curve(f"{tag}_LionTail", [(x - 0.26 * scale, 0.0, z + 0.1 * scale), (x - 0.55 * scale, 0.0, z + 0.52 * scale), (x - 0.38 * scale, 0.0, z + 0.76 * scale)], 0.055 * scale, GOLD)


def add_anchor(tag, cx, half_length, beam, base, scale):
    bow = cx + half_length
    for side in (-1.0, 1.0):
        y = side * (hull_width(half_length * 0.72, half_length, beam) + 0.35 * scale)
        x = bow - 2.15 * scale
        z = base + 0.65 * scale
        cylinder(f"{tag}_AnchorShank_{side}", (x, y, z), 0.07 * scale, 1.65 * scale, IRON, rotation=(0.0, 0.42, 0.0), vertices=10)
        torus(f"{tag}_AnchorRing_{side}", (x + 0.35 * scale, y, z + 0.73 * scale), 0.19 * scale, 0.045 * scale, IRON, rotation=(pi / 2.0, 0.0, 0.0))
        rope_curve(f"{tag}_AnchorFlukeA_{side}", [(x - 0.5 * scale, y, z - 0.55 * scale), (x - 0.76 * scale, y, z - 0.82 * scale)], 0.055 * scale, IRON)
        rope_curve(f"{tag}_AnchorFlukeB_{side}", [(x - 0.5 * scale, y, z - 0.55 * scale), (x - 0.18 * scale, y, z - 0.88 * scale)], 0.055 * scale, IRON)


def add_lifeboat(tag, cx, half_length, beam, base, scale):
    x = cx + half_length * 0.10
    y = -(beam + 0.46 * scale)
    z = base + 3.65 * scale
    cube(f"{tag}_LifeboatHull", (x, y, z), (3.1 * scale, 0.75 * scale, 0.34 * scale), OAK, 0.20 * scale)
    cube(f"{tag}_LifeboatInterior", (x, y - 0.02 * scale, z + 0.13 * scale), (2.65 * scale, 0.55 * scale, 0.12 * scale), VOID, 0.06 * scale)
    for index in range(3):
        cube(f"{tag}_LifeboatSeat_{index}", (x - 0.72 * scale + index * 0.72 * scale, y, z + 0.22 * scale), (0.10 * scale, 0.86 * scale, 0.08 * scale), DECK, 0.02)
    rope_curve(f"{tag}_LifeboatTackleA", [(x - 1.25 * scale, y, z + 0.18 * scale), (x - 1.25 * scale, y, z + 1.45 * scale)], 0.025 * scale)
    rope_curve(f"{tag}_LifeboatTackleB", [(x + 1.25 * scale, y, z + 0.18 * scale), (x + 1.25 * scale, y, z + 1.45 * scale)], 0.025 * scale)


def add_rigging_detail(tag, cx, half_length, beam, base, scale, mast_positions):
    for mast_index, mast_x in enumerate(mast_positions):
        mast_height = (base + (16.5 - mast_index * 1.6) * scale)
        deck_z = base + 3.7 * scale
        nest_z = base + (10.4 - mast_index * 0.75) * scale
        cylinder(f"{tag}_CrowsNestRing_{mast_index}", (mast_x, 0.0, nest_z), 0.38 * scale, 0.18 * scale, DARK, vertices=16)
        torus(f"{tag}_CrowsNestRail_{mast_index}", (mast_x, 0.0, nest_z + 0.12 * scale), 0.5 * scale, 0.035 * scale, GOLD)
        for side in (-1.0, 1.0):
            outer_y = side * (beam * 0.92)
            rope_curve(
                f"{tag}_Shroud_{mast_index}_{side}",
                [(mast_x, 0.0, mast_height), (mast_x - 0.7 * scale, side * beam * 0.38, nest_z + 0.2 * scale), (mast_x - 1.25 * scale, outer_y, deck_z)],
                0.028 * scale,
            )
            # Ratlines read much better at a fleet-camera distance than isolated shrouds.
            for rung in range(6):
                z = deck_z + 0.8 * scale + rung * 0.78 * scale
                spread = beam * (0.76 - rung * 0.07)
                rope_curve(
                    f"{tag}_Ratline_{mast_index}_{side}_{rung}",
                    [(mast_x - 1.15 * scale, side * spread, z), (mast_x + 0.25 * scale, side * spread * 0.45, z + 0.02 * scale)],
                    0.017 * scale,
                )
    bow = cx + half_length
    stern = cx - half_length
    rope_curve(f"{tag}_ForestayOuter", [(bow + 2.0 * scale, 0.0, base + 3.0 * scale), (mast_positions[-1], 0.0, base + 15.5 * scale)], 0.035 * scale)
    rope_curve(f"{tag}_BackstayOuter", [(stern - 0.35 * scale, 0.0, base + 6.2 * scale), (mast_positions[0], 0.0, base + 14.5 * scale)], 0.035 * scale)


def add_hull_detail(tag, cx, length, beam, base, scale, flagship):
    half_length = length / 2.0
    # Deep dark wales break the otherwise slab-like side into a ship-shaped hull.
    side_course(f"{tag}_LowerWale", cx, half_length, beam, base + 0.72 * scale, 0.10 * scale, DARK, 0.08 * scale)
    side_course(f"{tag}_MainWale", cx, half_length, beam, base + 1.78 * scale, 0.10 * scale, DARK, 0.10 * scale)
    side_course(f"{tag}_UpperWale", cx, half_length, beam, base + 3.38 * scale, 0.08 * scale, GOLD if flagship else DARK, 0.055 * scale)
    for course in range(6):
        side_course(
            f"{tag}_PlankCourse_{course}",
            cx,
            half_length,
            beam,
            base + (0.35 + course * 0.54) * scale,
            0.045 * scale,
            OAK,
            0.018 * scale,
        )
    # Broad, gently curved stern-to-bow ornament.  This is intentionally sparse on non-flagships.
    if flagship:
        for side in (-1.0, 1.0):
            points = []
            for index in range(9):
                local_x = -half_length * 0.80 + index * (half_length * 1.55 / 8.0)
                y = side * (hull_width(local_x, half_length, beam) + 0.16 * scale)
                z = base + 3.9 * scale + 0.16 * sin(index / 8.0 * pi)
                points.append((cx + local_x, y, z))
            rope_curve(f"{tag}_GoldSheerScroll_{side}", points, 0.048 * scale, GOLD)
    add_stern_gallery(tag, cx, half_length, beam, base, scale, flagship)
    add_figurehead(tag, cx, half_length, beam, base, scale)
    add_anchor(tag, cx, half_length, beam, base, scale)
    add_lifeboat(tag, cx, half_length, beam, base, scale)
    mast_positions = [cx - length * 0.25, cx, cx + length * 0.24]
    add_rigging_detail(tag, cx, half_length, beam, base, scale, mast_positions)


def add_all_refinements():
    # Existing source scene has these three ships placed side-by-side for the preview.
    add_hull_detail("Refined_FirstRate", -40.0, 27.0, 4.30, 0.0, 1.0, True)
    add_hull_detail("Refined_SecondRate", 0.0, 23.6, 3.45, 0.0, 0.84, False)
    add_hull_detail("Refined_Frigate", 35.0, 21.4, 2.80, 0.0, 0.70, False)


def is_exportable(obj):
    return obj.type in {"MESH", "CURVE"} and not obj.hide_render


def belongs_to_variant(obj, variant):
    name = obj.name
    if variant == "FirstRate":
        return is_exportable(obj) and not name.startswith(("SecondRate_", "Frigate_", "Refined_SecondRate", "Refined_Frigate", "Fleet", "Render"))
    if variant == "SecondRate":
        return is_exportable(obj) and name.startswith(("SecondRate_", "Refined_SecondRate"))
    return is_exportable(obj) and name.startswith(("Frigate_", "Refined_Frigate"))


def export_variant(variant, center_x, output_name):
    temporary = new_collection(f"_FBX_EXPORT_{variant}")
    copies = []
    for source in list(bpy.context.scene.objects):
        if not belongs_to_variant(source, variant):
            continue
        duplicate = source.copy()
        if source.data:
            duplicate.data = source.data.copy()
        temporary.objects.link(duplicate)
        duplicate.location.x -= center_x
        copies.append(duplicate)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in copies:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = copies[0]
    target = RAW / output_name
    bpy.ops.export_scene.fbx(
        filepath=str(target),
        use_selection=True,
        # The FBX operator categorizes Blender curve objects as OTHER.
        # Keeping them exports the rigging and decorative hull courses too.
        object_types={"MESH", "OTHER"},
        apply_unit_scale=True,
        bake_space_transform=True,
        add_leaf_bones=False,
        path_mode="AUTO",
    )
    for obj in copies:
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.data.collections.remove(temporary)
    print(f"Exported {variant}: {target} ({len(copies)} objects)")


def prepare_preview():
    for obj in bpy.context.scene.objects:
        if obj.type == "FONT":
            obj.hide_render = True
        if obj.name.startswith(("SecondRate_", "Frigate_", "Refined_SecondRate", "Refined_Frigate")):
            obj.hide_render = True
    camera = bpy.data.objects.get("FleetPreview_Camera")
    if camera is None:
        return
    bpy.context.scene.camera = camera
    camera.location = (-20.0, -68.0, 28.0)
    target = Vector((-40.0, 0.0, 8.0))
    direction = target - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    camera.data.lens = 54
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1536
    scene.render.resolution_y = 1024
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(PREVIEW)
    scene.render.film_transparent = False
    scene.world.color = (0.004, 0.008, 0.016)
    bpy.ops.render.render(write_still=True)


def main():
    if bpy.data.objects.get("Refined_FirstRate_LionBody") is None:
        add_all_refinements()
    # Save a distinct master scene; the original remains a baseline for future work.
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    export_variant("FirstRate", -40.0, "AgeOfSailWarship_FirstRate_GalleonRefined.fbx")
    export_variant("SecondRate", 0.0, "AgeOfSailWarship_SecondRate_GalleonRefined.fbx")
    export_variant("Frigate", 35.0, "AgeOfSailWarship_Frigate_GalleonRefined.fbx")
    prepare_preview()
    print("Galleon refinement complete")


if __name__ == "__main__":
    main()
