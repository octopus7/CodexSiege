"""Slice an ImageGen 4x2 directional ship atlas into uniform 512px sprites."""

from pathlib import Path

from PIL import Image


PROJECT = Path(__file__).resolve().parents[1]
SOURCE_DIR = PROJECT / "Content" / "Raw" / "Sprites" / "Ships"
DIRECTIONS = ("N", "NE", "E", "SE", "S", "SW", "W", "NW")
OUTPUT_SIZE = 512


def slice_atlas(faction: str) -> None:
    source = SOURCE_DIR / f"Ship_{faction}_8Dir.png"
    if not source.exists():
        raise FileNotFoundError(source)

    with Image.open(source).convert("RGBA") as atlas:
        width, height = atlas.size
        for index, direction in enumerate(DIRECTIONS):
            column = index % 4
            row = index // 4
            left = round(column * width / 4)
            right = round((column + 1) * width / 4)
            top = round(row * height / 2)
            bottom = round((row + 1) * height / 2)
            cell = atlas.crop((left, top, right, bottom))

            # Normalize the odd-sized ImageGen atlas cells without stretching
            # the painted ship. A square transparent canvas keeps all imported
            # UBillboardComponent sprites at an identical world scale.
            cell.thumbnail((OUTPUT_SIZE, OUTPUT_SIZE), Image.Resampling.LANCZOS)
            output = Image.new("RGBA", (OUTPUT_SIZE, OUTPUT_SIZE), (0, 0, 0, 0))
            output.alpha_composite(
                cell,
                ((OUTPUT_SIZE - cell.width) // 2, (OUTPUT_SIZE - cell.height) // 2),
            )
            output.save(SOURCE_DIR / f"Ship_{faction}_{direction}.png")


if __name__ == "__main__":
    for fleet_faction in ("Blue", "Red"):
        slice_atlas(fleet_faction)
