"""Import generated textures and Blender FBX, then build runtime materials."""

from pathlib import Path
import unreal


PROJECT = Path(unreal.Paths.project_dir())
RAW = PROJECT / "Content" / "Raw"


def import_file(
    source: Path,
    destination: str,
    replace: bool = True,
    asset_name: str | None = None,
):
    if not source.exists():
        unreal.log_warning(f"Missing generated asset: {source}")
        return None
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    if asset_name:
        task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", replace)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if task.get_editor_property("imported_object_paths"):
        return unreal.load_asset(task.get_editor_property("imported_object_paths")[0])
    return None


def import_texture(source: Path, asset_name: str, destination: str):
    texture = import_file(source, destination, asset_name=asset_name)
    if texture and texture.get_name() != asset_name:
        old_path = texture.get_path_name()
        unreal.EditorAssetLibrary.rename_asset(
            old_path,
            f"{destination}/{asset_name}",
        )
        texture = unreal.load_asset(f"{destination}/{asset_name}")
    if texture:
        try:
            texture.set_editor_property("srgb", True)
        except Exception:
            pass
        try:
            texture.set_editor_property(
                "compression_settings",
                unreal.TextureCompressionSettings.TC_DEFAULT,
            )
        except Exception:
            pass
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def import_normal_texture(source: Path, asset_name: str, destination: str):
    texture = import_file(source, destination, asset_name=asset_name)
    if texture and texture.get_name() != asset_name:
        old_path = texture.get_path_name()
        unreal.EditorAssetLibrary.rename_asset(
            old_path,
            f"{destination}/{asset_name}",
        )
        texture = unreal.load_asset(f"{destination}/{asset_name}")
    if texture:
        texture.set_editor_property("srgb", False)
        texture.set_editor_property(
            "compression_settings",
            unreal.TextureCompressionSettings.TC_NORMALMAP,
        )
        try:
            texture.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
            texture.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
        except Exception:
            pass
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def ensure_runtime_ui_font():
    """Wrap the imported TTF FontFace in the UFont asset Slate requires."""
    font_path = "/Game/UI/Fonts/FleetAntiqueRuntime"
    face_path = "/Game/UI/Fonts/FleetAntiqueFace"
    font_asset = unreal.load_asset(font_path)
    font_face = unreal.load_asset(face_path)

    # Migrate the first version of this project, where the imported FontFace was
    # incorrectly named FleetAntique and assigned directly to FSlateFontInfo.
    legacy_face = unreal.load_asset("/Game/UI/Fonts/FleetAntique")
    if not font_face and isinstance(legacy_face, unreal.FontFace):
        font_face = legacy_face

    if not font_face:
        imported_face = import_file(
            RAW / "UI" / "Fonts" / "PirataOne-Regular.ttf",
            "/Game/UI/Fonts",
        )
        if imported_face:
            unreal.EditorAssetLibrary.rename_asset(
                imported_face.get_path_name(),
                face_path,
            )
            font_face = unreal.load_asset(face_path)

    if not font_asset:
        font_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "FleetAntiqueRuntime",
            "/Game/UI/Fonts",
            unreal.Font,
            unreal.FontFactory(),
        )

    if not font_asset or not font_face:
        unreal.log_error("Could not create the FleetAntique runtime UI font.")
        return None
    if not unreal.SailFleetUIEditorLibrary.configure_runtime_font(
        font_asset, font_face
    ):
        unreal.log_error("Could not configure the FleetAntique runtime UI font.")
        return None
    unreal.EditorAssetLibrary.save_loaded_asset(font_face)
    unreal.EditorAssetLibrary.save_loaded_asset(font_asset)
    return font_asset


def make_texture_material(name: str, texture, translucent: bool = False):
    path = f"/Game/Art/Materials/{name}"
    existing = unreal.load_asset(path)
    material = existing
    if not material:
        factory = unreal.MaterialFactoryNew()
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name,
            "/Game/Art/Materials",
            unreal.Material,
            factory,
        )
    if not material or not texture:
        return
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSample,
        -200,
        0,
    )
    sample.set_editor_property("texture", texture)
    if translucent:
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        material.set_editor_property("two_sided", True)
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        unreal.MaterialEditingLibrary.connect_material_property(
            sample,
            "RGB",
            unreal.MaterialProperty.MP_EMISSIVE_COLOR,
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            sample,
            "A",
            unreal.MaterialProperty.MP_OPACITY,
        )
    else:
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
        material.set_editor_property("two_sided", True)
        unreal.MaterialEditingLibrary.connect_material_property(
            sample,
            "RGB",
            unreal.MaterialProperty.MP_BASE_COLOR,
        )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)


def make_wake_material():
    name = "M_ShipWake"
    path = f"/Game/Art/Materials/{name}"
    material = unreal.load_asset(path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name,
            "/Game/Art/Materials",
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if not material:
        return
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    vertex_color = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionVertexColor,
        -200,
        0,
    )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    unreal.MaterialEditingLibrary.connect_material_property(
        vertex_color,
        "RGB",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        vertex_color,
        "A",
        unreal.MaterialProperty.MP_OPACITY,
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)


def make_ocean_material(wave_normal):
    name = "M_ProceduralOcean"
    path = f"/Game/Art/Materials/{name}"
    material = unreal.load_asset(path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name,
            "/Game/Art/Materials",
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if not material:
        return
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    vertex_color = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionVertexColor,
        -620,
        -180,
    )
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionConstant,
        -260,
        100,
    )
    roughness.set_editor_property("r", 0.16)
    specular = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionConstant,
        -260,
        190,
    )
    specular.set_editor_property("r", 0.72)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("two_sided", True)
    try:
        material.set_editor_property("tangent_space_normal", True)
    except Exception:
        pass
    unreal.MaterialEditingLibrary.connect_material_property(
        vertex_color,
        "RGB",
        unreal.MaterialProperty.MP_BASE_COLOR,
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness,
        "",
        unreal.MaterialProperty.MP_ROUGHNESS,
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        specular,
        "",
        unreal.MaterialProperty.MP_SPECULAR,
    )

    if wave_normal:
        coordinates_a = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionTextureCoordinate,
            -650,
            40,
        )
        coordinates_a.set_editor_property("u_tiling", 18.0)
        coordinates_a.set_editor_property("v_tiling", 18.0)
        panner_a = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionPanner,
            -470,
            20,
        )
        panner_a.set_editor_property("speed_x", 0.012)
        panner_a.set_editor_property("speed_y", 0.006)
        sample_a = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionTextureSample,
            -250,
            -70,
        )
        sample_a.set_editor_property("texture", wave_normal)
        sample_a.set_editor_property(
            "sampler_type",
            unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
        )

        coordinates_b = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionTextureCoordinate,
            -650,
            260,
        )
        coordinates_b.set_editor_property("u_tiling", 47.0)
        coordinates_b.set_editor_property("v_tiling", 47.0)
        panner_b = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionPanner,
            -470,
            250,
        )
        panner_b.set_editor_property("speed_x", -0.008)
        panner_b.set_editor_property("speed_y", 0.011)
        sample_b = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionTextureSample,
            -250,
            290,
        )
        sample_b.set_editor_property("texture", wave_normal)
        sample_b.set_editor_property(
            "sampler_type",
            unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
        )
        blend = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionLinearInterpolate,
            20,
            30,
        )
        blend.set_editor_property("const_alpha", 0.42)

        unreal.MaterialEditingLibrary.connect_material_expressions(
            coordinates_a, "", panner_a, "Coordinate")
        unreal.MaterialEditingLibrary.connect_material_expressions(
            panner_a, "", sample_a, "UVs")
        unreal.MaterialEditingLibrary.connect_material_expressions(
            coordinates_b, "", panner_b, "Coordinate")
        unreal.MaterialEditingLibrary.connect_material_expressions(
            panner_b, "", sample_b, "UVs")
        unreal.MaterialEditingLibrary.connect_material_expressions(
            sample_a, "RGB", blend, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(
            sample_b, "RGB", blend, "B")
        unreal.MaterialEditingLibrary.connect_material_property(
            blend,
            "",
            unreal.MaterialProperty.MP_NORMAL,
        )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)


def import_ship_fbx(source_name: str, asset_name: str):
    source = RAW / source_name
    if not source.exists():
        unreal.log_warning(
            f"Blender FBX {source_name} is not present; procedural ship fallback remains active."
        )
        return
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("import_materials", True)
    ui.set_editor_property("import_textures", False)
    ui.static_mesh_import_data.set_editor_property("combine_meshes", True)
    ui.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", True)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", "/Game/Art/Ships")
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])


def main():
    blue = import_texture(
        RAW / "Textures" / "BlueFleetSail.png",
        "T_BlueFleetSail",
        "/Game/Art/Textures",
    )
    red = import_texture(
        RAW / "Textures" / "RedFleetSail.png",
        "T_RedFleetSail",
        "/Game/Art/Textures",
    )
    make_texture_material("M_BlueFleetSail", blue)
    make_texture_material("M_RedFleetSail", red)
    hull_oak = import_texture(
        RAW / "Textures" / "HullOakPlanks.png",
        "T_HullOakPlanks",
        "/Game/Art/Textures",
    )
    deck_oak = import_texture(
        RAW / "Textures" / "DeckOakPlanks.png",
        "T_DeckOakPlanks",
        "/Game/Art/Textures",
    )
    ocean_normal = import_normal_texture(
        RAW / "Textures" / "OceanWaveNormal.png",
        "T_OceanWaveNormal",
        "/Game/Art/Textures",
    )
    make_texture_material("M_HullOakPlanks", hull_oak)
    make_texture_material("M_DeckOakPlanks", deck_oak)

    effect_specs = [
        ("CannonMuzzle_Flipbook.png", "T_FX_CannonMuzzle", "M_FX_CannonMuzzle"),
        ("HullImpact_Flipbook.png", "T_FX_HullImpact", "M_FX_HullImpact"),
        ("WaterImpact_Flipbook.png", "T_FX_WaterImpact", "M_FX_WaterImpact"),
    ]
    for filename, texture_name, material_name in effect_specs:
        texture = import_texture(RAW / "FX" / filename, texture_name, "/Game/Art/FX")
        make_texture_material(material_name, texture, translucent=True)
    make_wake_material()
    make_ocean_material(ocean_normal)

    ship_sprite_dir = RAW / "Sprites" / "Ships"
    for faction in ("Blue", "Red"):
        for direction in ("N", "NE", "E", "SE", "S", "SW", "W", "NW"):
            texture = import_texture(
                ship_sprite_dir / f"Ship_{faction}_{direction}.png",
                f"T_Ship_{faction}_{direction}",
                "/Game/Art/Sprites/Ships",
            )
            if texture:
                try:
                    texture.set_editor_property(
                        "lod_group",
                        unreal.TextureGroup.TEXTUREGROUP_PIXELS2D,
                    )
                except Exception:
                    pass
                unreal.EditorAssetLibrary.save_loaded_asset(texture)

    glyph_dir = RAW / "UI" / "DateGlyphs"
    for source in sorted(glyph_dir.glob("*.png")):
        if "_Source" in source.stem or "Atlas" in source.stem:
            continue
        import_texture(source, f"T_Date_{source.stem}", "/Game/UI/DateGlyphs")

    captain_dir = RAW / "UI" / "CaptainsMasked"
    for source in sorted(captain_dir.glob("*.png")):
        if "_Source" in source.stem:
            continue
        import_texture(source, f"T_Captain_{source.stem}", "/Game/UI/Captains")

    locket_dir = RAW / "UI" / "Lockets" / "Final"
    for tier in ("Bronze", "Silver", "Gold"):
        import_texture(
            locket_dir / f"Locket_{tier}.png",
            f"T_Locket_{tier}",
            "/Game/UI/Lockets",
        )

    import_texture(
        RAW / "UI" / "Title" / "DepartureButton.png",
        "T_DepartureButton",
        "/Game/UI/Title",
    )
    import_texture(
        RAW / "UI" / "Title" / "AgeOfSail_Logo.png",
        "T_AgeOfSail_Logo",
        "/Game/UI/Title",
    )

    wind_dir = RAW / "UI" / "Wind"
    import_texture(
        wind_dir / "Wind_Compass.png",
        "T_Wind_Compass",
        "/Game/UI/DateGlyphs",
    )
    import_texture(
        wind_dir / "Wind_Arrow.png",
        "T_Wind_Arrow",
        "/Game/UI/DateGlyphs",
    )

    ensure_runtime_ui_font()

    ship_variants = [
        ("AgeOfSailWarship_FirstRate_GalleonRefined.fbx", "SM_Warship_FirstRate"),
        ("AgeOfSailWarship_SecondRate_GalleonRefined.fbx", "SM_Warship_SecondRate"),
        ("AgeOfSailWarship_Frigate_GalleonRefined.fbx", "SM_Warship_Frigate"),
    ]
    imported_variant = False
    for source_name, asset_name in ship_variants:
        if (RAW / source_name).exists():
            imported_variant = True
            import_ship_fbx(source_name, asset_name)
    if not imported_variant:
        import_ship_fbx("AgeOfSailWarship.fbx", "SM_AgeOfSailWarship")
    unreal.EditorAssetLibrary.save_directory("/Game")
    unreal.log("Age of Sail generated art import complete.")


if __name__ == "__main__":
    main()
