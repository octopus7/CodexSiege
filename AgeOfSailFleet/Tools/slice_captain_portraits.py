"""Slice the ImageGen 4x2 captain atlas into seven game portrait textures."""

from pathlib import Path
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Content" / "Raw" / "UI" / "Captains" / "CaptainAtlas_Source.png"
OUTPUT = SOURCE.parent
NAMES = [
    "Blue_Admiral_Ward",
    "Blue_Captain_Mercer",
    "Blue_Captain_Reed",
    "Red_Admiral_Voss",
    "Red_Captain_Marat",
    "Red_Captain_Vale",
    "Red_Captain_Cruz",
]


def main() -> None:
    atlas = Image.open(SOURCE).convert("RGB")
    cell_width = atlas.width // 4
    cell_height = atlas.height // 2
    cells = [(0, 0), (1, 0), (2, 0), (3, 0), (0, 1), (1, 1), (2, 1)]
    for name, (column, row) in zip(NAMES, cells):
        portrait = atlas.crop(
            (
                column * cell_width,
                row * cell_height,
                (column + 1) * cell_width,
                (row + 1) * cell_height,
            )
        )
        portrait.save(OUTPUT / f"{name}.png", quality=95)
        print(f"Wrote {name}.png ({portrait.width}x{portrait.height})")


if __name__ == "__main__":
    main()
