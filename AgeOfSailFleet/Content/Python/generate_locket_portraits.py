"""Create captain portraits that share one exact oval alpha mask.

The ImageGen locket frames are authored on a 1254 square canvas and intentionally
share the same inner aperture. Every portrait is fitted to the conservative oval
below, so bronze, silver, and gold frames can be swapped without changing layout
or exposing square portrait corners.
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageOps


PROJECT = Path(__file__).resolve().parents[2]
SOURCE_DIR = PROJECT / "Content" / "Raw" / "UI" / "Captains"
OUTPUT_DIR = PROJECT / "Content" / "Raw" / "UI" / "CaptainsMasked"

CANVAS_SIZE = (1254, 1254)
# Shared by all frame tiers. The inset keeps the portrait safely beneath the
# metallic inner lip even after UI filtering and down-scaling.
PORTRAIT_OVAL = (366, 306, 885, 991)


def build_masked_portrait(source: Path, output: Path) -> None:
    left, top, right, bottom = PORTRAIT_OVAL
    aperture_size = (right - left, bottom - top)

    portrait = Image.open(source).convert("RGB")
    portrait = ImageOps.fit(
        portrait,
        aperture_size,
        method=Image.Resampling.LANCZOS,
        centering=(0.5, 0.36),
    )

    # A one-pixel soft edge survives bilinear UI sampling without a hard halo.
    aperture_mask = Image.new("L", aperture_size, 0)
    draw = ImageDraw.Draw(aperture_mask)
    draw.ellipse((1, 1, aperture_size[0] - 2, aperture_size[1] - 2), fill=255)
    aperture_mask = aperture_mask.filter(ImageFilter.GaussianBlur(0.65))

    canvas = Image.new("RGBA", CANVAS_SIZE, (0, 0, 0, 0))
    portrait_rgba = portrait.convert("RGBA")
    portrait_rgba.putalpha(aperture_mask)
    canvas.alpha_composite(portrait_rgba, (left, top))
    canvas.save(output, optimize=True)


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for source in sorted(SOURCE_DIR.glob("*.png")):
        if "_Source" in source.stem or "Atlas" in source.stem:
            continue
        output = OUTPUT_DIR / f"{source.stem}_Oval.png"
        build_masked_portrait(source, output)
        print(f"Wrote {output}")


if __name__ == "__main__":
    main()
