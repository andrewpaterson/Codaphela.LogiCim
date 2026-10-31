#!/usr/bin/env python3
"""Converts Logicim's KiCad style icons to IntelliJ IDEA's dark theme.

KiCad draws its icons for a light background: dark grey strokes, near white fills and blue, red and orange accents.
On IntelliJ's dark background the strokes are light grey, the fills dark grey and the accents are the dark theme's
blue, red, yellow and green.

Every pixel keeps its opacity.  A grey pixel is placed between the light stroke grey and the dark fill grey by how
light it was, so anti-aliased edges stay smooth.  A near white pixel surrounded by colour, like the cross on the red
cancel circle, is a glyph drawn over the colour and stays white.  A coloured pixel takes the theme colour for its hue
with its lightness moved by as much as it differed from the KiCad colour.

    python3 ConvertLogicimIcons.py <Logicim res/icons directory> <output directory>
"""

import colorsys
import os
import sys

from PIL import Image


STROKE = (0xCE, 0xD0, 0xD6)  # IntelliJ dark theme icon grey.
FILL = (0x43, 0x45, 0x4A)  # IntelliJ dark theme icon fill.
WHITE = (0xFF, 0xFF, 0xFF)

KICAD_STROKE_LIGHTNESS = 0x53 / 255.0
KICAD_FILL_LIGHTNESS = 0xF3 / 255.0

# Theme colours by the KiCad hue they replace: (lowest hue, highest hue, KiCad colour, theme colour).
BLUE = (0x54, 0x8A, 0xF7)
RED = (0xDB, 0x5C, 0x5C)
YELLOW = (0xF2, 0xC5, 0x5C)
GREEN = (0x5F, 0xB8, 0x65)
HUES = [
    (170, 260, (0x1A, 0x81, 0xC4), BLUE),
    (300, 360, (0xBF, 0x26, 0x41), RED),
    (0, 15, (0xBF, 0x26, 0x41), RED),
    (15, 70, (0xF2, 0x91, 0x00), YELLOW),
]

# IntelliJ runs things with a green triangle, KiCad's is blue.
ICON_HUES = {
    "sim_run_24.png": [(170, 260, (0x1A, 0x81, 0xC4), GREEN)],
}

GREY_SATURATION = 0.15
GLYPH_LIGHTNESS = 0.85


def lightness(rgb):
    return colorsys.rgb_to_hls(*[c / 255.0 for c in rgb])[1]


def saturation(rgb):
    return colorsys.rgb_to_hls(*[c / 255.0 for c in rgb])[2]


def lerp(a, b, t):
    t = max(0.0, min(1.0, t))
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def is_coloured(pixel):
    return pixel[3] > 128 and saturation(pixel[:3]) >= GREY_SATURATION


def is_dark_grey(pixel):
    return pixel[3] > 128 and saturation(pixel[:3]) < GREY_SATURATION and lightness(pixel[:3]) < 0.6


def is_glyph(pixels, width, height, x, y):
    """A light pixel with more colour than dark grey strokes around it is drawn over a coloured shape."""
    coloured = 0
    grey = 0
    for dy in range(-3, 4):
        for dx in range(-3, 4):
            nx = x + dx
            ny = y + dy
            if 0 <= nx < width and 0 <= ny < height:
                neighbour = pixels[ny * width + nx]
                if is_coloured(neighbour):
                    coloured += 1
                elif is_dark_grey(neighbour):
                    grey += 1
    return coloured > grey


def convert_grey(rgb):
    t = (lightness(rgb) - KICAD_STROKE_LIGHTNESS) / (KICAD_FILL_LIGHTNESS - KICAD_STROKE_LIGHTNESS)
    return lerp(STROKE, FILL, t)


def convert_colour(rgb, hues):
    hue, light, _ = colorsys.rgb_to_hls(*[c / 255.0 for c in rgb])
    hue *= 360.0
    for low, high, kicad, theme in hues:
        if low <= hue < high:
            kicad_light = lightness(kicad)
            theme_hue, theme_light, theme_saturation = colorsys.rgb_to_hls(*[c / 255.0 for c in theme])
            if light >= kicad_light:
                # Lighter than the KiCad colour: a blend with a white glyph, which stays white.
                t = (light - kicad_light) / (1.0 - kicad_light)
                return lerp(theme, WHITE, t)
            else:
                # Darker: a blend with a dark stroke, which becomes the light stroke grey.
                t = (kicad_light - light) / (kicad_light - KICAD_STROKE_LIGHTNESS)
                return lerp(theme, STROKE, t)
    return convert_grey(rgb)


def convert(source_filename, dest_filename):
    image = Image.open(source_filename).convert("RGBA")
    width, height = image.size
    pixels = list(image.getdata())
    hues = ICON_HUES.get(os.path.basename(source_filename), []) + HUES

    converted = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[y * width + x]
            if a == 0:
                converted.append((0, 0, 0, 0))
                continue
            rgb = (r, g, b)
            if saturation(rgb) >= GREY_SATURATION:
                rgb = convert_colour(rgb, hues)
            elif lightness(rgb) >= GLYPH_LIGHTNESS and is_glyph(pixels, width, height, x, y):
                rgb = WHITE
            else:
                rgb = convert_grey(rgb)
            converted.append(rgb + (a,))

    result = Image.new("RGBA", (width, height))
    result.putdata(converted)
    result.save(dest_filename)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 1

    source_directory = sys.argv[1]
    dest_directory = sys.argv[2]
    os.makedirs(dest_directory, exist_ok=True)
    for filename in sorted(os.listdir(source_directory)):
        if filename.endswith(".png"):
            convert(os.path.join(source_directory, filename), os.path.join(dest_directory, filename))
    return 0


if __name__ == "__main__":
    sys.exit(main())
