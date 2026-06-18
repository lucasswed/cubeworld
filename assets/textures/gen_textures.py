"""
Generate pixel-art block and tool textures for CubeWorld.
Run: python3 assets/textures/gen_textures.py
Output: assets/textures/*.png  (16×16 per tile, multi-column for multi-face blocks)

grass.png   — 32×16: [top | side]  (bottom reuses dirt.png automatically)
dirt.png    — 16×16
stone.png   — 16×16
wood.png    — 32×16: [top rings | side bark]
leaves.png  — 16×16 (cutout transparency)
pickaxe.png — 16×16
shovel.png  — 16×16
axe.png     — 16×16
sword.png   — 16×16
"""

import os, random
from PIL import Image, ImageDraw

OUT = os.path.dirname(os.path.abspath(__file__))
T = 16   # tile size

# ── helpers ────────────────────────────────────────────────────────────────────

def new(cols=1):
    return Image.new("RGBA", (T * cols, T), (0, 0, 0, 0))

def noise(x, y, seed=0, scale=14):
    """Deterministic hash noise in [-scale, +scale]."""
    h = (x * 1313 ^ y * 7177 ^ seed * 9781) & 0xFFFFFFFF
    h ^= h >> 13; h = (h * 0x85EBCA6B) & 0xFFFFFFFF; h ^= h >> 16
    return int(h % (scale * 2 + 1)) - scale

def clamp(v, lo=0, hi=255):
    return max(lo, min(hi, v))

def fill_noise(img, ox, base_rgb, seed, scale=14, alpha=255):
    """Fill a T×T region at x-offset ox with noise-varied base colour."""
    px = img.load()
    r, g, b = base_rgb
    for y in range(T):
        for x in range(T):
            n = noise(x, y, seed, scale)
            px[ox + x, y] = (clamp(r + n), clamp(g + n), clamp(b + n), alpha)

def put_pixel(img, ox, x, y, rgba):
    if 0 <= x < T and 0 <= y < T:
        img.putpixel((ox + x, y), rgba)

def hline(img, ox, x0, x1, y, rgba):
    for x in range(x0, x1 + 1):
        put_pixel(img, ox, x, y, rgba)

def vline(img, ox, x, y0, y1, rgba):
    for y in range(y0, y1 + 1):
        put_pixel(img, ox, x, y, rgba)

def rect(img, ox, x0, y0, x1, y1, rgba):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put_pixel(img, ox, x, y, rgba)

def line(img, ox, x0, y0, x1, y1, rgba):
    """Bresenham line."""
    dx, dy = abs(x1 - x0), abs(y1 - y0)
    sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
    err = dx - dy
    while True:
        put_pixel(img, ox, x0, y0, rgba)
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 > -dy: err -= dy; x0 += sx
        if e2 < dx:  err += dx; y0 += sy

# ── grass.png  (32×16: col0=top, col1=side) ───────────────────────────────────

def make_grass():
    img = new(2)
    px = img.load()

    # col 0 — top: bright green with variation and small dark specks
    for y in range(T):
        for x in range(T):
            n = noise(x, y, 0, 18)
            # base: #7AC048 with brightness noise
            r = clamp(100 + n)
            g = clamp(178 + n * 2)
            b = clamp(58 + n)
            # occasional dark speck (grass blades)
            if noise(x, y, 99, 4) > 3:
                r = clamp(r - 20); g = clamp(g - 20); b = clamp(b - 10)
            px[x, y] = (r, g, b, 255)

    # col 1 — side: green strip on top 3 rows, then blended transition, then dirt
    for y in range(T):
        for x in range(T):
            n = noise(x, y, 1, 14)
            nd = noise(x, y, 11, 14)
            dr = clamp(134 + nd); dg = clamp(96 + nd); db = clamp(57 + nd // 2)
            if y >= 13:        # top 3 rows = grass green
                r = clamp(100 + n); g = clamp(178 + n * 2); b = clamp(58 + n)
                px[T + x, y] = (r, g, b, 255)
            elif y == 12:      # blend row
                t = 0.5
                r = clamp(int((100 + n) * t + dr * (1 - t)))
                g = clamp(int((178 + n) * t + dg * (1 - t)))
                b = clamp(int((58  + n) * t + db * (1 - t)))
                px[T + x, y] = (r, g, b, 255)
            else:              # dirt
                px[T + x, y] = (dr, dg, db, 255)

    img.save(os.path.join(OUT, "grass.png"))
    print("grass.png")

# ── dirt.png  (16×16) ─────────────────────────────────────────────────────────

def make_dirt():
    img = new()
    px = img.load()
    for y in range(T):
        for x in range(T):
            n = noise(x, y, 2, 16)
            # small dark pebble specks
            speck = noise(x, y, 77, 5) > 4
            r = clamp(134 + n - (20 if speck else 0))
            g = clamp( 96 + n - (15 if speck else 0))
            b = clamp( 57 + n // 2)
            px[x, y] = (r, g, b, 255)
    img.save(os.path.join(OUT, "dirt.png"))
    print("dirt.png")

# ── stone.png  (16×16) ────────────────────────────────────────────────────────

def make_stone():
    img = new()
    px = img.load()
    # base gray
    for y in range(T):
        for x in range(T):
            n = noise(x, y, 3, 12)
            v = clamp(128 + n)
            px[x, y] = (v, v, v, 255)
    # crack network
    dk = (82, 82, 82, 255)
    hl = (160, 160, 160, 255)
    # horizontal crack
    for x in range(1, 12): put_pixel(img, 0, x, 6, dk)
    put_pixel(img, 0, 3, 7, dk); put_pixel(img, 0, 6, 5, dk)
    put_pixel(img, 0, 9, 7, dk); put_pixel(img, 0, 11, 5, dk)
    # vertical crack
    for y in range(2, 14): put_pixel(img, 0, 12, y, dk)
    put_pixel(img, 0, 11, 7, dk); put_pixel(img, 0, 13, 4, dk)
    # highlight edge of crack
    for x in range(1, 12): put_pixel(img, 0, x, 5, hl)
    for y in range(2, 14): put_pixel(img, 0, 13, y, hl)
    # corner chips
    ch = (110, 110, 110, 255)
    put_pixel(img, 0, 1, 14, ch); put_pixel(img, 0, 14, 1, ch)
    put_pixel(img, 0, 0, 15, dk); put_pixel(img, 0, 15, 0, dk)
    img.save(os.path.join(OUT, "stone.png"))
    print("stone.png")

# ── wood.png  (32×16: col0=top rings, col1=side bark) ────────────────────────

def make_wood():
    import math
    img = new(2)
    px = img.load()

    # col 0 — top: annual ring pattern
    for y in range(T):
        for x in range(T):
            n = noise(x, y, 40, 8)
            cx, cy = x - 7.5, y - 7.5
            r = math.sqrt(cx*cx + cy*cy)
            ring = int(r + n * 0.15) % 2
            # sapwood (outer) lighter, heartwood (inner) darker
            if r < 2.5:
                base = 95 + n // 2   # dark heartwood
            elif ring == 0:
                base = 130 + n // 2  # light ring
            else:
                base = 108 + n // 2  # dark ring
            px[x, y] = (clamp(base), clamp(base * 75 // 100), clamp(base * 40 // 100), 255)

    # col 1 — side: vertical bark stripes with knot
    for y in range(T):
        for x in range(T):
            n  = noise(x, y, 50, 10)
            ns = noise(x, y * 3, 51, 6)
            stripe = (x + ns // 8) % 4
            base = 118 if stripe < 2 else 96
            base += n // 2
            px[T + x, y] = (clamp(base), clamp(base * 74 // 100), clamp(base * 39 // 100), 255)
    # knot on bark side
    kx, ky = T + 10, 5
    for dx in range(-1, 2):
        for dy in range(-1, 2):
            if abs(dx) + abs(dy) <= 2:
                img.putpixel((kx + dx, ky + dy), (72, 48, 22, 255))
    img.putpixel((kx, ky), (55, 36, 14, 255))

    img.save(os.path.join(OUT, "wood.png"))
    print("wood.png")

# ── leaves.png  (16×16, cutout transparency) ──────────────────────────────────

def make_leaves():
    import math
    img = new()
    px = img.load()

    # Cluster of leaf shapes — semi-random filled blobs with gaps
    for y in range(T):
        for x in range(T):
            n  = noise(x, y, 60, 16)
            ns = noise(x, y, 61, 6)
            # Voronoi-ish: transparent where noise pushes below threshold
            if ns < -2:
                px[x, y] = (0, 0, 0, 0)
                continue
            # Green leaf colour with variation
            r = clamp(48  + n)
            g = clamp(120 + n * 2)
            b = clamp(28  + n)
            # Slightly darker at edges of each "leaf cluster"
            edge = noise(x, y, 62, 4)
            r = clamp(r + edge); g = clamp(g + edge); b = clamp(b + edge // 2)
            px[x, y] = (r, g, b, 255)

    # Ensure border pixels are mostly transparent for natural look
    for i in range(T):
        if noise(i, 0,    63, 4) < 0: img.putpixel((i, 0),    (0, 0, 0, 0))
        if noise(i, T-1,  63, 4) < 0: img.putpixel((i, T-1),  (0, 0, 0, 0))
        if noise(0, i,    63, 4) < 0: img.putpixel((0, i),    (0, 0, 0, 0))
        if noise(T-1, i,  63, 4) < 0: img.putpixel((T-1, i),  (0, 0, 0, 0))

    img.save(os.path.join(OUT, "leaves.png"))
    print("leaves.png")

# ── Tool palette ──────────────────────────────────────────────────────────────
WD  = (162, 107, 47, 255)   # wood
WDK = (110,  68, 22, 255)   # wood dark
IR  = (195, 195, 195, 255)  # iron
IRK = (120, 120, 120, 255)  # iron dark
IRL = (225, 225, 225, 255)  # iron light (highlight)
SL  = (210, 218, 235, 255)  # silver blade
SLK = (155, 162, 185, 255)  # silver dark
SLL = (240, 245, 255, 255)  # silver highlight

# ── pickaxe.png  (16×16) ──────────────────────────────────────────────────────

def make_pickaxe():
    img = new()
    ox = 0
    # iron head — horizontal bar at rows 10-13
    rect(img, ox, 0, 10, 9, 13, IR)
    # two prongs pointing down at left and right
    rect(img, ox, 0,  8, 1, 10, IR)
    rect(img, ox, 8,  8, 9, 10, IR)
    # dark bottom/left edges of head
    hline(img, ox, 0, 9, 10, IRK); hline(img, ox, 0, 9, 13, IRK)
    vline(img, ox, 0, 8, 13, IRK)
    # highlight top of head
    hline(img, ox, 0, 9,  9, IRL)
    # wood handle — diagonal from (5,9) to (14,0) — 2px thick
    line(img, ox, 5, 9, 14, 0, WD)
    line(img, ox, 6, 9, 15, 0, WDK)
    img.save(os.path.join(OUT, "pickaxe.png"))
    print("pickaxe.png")

# ── shovel.png  (16×16) ───────────────────────────────────────────────────────

def make_shovel():
    img = new()
    ox = 0
    # iron blade — wide rectangle at bottom
    rect(img, ox, 4, 10, 11, 15, IR)
    # blade edges
    hline(img, ox, 4, 11, 10, IRK)       # bottom edge of blade
    vline(img, ox, 4, 10, 15, IRK)       # left edge
    vline(img, ox, 11, 10, 15, IRK)      # right edge
    hline(img, ox, 4, 11, 15, IRL)       # top highlight
    # highlight inner left
    vline(img, ox, 5, 10, 14, IRL)
    # neck connector
    put_pixel(img, ox, 7, 9, IRK); put_pixel(img, ox, 8, 9, IR)
    # wood handle 2px wide, rows 1–8
    vline(img, ox, 7, 1, 8, WD)
    vline(img, ox, 8, 1, 8, WDK)
    # end cap
    rect(img, ox, 6, 0, 9, 1, IRK)
    put_pixel(img, ox, 7, 0, WD); put_pixel(img, ox, 8, 0, WD)
    img.save(os.path.join(OUT, "shovel.png"))
    print("shovel.png")

# ── axe.png  (16×16) ──────────────────────────────────────────────────────────

def make_axe():
    img = new()
    ox = 0
    # iron blade — large square in upper-left
    rect(img, ox, 0,  9, 7, 15, IR)
    rect(img, ox, 1,  8, 5, 13, IR)      # rounded lower curve
    # blade edges dark
    hline(img, ox, 0, 7,  9, IRK)
    vline(img, ox, 0, 9, 15, IRK)
    put_pixel(img, ox, 7, 15, IRK)
    # edge highlight
    hline(img, ox, 1, 7,  8, IRL)
    vline(img, ox, 1, 9, 14, IRL)
    # handle diagonal from (6,8) to (14,1) — 2px
    line(img, ox, 6,  8, 14, 1, WD)
    line(img, ox, 7,  8, 15, 1, WDK)
    img.save(os.path.join(OUT, "axe.png"))
    print("axe.png")

# ── sword.png  (16×16) ────────────────────────────────────────────────────────

def make_sword():
    img = new()
    ox = 0
    # silver blade — 2px wide, rows 7-15, pointing down
    vline(img, ox, 7, 7, 15, SL)
    vline(img, ox, 8, 7, 15, SL)
    # blade edges
    vline(img, ox, 9, 7, 14, SLK)         # right dark edge
    vline(img, ox, 6, 7, 14, SLL)         # left highlight
    # tip
    put_pixel(img, ox, 7, 15, SLL); put_pixel(img, ox, 8, 15, SLK)
    # cross-guard: 3px tall, 7px wide at row 5-6
    rect(img, ox, 4, 5, 12, 6, IR)
    hline(img, ox, 4, 12, 5, IRK)         # bottom edge
    hline(img, ox, 4, 12, 4, IRL)         # top highlight
    # wood grip rows 1-4, 2px wide
    vline(img, ox, 7, 1, 4, WD)
    vline(img, ox, 8, 1, 4, WDK)
    # add wrapping lines on grip
    hline(img, ox, 7, 8, 2, WDK); hline(img, ox, 7, 8, 4, WDK)
    # pommel
    rect(img, ox, 6, 0, 9, 1, IRK)
    put_pixel(img, ox, 7, 0, IR); put_pixel(img, ox, 8, 0, IR)
    img.save(os.path.join(OUT, "sword.png"))
    print("sword.png")

# ── run ───────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    make_grass()
    make_dirt()
    make_stone()
    make_wood()
    make_leaves()
    make_pickaxe()
    make_shovel()
    make_axe()
    make_sword()
    print("Done — textures written to", OUT)
