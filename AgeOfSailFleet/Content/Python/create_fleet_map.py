"""Create and save the minimal FleetOcean map used by the runtime world director."""

import unreal


MAP_PATH = "/Game/Maps/FleetOcean"


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        created = unreal.EditorLevelLibrary.load_level(MAP_PATH)
    else:
        created = unreal.EditorLevelLibrary.new_level(MAP_PATH)
    if not created:
        raise RuntimeError("Could not open or create {}".format(MAP_PATH))

    world = unreal.EditorLevelLibrary.get_editor_world()
    if world:
        settings = world.get_world_settings()
        game_mode_class = unreal.load_class(None, "/Script/AgeOfSailFleet.SailGameMode")
        if settings and game_mode_class:
            settings.set_editor_property("default_game_mode", game_mode_class)

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save {}".format(MAP_PATH))
    unreal.log("Created {}".format(MAP_PATH))


main()
