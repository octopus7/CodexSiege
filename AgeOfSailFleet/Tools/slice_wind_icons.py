"""Slice the ImageGen compass/arrow atlas into padded transparent UI icons."""

from pathlib import Path
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Content" / "Raw" / "UI" / "Wind" / "WindIcons_Alpha.png"
OUTPUT = SOURCE.parent


def crop_icon(atlas: Image.Image, left: int, right: int, name: str) -> None:
    cell = atlas.crop((left, 0, right, atlas.height))
    bounds = cell.getchannel("A").getbbox()
    if not bounds:
        raise RuntimeError(f"No opaque pixels found for {name}")
    padding = 28
    box = (
        max(0, bounds[0] - padding),
        max(0, bounds[1] - padding),
        min(cell.width, bounds[2] + padding),
        min(cell.height, bounds[3] + padding),
    )
    icon = cell.crop(box)
    icon.save(OUTPUT / f"{name}.png")
    print(f"Wrote {name}.png ({icon.width}x{icon.height})")


def main() -> None:
    atlas = Image.open(SOURCE).convert("RGBA")
    middle = atlas.width // 2
    crop_icon(atlas, 0, middle, "Wind_Compass")
    crop_icon(atlas, middle, atlas.width, "Wind_Arrow")


if __name__ == "__main__":
    main()
