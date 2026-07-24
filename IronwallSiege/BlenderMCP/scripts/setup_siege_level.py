import unreal


LEVEL_PATH = "/Game/Maps/SiegeField"
STATIC_FOLDER = "Battlefield/Static"
ENVIRONMENT_FOLDER = "Battlefield/Environment"
GAMEPLAY_FOLDER = "Battlefield/Gameplay"


def label_and_folder(actor, label, folder):
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    return actor


def spawn_actor(actor_class, label, location=None, rotation=None, folder=STATIC_FOLDER):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class,
        location or unreal.Vector(),
        rotation or unreal.Rotator(),
    )
    if actor is None:
        raise RuntimeError(f"Could not spawn {label}")
    return label_and_folder(actor, label, folder)


def set_mobility(component, mobility):
    if component is not None:
        component.set_editor_property("mobility", mobility)


def spawn_siege_asset(label, slot, location, scale=None):
    actor = spawn_actor(
        unreal.SiegeAssetProxyActor,
        label,
        unreal.Vector(*location),
        folder=STATIC_FOLDER,
    )
    actor.configure_asset(slot)
    actor.set_actor_scale3d(unreal.Vector(*(scale or (1.0, 1.0, 1.0))))
    return actor


def configure_environment():
    sun = spawn_actor(
        unreal.DirectionalLight,
        "Sun",
        unreal.Vector(0.0, 0.0, 2200.0),
        unreal.Rotator(pitch=-34.0, yaw=105.0, roll=0.0),
        ENVIRONMENT_FOLDER,
    )
    sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sun_component.set_editor_property("intensity", 5.0)
    sun_component.set_editor_property("light_color", unreal.Color(255, 232, 207, 255))
    sun_component.set_editor_property("atmosphere_sun_light", True)
    sun_component.set_editor_property("cast_shadows", True)
    set_mobility(sun_component, unreal.ComponentMobility.MOVABLE)

    spawn_actor(
        unreal.SkyAtmosphere,
        "SkyAtmosphere",
        folder=ENVIRONMENT_FOLDER,
    )

    sky_light = spawn_actor(
        unreal.SkyLight,
        "SkyLight",
        unreal.Vector(0.0, 0.0, 1200.0),
        folder=ENVIRONMENT_FOLDER,
    )
    sky_component = sky_light.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_editor_property("intensity", 1.65)
    sky_component.set_editor_property("real_time_capture", True)
    set_mobility(sky_component, unreal.ComponentMobility.MOVABLE)

    fog = spawn_actor(
        unreal.ExponentialHeightFog,
        "HeightFog",
        unreal.Vector(0.0, 0.0, -100.0),
        folder=ENVIRONMENT_FOLDER,
    )
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_component.set_editor_property("fog_density", 0.005)
    fog_component.set_editor_property("fog_height_falloff", 0.16)
    fog_component.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.45, 0.56, 0.70, 1.0))
    fog_component.set_editor_property("enable_volumetric_fog", True)
    fog_component.set_editor_property("volumetric_fog_scattering_distribution", 0.35)
    fog_component.set_editor_property("volumetric_fog_extinction_scale", 0.65)

    cloud = spawn_actor(
        unreal.VolumetricCloud,
        "VolumetricCloud",
        unreal.Vector(0.0, 0.0, 0.0),
        folder=ENVIRONMENT_FOLDER,
    )
    cloud_component = cloud.get_component_by_class(unreal.VolumetricCloudComponent)
    cloud_material = unreal.load_asset(
        "/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst"
    )
    if cloud_component is not None and cloud_material is not None:
        cloud_component.set_material(cloud_material)

    post_process = spawn_actor(
        unreal.PostProcessVolume,
        "GlobalPostProcess",
        folder=ENVIRONMENT_FOLDER,
    )
    post_process.set_editor_property("unbound", True)
    settings = post_process.get_editor_property("settings")
    settings.override_auto_exposure_bias = True
    settings.auto_exposure_bias = 0.0
    settings.override_color_saturation = True
    settings.color_saturation = unreal.Vector4(1.02, 1.0, 0.95, 1.0)
    settings.override_color_contrast = True
    settings.color_contrast = unreal.Vector4(1.02, 1.02, 1.01, 1.0)
    post_process.set_editor_property("settings", settings)


def configure_static_battlefield():
    spawn_siege_asset(
        "SiegeField",
        unreal.SiegeAssetSlot.GROUND,
        (0.0, 0.0, -5.0),
        (1.4, 1.5, 1.0),
    )

    fortress_y = 1250.0
    spawn_siege_asset(
        "Gatehouse",
        unreal.SiegeAssetSlot.GATE,
        (0.0, fortress_y, 0.0),
    )
    spawn_siege_asset(
        "Wall_L1",
        unreal.SiegeAssetSlot.WALL,
        (-640.0, fortress_y, 0.0),
        (1.05, 1.0, 1.0),
    )
    spawn_siege_asset(
        "Wall_L2",
        unreal.SiegeAssetSlot.WALL,
        (-1280.0, fortress_y, 0.0),
        (1.05, 1.0, 1.0),
    )
    spawn_siege_asset(
        "Wall_R1",
        unreal.SiegeAssetSlot.WALL,
        (640.0, fortress_y, 0.0),
        (1.05, 1.0, 1.0),
    )
    spawn_siege_asset(
        "Wall_R2",
        unreal.SiegeAssetSlot.WALL,
        (1280.0, fortress_y, 0.0),
        (1.05, 1.0, 1.0),
    )
    spawn_siege_asset(
        "Tower_L",
        unreal.SiegeAssetSlot.TOWER,
        (-1740.0, fortress_y, 0.0),
    )
    spawn_siege_asset(
        "Tower_R",
        unreal.SiegeAssetSlot.TOWER,
        (1740.0, fortress_y, 0.0),
    )


def configure_gameplay_anchors():
    spawn_actor(
        unreal.SiegeCameraPawn,
        "BattlefieldCamera",
        unreal.Vector(0.0, -5200.0, 2000.0),
        unreal.Rotator(pitch=-20.0, yaw=90.0, roll=0.0),
        GAMEPLAY_FOLDER,
    )


def build_level():
    unreal.log(f"Creating level {LEVEL_PATH}")
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        if not unreal.EditorLevelLibrary.load_level(LEVEL_PATH):
            raise RuntimeError(f"Could not load level {LEVEL_PATH}")
        for actor in unreal.EditorLevelLibrary.get_all_level_actors():
            unreal.EditorLevelLibrary.destroy_actor(actor)
    elif not unreal.EditorLevelLibrary.new_level(LEVEL_PATH):
        raise RuntimeError(f"Could not create level {LEVEL_PATH}")

    configure_environment()
    configure_static_battlefield()
    configure_gameplay_anchors()

    world = unreal.EditorLevelLibrary.get_editor_world()
    world_settings = world.get_world_settings()
    world_settings.set_editor_property("kill_z", -5000.0)

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError(f"Could not save level {LEVEL_PATH}")

    unreal.EditorAssetLibrary.save_directory("/Game/Maps", only_if_is_dirty=False, recursive=True)
    unreal.log("IronwallSiegeLevelSetup success level=/Game/Maps/SiegeField")


build_level()
