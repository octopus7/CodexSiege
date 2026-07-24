"""Rebuild only WBP_SailFleetHUD without touching the title-screen asset."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from create_ui_assets import _build_blueprint


if __name__ == "__main__":
    _build_blueprint()
