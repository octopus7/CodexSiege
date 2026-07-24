"""Rebuild only WBP_TitleScreen without touching the battle HUD asset."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from create_ui_assets import _build_title_blueprint


if __name__ == "__main__":
    _build_title_blueprint()
