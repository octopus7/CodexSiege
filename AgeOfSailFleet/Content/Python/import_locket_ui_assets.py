"""Import only the oval captain portraits and rank locket textures."""

from pathlib import Path
import sys

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_game_assets import import_texture


PROJECT = Path(unreal.Paths.project_dir())
RAW_UI = PROJECT / "Content" / "Raw" / "UI"


def main() -> None:
    captain_dir = RAW_UI / "CaptainsMasked"
    for source in sorted(captain_dir.glob("*.png")):
        import_texture(
            source,
            f"T_Captain_{source.stem}",
            "/Game/UI/Captains",
        )

    locket_dir = RAW_UI / "Lockets" / "Final"
    for tier in ("Bronze", "Silver", "Gold"):
        import_texture(
            locket_dir / f"Locket_{tier}.png",
            f"T_Locket_{tier}",
            "/Game/UI/Lockets",
        )

    unreal.EditorAssetLibrary.save_directory("/Game/UI")
    unreal.log("Imported shared-oval captain portraits and rank locket frames.")


if __name__ == "__main__":
    main()
