"""Slice every ImageGen weekday/month cell with border inset and safe padding."""

from pathlib import Path
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
GLYPHS = ROOT / "Content" / "Raw" / "UI" / "DateGlyphs"

WEEKDAYS = ["MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN", None]
MONTHS = [
    "JANUARY",
    "FEBRUARY",
    "MARCH",
    "APRIL",
    "MAY",
    "JUNE",
    "JULY",
    "AUGUST",
    "SEPTEMBER",
    "OCTOBER",
    "NOVEMBER",
    "DECEMBER",
]


def slice_atlas(
    source_name: str,
    names: list[str | None],
    columns: int,
    rows: int,
    prefix: str,
) -> None:
    atlas = Image.open(GLYPHS / source_name).convert("RGBA")
    cleaned_pixels = []
    for red, green, blue, _alpha in atlas.getdata():
        green_excess = green - max(red, blue)
        if green > 55 and green_excess > 24:
            alpha = 0 if green_excess >= 72 else round(255 * (72 - green_excess) / 48)
            green = min(green, max(red, blue) + 12)
        else:
            alpha = 255
        cleaned_pixels.append((red, green, blue, alpha))
    atlas.putdata(cleaned_pixels)
    for index, name in enumerate(names):
        if not name:
            continue
        column = index % columns
        row = index // columns
        left = round(column * atlas.width / columns)
        right = round((column + 1) * atlas.width / columns)
        top = round(row * atlas.height / rows)
        bottom = round((row + 1) * atlas.height / rows)

        # ImageGen added thin black grid lines despite the prompt. Inset first so
        # those lines cannot become part of the alpha bounding box.
        inset = 12
        cell = atlas.crop((left + inset, top + inset, right - inset, bottom - inset))
        bounds = cell.getchannel("A").getbbox()
        if not bounds:
            raise RuntimeError(f"No visible pixels found for {prefix}_{name}")

        padding = 24
        crop_box = (
            max(0, bounds[0] - padding),
            max(0, bounds[1] - padding),
            min(cell.width, bounds[2] + padding),
            min(cell.height, bounds[3] + padding),
        )
        glyph = cell.crop(crop_box)
        output = GLYPHS / f"{prefix}_{name}.png"
        glyph.save(output)
        print(f"Wrote {output.name} ({glyph.width}x{glyph.height})")


def validate_existing_digits() -> None:
    for name in [*(f"Digit_{n}" for n in range(10)), "Punctuation_Dot", "Punctuation_Comma"]:
        path = GLYPHS / f"{name}.png"
        image = Image.open(path).convert("RGBA")
        bounds = image.getchannel("A").getbbox()
        if not bounds:
            raise RuntimeError(f"{name} has no visible pixels")
        if (
            bounds[0] <= 1
            or bounds[1] <= 1
            or bounds[2] >= image.width - 1
            or bounds[3] >= image.height - 1
        ):
            raise RuntimeError(f"{name} touches its crop edge")
        print(f"Validated {name}.png ({image.width}x{image.height})")


def main() -> None:
    slice_atlas("WeekdayGlyphAtlas_Source.png", WEEKDAYS, 4, 2, "Weekday")
    slice_atlas("MonthGlyphAtlas_Source.png", MONTHS, 3, 4, "Month")
    validate_existing_digits()


if __name__ == "__main__":
    main()
