"""
make_icons.py - generate multi-resolution .ico files for all four Nexor apps.

The architecture (Platform-Architecture.pdf, page 1) names a colour for each
app:
    Studio   - blue   (developer)
    Command  - orange (operations)
    Core     - purple (server)
    Flux     - green  (end-user)

Each icon is rendered at 256x256 and 64x64 source resolutions, then bundled
into a Windows .ico (16, 24, 32, 48, 64, 128, 256 multi-resolution).  The
glyphs are deliberately simple and geometric:

    Studio   "</>"        code brackets
    Command  star burst   admin / orchestration
    Core     server stack three horizontal cylinders
    Flux     play arrow   forward motion / end-user run
"""
import os
from PIL import Image, ImageDraw, ImageFilter

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "..", "icons")
os.makedirs(OUT_DIR, exist_ok=True)

# Sizes that ship in every .ico file - Windows picks the closest match per
# render context (taskbar, alt-tab, file explorer, etc.).
ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]


def rounded_rect(d, box, radius, fill, outline=None, width=1):
    """Pillow >= 8 has rounded_rectangle; fall back gracefully otherwise."""
    if hasattr(d, "rounded_rectangle"):
        d.rounded_rectangle(box, radius=radius, fill=fill,
                           outline=outline, width=width)
    else:
        d.rectangle(box, fill=fill, outline=outline, width=width)


def base_canvas(size, bg_top, bg_bot):
    """A square gradient background (top-down) with a soft-rounded corner."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))

    # Vertical gradient.
    grad = Image.new("RGBA", (size, size))
    for y in range(size):
        t = y / max(size - 1, 1)
        r = int(bg_top[0] * (1 - t) + bg_bot[0] * t)
        g = int(bg_top[1] * (1 - t) + bg_bot[1] * t)
        b = int(bg_top[2] * (1 - t) + bg_bot[2] * t)
        for x in range(size):
            grad.putpixel((x, y), (r, g, b, 255))

    # Soft-rounded mask.
    mask = Image.new("L", (size, size), 0)
    md = ImageDraw.Draw(mask)
    if hasattr(md, "rounded_rectangle"):
        md.rounded_rectangle((0, 0, size - 1, size - 1),
                            radius=size // 6, fill=255)
    else:
        md.rectangle((0, 0, size - 1, size - 1), fill=255)
    img.paste(grad, (0, 0), mask)

    # Subtle inner highlight for depth.
    hl = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    hd = ImageDraw.Draw(hl)
    hd.rectangle((0, 0, size, size // 3), fill=(255, 255, 255, 24))
    img.alpha_composite(hl)
    return img


def studio_glyph(size):
    """Code brackets:  </>  in the foreground."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx, cy = size / 2, size / 2
    arm = size * 0.32
    thick = max(2, int(size * 0.07))
    fg = (255, 255, 255, 240)

    # Left bracket  <
    d.line([(cx - arm * 0.55, cy - arm * 0.65),
            (cx - arm,        cy),
            (cx - arm * 0.55, cy + arm * 0.65)],
           fill=fg, width=thick, joint="curve")
    # Right bracket  >
    d.line([(cx + arm * 0.55, cy - arm * 0.65),
            (cx + arm,        cy),
            (cx + arm * 0.55, cy + arm * 0.65)],
           fill=fg, width=thick, joint="curve")
    # Slash  /
    d.line([(cx + arm * 0.10, cy + arm * 0.75),
            (cx - arm * 0.10, cy - arm * 0.75)],
           fill=fg, width=thick)
    return img


def core_glyph(size):
    """Server stack: three flat cylinders ringed with status dots."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx, cy = size / 2, size / 2
    w = size * 0.66
    h = size * 0.10
    gap = size * 0.06
    fg = (255, 255, 255, 235)
    fg2 = (255, 255, 255, 90)

    for i in (-1, 0, 1):
        y = cy + i * (h + gap) - h / 2
        rounded_rect(d, (cx - w / 2, y, cx + w / 2, y + h),
                     radius=h * 0.45, fill=fg)
        # status pip
        pr = h * 0.22
        d.ellipse((cx - w / 2 + h * 0.6 - pr,
                   y + h / 2 - pr,
                   cx - w / 2 + h * 0.6 + pr,
                   y + h / 2 + pr),
                  fill=fg2)
    return img


def command_glyph(size):
    """Star burst (orchestration)."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx, cy = size / 2, size / 2
    r1 = size * 0.36
    r2 = size * 0.16
    fg = (255, 255, 255, 240)

    import math
    pts = []
    for i in range(8):
        a = -math.pi / 2 + i * math.pi / 4
        r = r1 if i % 2 == 0 else r2
        pts.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
    d.polygon(pts, fill=fg)

    # Centre disc
    cr = size * 0.06
    d.ellipse((cx - cr, cy - cr, cx + cr, cy + cr), fill=(255, 255, 255, 90))
    return img


def flux_glyph(size):
    """Play triangle (forward motion / end-user run)."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx, cy = size / 2, size / 2
    r = size * 0.30
    fg = (255, 255, 255, 240)

    # Equilateral-ish play triangle, a bit nudged right of centre so the
    # optical balance feels right.
    nudge = size * 0.04
    pts = [
        (cx - r * 0.85 + nudge, cy - r),
        (cx + r        + nudge, cy),
        (cx - r * 0.85 + nudge, cy + r),
    ]
    d.polygon(pts, fill=fg)
    return img


SPECS = {
    "nexor_studio": {
        "bg":   ((0x32, 0x4f, 0x9c), (0x1e, 0x3a, 0x5f)),     # blue
        "glyph": studio_glyph,
    },
    "nexor_core": {
        "bg":   ((0x6b, 0x46, 0xc1), (0x3a, 0x1e, 0x5f)),     # purple
        "glyph": core_glyph,
    },
    "nexor_command": {
        "bg":   ((0xfb, 0x92, 0x3c), (0x9a, 0x4a, 0x14)),     # orange
        "glyph": command_glyph,
    },
    "nexor_flux": {
        "bg":   ((0x4a, 0xc0, 0x6e), (0x16, 0x6f, 0x3c)),     # green
        "glyph": flux_glyph,
    },
}


def build(name, spec):
    # Source render at 256 (max .ico size).
    canvas = base_canvas(256, *spec["bg"])
    canvas.alpha_composite(spec["glyph"](256))

    # Save the 256 PNG for use elsewhere (Qt resources, store listings).
    png_path = os.path.join(OUT_DIR, f"{name}.png")
    canvas.save(png_path, "PNG")

    # Multi-resolution .ico.
    icons = []
    for s in ICO_SIZES:
        c = base_canvas(s, *spec["bg"])
        c.alpha_composite(spec["glyph"](s))
        icons.append(c)
    ico_path = os.path.join(OUT_DIR, f"{name}.ico")
    icons[0].save(ico_path, format="ICO",
                  sizes=[(s, s) for s in ICO_SIZES])
    print(f"  {name:14}  ->  {png_path}, {ico_path}")


if __name__ == "__main__":
    print("Generating Nexor application icons...")
    for name, spec in SPECS.items():
        build(name, spec)
    print(f"Done. Icons under: {os.path.abspath(OUT_DIR)}")
