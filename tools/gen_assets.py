#!/usr/bin/env python3
"""Generates every shared asset for the AH: Hell Aisle bake-off.

Deterministic: same seed in, same PNGs out. Run from the repo root:

    python3 tools/gen_assets.py

Writes to assets/. See assets/MANIFEST.md for the resulting contract.
"""

import random
from pathlib import Path

from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parent.parent / "assets"
SEED = 20260713

# Albert Heijn house palette, plus the grime.
AH_BLUE = (0, 160, 226)
AH_DARK = (0, 106, 158)
AH_LIGHT = (170, 224, 247)
WHITE = (245, 245, 245)
OFFWHITE = (222, 222, 216)
GREY = (128, 128, 132)
DARKGREY = (64, 64, 68)
NEARBLACK = (24, 24, 28)
STEEL = (166, 170, 178)
BLOOD = (150, 20, 24)
RED = (208, 40, 40)
GREEN = (60, 170, 70)
YELLOW = (240, 200, 60)
FLESH = (198, 178, 150)
ROT = (140, 160, 120)
BROWN = (120, 84, 52)
NONE = (0, 0, 0, 0)

TILE = 64  # wall / floor / ceiling texture size
ENEMY = 64  # one enemy frame
PICKUP = 32
WEAPON_W, WEAPON_H = 192, 144
FACE_W, FACE_H = 48, 56


def new(w, h, color=NONE):
    return Image.new("RGBA", (w, h), color)


def save(img, name):
    img.save(OUT / name)
    print(f"  {name}  {img.width}x{img.height}")


def strip(frames):
    """Lay frames out left-to-right into one horizontal sheet."""
    w, h = frames[0].size
    sheet = new(w * len(frames), h)
    for i, f in enumerate(frames):
        sheet.paste(f, (i * w, 0), f)
    return sheet


def noise(img, amount=10, alpha_only_where_opaque=True):
    """Per-pixel grain so the flat fills read as texture, not vector art."""
    px = img.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            if alpha_only_where_opaque and a == 0:
                continue
            n = random.randint(-amount, amount)
            px[x, y] = (
                max(0, min(255, r + n)),
                max(0, min(255, g + n)),
                max(0, min(255, b + n)),
                a,
            )
    return img


def shade(color, factor):
    return tuple(max(0, min(255, int(c * factor))) for c in color[:3])


# --------------------------------------------------------------------------
# Walls, floor, ceiling
# --------------------------------------------------------------------------

PRODUCT_COLORS = [
    (200, 60, 50), (240, 180, 40), (60, 130, 200), (80, 170, 90),
    (230, 120, 40), (170, 80, 170), (220, 220, 210), (110, 70, 50),
]


def wall_shelf(stocked=True):
    """Supermarket gondola shelving, seen head-on. The workhorse wall."""
    img = new(TILE, TILE, (196, 198, 200, 255))
    d = ImageDraw.Draw(img)
    # Back panel
    d.rectangle([0, 0, TILE - 1, TILE - 1], fill=(178, 180, 184, 255))
    shelf_ys = [4, 20, 36, 52]
    for sy in shelf_ys:
        if stocked:
            # Row of products standing on the shelf.
            x = 2
            while x < TILE - 4:
                w = random.choice([5, 6, 7])
                h = random.choice([9, 11, 12])
                c = random.choice(PRODUCT_COLORS)
                top = sy + 13 - h
                d.rectangle([x, top, x + w - 1, sy + 12], fill=c + (255,))
                # label band
                d.rectangle([x, top + h // 3, x + w - 1, top + h // 3 + 1],
                            fill=shade(c, 0.6) + (255,))
                x += w + 1
        # The steel shelf lip
        d.rectangle([0, sy + 13, TILE - 1, sy + 14], fill=STEEL + (255,))
        d.rectangle([0, sy + 15, TILE - 1, sy + 15], fill=DARKGREY + (255,))
        # Price rail
        d.rectangle([0, sy + 13, TILE - 1, sy + 13], fill=YELLOW + (255,))
    return noise(img, 6)


def wall_freezer():
    """Glass freezer doors, frosted, lit from within. Used in the cold aisle."""
    img = new(TILE, TILE, (200, 228, 240, 255))
    d = ImageDraw.Draw(img)
    for i, x in enumerate([0, 32]):
        d.rectangle([x, 0, x + 31, TILE - 1], fill=(150, 200, 222, 255))
        d.rectangle([x + 3, 3, x + 28, TILE - 4], fill=(190, 226, 240, 255))
        # frost blooms
        for _ in range(28):
            fx = random.randint(x + 4, x + 27)
            fy = random.randint(4, TILE - 5)
            r = random.randint(1, 3)
            d.ellipse([fx - r, fy - r, fx + r, fy + r], fill=(230, 245, 252, 255))
        # handle
        d.rectangle([x + 26, 22, x + 27, 42], fill=STEEL + (255,))
        # frame
        d.rectangle([x, 0, x + 31, TILE - 1], outline=(230, 232, 235, 255), width=2)
    return noise(img, 5)


def wall_plain():
    """Painted store wall with the AH blue band. Perimeter of the shop."""
    img = new(TILE, TILE, WHITE + (255,))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 24, TILE - 1, 39], fill=AH_BLUE + (255,))
    d.rectangle([0, 22, TILE - 1, 23], fill=AH_LIGHT + (255,))
    d.rectangle([0, 40, TILE - 1, 41], fill=AH_DARK + (255,))
    # scuffs along the skirting
    for _ in range(20):
        x = random.randint(0, TILE - 3)
        y = random.randint(52, TILE - 2)
        d.rectangle([x, y, x + random.randint(1, 3), y + 1],
                    fill=(190, 190, 190, 255))
    return noise(img, 5)


def wall_checkout():
    """The checkout lane, seen as a wall segment: belt, register, divider."""
    img = new(TILE, TILE, (206, 208, 210, 255))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, TILE - 1, 27], fill=(228, 230, 232, 255))  # backboard
    d.rectangle([0, 28, TILE - 1, 43], fill=NEARBLACK + (255,))    # belt
    for x in range(0, TILE, 8):  # belt segments
        d.rectangle([x, 28, x, 43], fill=(58, 58, 62, 255))
    d.rectangle([0, 44, TILE - 1, TILE - 1], fill=(150, 152, 156, 255))
    # register
    d.rectangle([40, 6, 58, 27], fill=(80, 82, 88, 255))
    d.rectangle([42, 9, 56, 19], fill=(120, 220, 140, 255))  # screen
    # AH logo tile
    d.rectangle([4, 6, 20, 22], fill=AH_BLUE + (255,))
    d.rectangle([9, 9, 11, 19], fill=WHITE + (255,))
    d.rectangle([13, 9, 15, 19], fill=WHITE + (255,))
    d.rectangle([9, 13, 15, 15], fill=WHITE + (255,))
    return noise(img, 5)


def wall_magazijn():
    """Corrugated steel: the stockroom / back-of-house."""
    img = new(TILE, TILE, (132, 136, 142, 255))
    d = ImageDraw.Draw(img)
    for x in range(0, TILE, 6):
        d.rectangle([x, 0, x + 2, TILE - 1], fill=(154, 158, 164, 255))
        d.rectangle([x + 3, 0, x + 3, TILE - 1], fill=(104, 108, 114, 255))
    # rust bleeding from the bottom
    for _ in range(24):
        rx = random.randint(0, TILE - 1)
        ry = random.randint(46, TILE - 1)
        d.rectangle([rx, ry, rx + 1, min(TILE - 1, ry + random.randint(1, 6))],
                    fill=(122, 78, 48, 255))
    return noise(img, 7)


def door_keycard():
    """Locked stockroom door. Red reader = you need the bedrijfsleider's pass."""
    img = new(TILE, TILE, (110, 114, 120, 255))
    d = ImageDraw.Draw(img)
    d.rectangle([2, 0, TILE - 3, TILE - 1], fill=(140, 144, 150, 255))
    d.rectangle([2, 0, TILE - 3, TILE - 1], outline=(80, 84, 90, 255), width=2)
    d.rectangle([6, 6, TILE - 7, 30], fill=(120, 124, 130, 255))
    d.rectangle([6, 34, TILE - 7, 57], fill=(120, 124, 130, 255))
    # card reader
    d.rectangle([48, 28, 56, 40], fill=NEARBLACK + (255,))
    d.rectangle([50, 30, 54, 33], fill=RED + (255,))
    # ALLEEN PERSONEEL stripe
    d.rectangle([6, 44, TILE - 7, 50], fill=YELLOW + (255,))
    for x in range(6, TILE - 7, 6):
        d.polygon([(x, 50), (x + 3, 44), (x + 6, 44), (x + 3, 50)],
                  fill=NEARBLACK + (255,))
    return noise(img, 5)


def door_exit():
    """Loading-dock door. Green = the way out. Ends the level."""
    img = new(TILE, TILE, (60, 140, 70, 255))
    d = ImageDraw.Draw(img)
    d.rectangle([2, 0, TILE - 3, TILE - 1], fill=GREEN + (255,))
    d.rectangle([2, 0, TILE - 3, TILE - 1], outline=(30, 100, 40, 255), width=2)
    # NOODUITGANG sign: running figure
    d.rectangle([20, 8, 44, 26], fill=(20, 90, 30, 255))
    d.ellipse([28, 11, 32, 15], fill=WHITE + (255,))
    d.polygon([(30, 16), (34, 22), (30, 22)], fill=WHITE + (255,))
    d.line([(30, 22), (36, 24)], fill=WHITE + (255,), width=2)
    d.line([(30, 22), (25, 24)], fill=WHITE + (255,), width=2)
    # push bar
    d.rectangle([8, 38, TILE - 9, 42], fill=STEEL + (255,))
    return noise(img, 5)


def floor_tiles():
    img = new(TILE, TILE, OFFWHITE + (255,))
    d = ImageDraw.Draw(img)
    for gy in range(2):
        for gx in range(2):
            c = OFFWHITE if (gx + gy) % 2 == 0 else (204, 204, 198)
            d.rectangle([gx * 32, gy * 32, gx * 32 + 30, gy * 32 + 30],
                        fill=c + (255,))
    # grout
    d.rectangle([31, 0, 31, TILE - 1], fill=(160, 160, 155, 255))
    d.rectangle([0, 31, TILE - 1, 31], fill=(160, 160, 155, 255))
    d.rectangle([63, 0, 63, TILE - 1], fill=(160, 160, 155, 255))
    d.rectangle([0, 63, TILE - 1, 63], fill=(160, 160, 155, 255))
    return noise(img, 6)


def ceiling_tiles():
    img = new(TILE, TILE, (208, 208, 205, 255))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, TILE - 1, TILE - 1], outline=(180, 180, 176, 255), width=1)
    d.rectangle([0, 28, TILE - 1, 35], fill=(240, 240, 232, 255))  # strip light
    d.rectangle([0, 28, TILE - 1, 28], fill=(255, 255, 250, 255))
    for _ in range(40):  # tile speckle
        x, y = random.randint(0, TILE - 1), random.randint(0, TILE - 1)
        d.point((x, y), fill=(190, 190, 186, 255))
    return noise(img, 4)


# --------------------------------------------------------------------------
# Enemies — 6-frame strips: walk0, walk1, attack, die0, die1, die2
# --------------------------------------------------------------------------

def winkelwagen_frame(kind, t=0):
    """Rogue shopping trolley. Fast, low HP, bites."""
    img = new(ENEMY, ENEMY)
    d = ImageDraw.Draw(img)
    if kind == "die":
        # Collapses into a heap of bent wire.
        squash = [0, 12, 22][t]
        top = 30 + squash
        d.polygon([(10, top), (54, top), (48, 58), (16, 58)],
                  fill=shade(STEEL, 0.9 - 0.15 * t) + (255,))
        for i in range(4):
            y = top + 4 + i * ((58 - top) // 5 or 1)
            d.line([(12, y), (52, y)], fill=DARKGREY + (255,), width=1)
        if t >= 1:
            d.line([(6, 58), (20, 44)], fill=STEEL + (255,), width=2)   # sprung wire
            d.line([(58, 58), (44, 46)], fill=STEEL + (255,), width=2)
        if t == 2:
            for _ in range(14):  # oil / blood puddle
                px = random.randint(8, 56)
                py = random.randint(54, 62)
                d.point((px, py), fill=BLOOD + (255,))
        return img

    bob = 0 if kind != "walk" else (0 if t == 0 else 1)
    lunge = 3 if kind == "attack" else 0

    # basket (wireframe trapezoid)
    top = 20 + bob - lunge
    d.polygon([(12, top), (52, top), (46, 48 + bob), (18, 48 + bob)],
              fill=(190, 194, 200, 255))
    d.polygon([(12, top), (52, top), (46, 48 + bob), (18, 48 + bob)],
              outline=DARKGREY + (255,))
    for i in range(1, 5):  # wire mesh
        y = top + i * 6
        d.line([(13 + i, y), (51 - i, y)], fill=(150, 154, 160, 255), width=1)
    for i in range(1, 6):
        x = 14 + i * 6
        d.line([(x, top + 1), (x - 1, 47 + bob)], fill=(150, 154, 160, 255), width=1)

    # red handle
    d.rectangle([10, top - 5, 54, top - 2], fill=RED + (255,))

    # maw: the front of the basket is a mouth
    mouth_open = 8 if kind == "attack" else 3
    d.rectangle([20, 42 + bob, 44, 42 + bob + mouth_open],
                fill=NEARBLACK + (255,))
    for tx in range(21, 44, 5):  # teeth
        d.polygon([(tx, 42 + bob), (tx + 2, 42 + bob + mouth_open - 1),
                   (tx + 4, 42 + bob)], fill=WHITE + (255,))
    if kind == "attack":
        for tx in range(23, 44, 5):
            d.polygon([(tx, 42 + bob + mouth_open),
                       (tx + 2, 42 + bob + 1),
                       (tx + 4, 42 + bob + mouth_open)], fill=OFFWHITE + (255,))
        d.point((26, 44 + bob), fill=BLOOD + (255,))

    # eyes glowing in the mesh
    d.ellipse([22, top + 4, 27, top + 9], fill=RED + (255,))
    d.ellipse([37, top + 4, 42, top + 9], fill=RED + (255,))
    d.point((24, top + 6), fill=YELLOW + (255,))
    d.point((39, top + 6), fill=YELLOW + (255,))

    # wheels
    wy = 54 + bob
    for wx in (18, 44):
        d.ellipse([wx - 5, wy - 5, wx + 5, wy + 5], fill=NEARBLACK + (255,))
        d.ellipse([wx - 2, wy - 2, wx + 2, wy + 2], fill=GREY + (255,))
    return img


def vakkenvuller_frame(kind, t=0):
    """Undead night-shift shelf stocker. Lobs soup cans."""
    img = new(ENEMY, ENEMY)
    d = ImageDraw.Draw(img)
    if kind == "die":
        drop = [6, 18, 28][t]
        # crumpling body
        d.rectangle([16, 30 + drop, 48, 58], fill=AH_BLUE + (255,))
        d.ellipse([20 + t * 4, 24 + drop, 36 + t * 4, 40 + drop],
                  fill=shade(ROT, 0.9) + (255,))
        if t >= 1:
            d.rectangle([10, 52, 54, 58], fill=shade(AH_BLUE, 0.7) + (255,))
        if t == 2:
            for _ in range(22):
                px = random.randint(8, 56)
                py = random.randint(52, 62)
                d.point((px, py), fill=BLOOD + (255,))
        return img

    sway = 0 if kind != "walk" else (-1 if t == 0 else 1)
    throw = kind == "attack"

    # legs (dark trousers)
    d.rectangle([24 - sway, 46, 30 - sway, 62], fill=(48, 50, 56, 255))
    d.rectangle([34 + sway, 46, 40 + sway, 62], fill=(48, 50, 56, 255))

    # AH-blue apron / polo
    d.rectangle([20, 26, 44, 48], fill=AH_BLUE + (255,))
    d.rectangle([20, 26, 44, 29], fill=AH_DARK + (255,))
    d.rectangle([27, 34, 30, 44], fill=WHITE + (255,))  # crude 'AH' on the chest
    d.rectangle([33, 34, 36, 44], fill=WHITE + (255,))
    d.rectangle([27, 38, 36, 40], fill=WHITE + (255,))

    # head — rotten
    d.ellipse([24, 8, 40, 26], fill=ROT + (255,))
    d.ellipse([27, 14, 30, 18], fill=NEARBLACK + (255,))  # sunken eyes
    d.ellipse([34, 14, 37, 18], fill=NEARBLACK + (255,))
    d.point((28, 15), fill=RED + (255,))
    d.point((35, 15), fill=RED + (255,))
    d.rectangle([28, 21, 36, 23], fill=NEARBLACK + (255,))  # slack jaw
    d.line([(30, 21), (30, 23)], fill=OFFWHITE + (255,))
    d.line([(34, 21), (34, 23)], fill=OFFWHITE + (255,))

    if throw:
        # arm cocked back above the head with a soup can
        d.line([(44, 30), (52, 16)], fill=ROT + (255,), width=4)
        d.rectangle([48, 6, 58, 16], fill=(200, 60, 50, 255))
        d.rectangle([48, 9, 58, 11], fill=OFFWHITE + (255,))
        d.rectangle([16, 30, 22, 46], fill=ROT + (255,))  # other arm forward
    else:
        d.rectangle([14 + sway, 28, 20 + sway, 46], fill=ROT + (255,))
        d.rectangle([44 - sway, 28, 50 - sway, 46], fill=ROT + (255,))
    return img


def zelfscanner_frame(kind, t=0):
    """Self-checkout terminal. Near-stationary hitscan turret. Shrieks."""
    img = new(ENEMY, ENEMY)
    d = ImageDraw.Draw(img)
    if kind == "die":
        tilt = [4, 14, 26][t]
        d.rectangle([18, 30 + tilt, 46, 60], fill=(90, 94, 100, 255))
        d.rectangle([20, 32 + tilt, 44, 48 + tilt // 2], fill=NEARBLACK + (255,))
        # cracked screen
        d.line([(22, 34 + tilt), (40, 46 + tilt // 2)], fill=WHITE + (255,))
        d.line([(40, 34 + tilt), (26, 46 + tilt // 2)], fill=WHITE + (255,))
        if t >= 1:
            for _ in range(10):  # sparks
                sx = random.randint(20, 44)
                sy = random.randint(30 + tilt, 46 + tilt)
                d.point((sx, sy), fill=YELLOW + (255,))
        if t == 2:
            for _ in range(16):
                px = random.randint(12, 52)
                py = random.randint(54, 62)
                d.point((px, py), fill=(40, 40, 46, 255))
        return img

    bob = 0 if kind != "walk" else (0 if t == 0 else 1)
    firing = kind == "attack"

    # pedestal
    d.rectangle([26, 46 + bob, 38, 60], fill=(110, 114, 120, 255))
    d.ellipse([18, 56, 46, 63], fill=(80, 84, 90, 255))

    # terminal body
    d.rectangle([16, 12 + bob, 48, 48 + bob], fill=(150, 154, 160, 255))
    d.rectangle([16, 12 + bob, 48, 48 + bob], outline=(90, 94, 100, 255), width=2)

    # screen — the face
    scr = RED if firing else (40, 60, 90)
    d.rectangle([20, 16 + bob, 44, 38 + bob], fill=scr + (255,))
    eye = WHITE if firing else AH_LIGHT
    if firing:
        # furious: X eyes, gaping mouth
        d.line([(24, 20 + bob), (30, 26 + bob)], fill=eye + (255,), width=2)
        d.line([(30, 20 + bob), (24, 26 + bob)], fill=eye + (255,), width=2)
        d.line([(34, 20 + bob), (40, 26 + bob)], fill=eye + (255,), width=2)
        d.line([(40, 20 + bob), (34, 26 + bob)], fill=eye + (255,), width=2)
        d.ellipse([27, 29 + bob, 37, 36 + bob], fill=NEARBLACK + (255,))
    else:
        d.rectangle([24, 21 + bob, 29, 25 + bob], fill=eye + (255,))
        d.rectangle([35, 21 + bob, 40, 25 + bob], fill=eye + (255,))
        d.rectangle([26, 32 + bob, 38, 34 + bob], fill=eye + (255,))

    # scanner eye + beam
    d.ellipse([28, 40 + bob, 36, 46 + bob], fill=NEARBLACK + (255,))
    d.ellipse([30, 42 + bob, 34, 45 + bob], fill=RED + (255,))
    if firing:
        d.line([(32, 44 + bob), (32, 63)], fill=(255, 60, 60, 200), width=3)
        d.line([(32, 44 + bob), (32, 63)], fill=(255, 200, 200, 255), width=1)
    return img


def enemy_sheet(fn):
    return strip([
        fn("walk", 0), fn("walk", 1), fn("attack", 0),
        fn("die", 0), fn("die", 1), fn("die", 2),
    ])


# --------------------------------------------------------------------------
# Projectile + pickups
# --------------------------------------------------------------------------

def soepblik():
    """The Vakkenvuller's thrown soup can."""
    img = new(16, 16)
    d = ImageDraw.Draw(img)
    d.rectangle([3, 2, 12, 13], fill=(200, 60, 50, 255))
    d.ellipse([3, 0, 12, 4], fill=STEEL + (255,))
    d.ellipse([3, 11, 12, 15], fill=shade(STEEL, 0.8) + (255,))
    d.rectangle([3, 6, 12, 9], fill=OFFWHITE + (255,))
    d.rectangle([5, 7, 10, 8], fill=(200, 60, 50, 255))
    return img


def pickup_appelflap():
    """+25 health. A warm apple turnover."""
    img = new(PICKUP, PICKUP)
    d = ImageDraw.Draw(img)
    d.polygon([(4, 24), (28, 24), (16, 6)], fill=(196, 148, 82, 255))
    d.polygon([(7, 22), (25, 22), (16, 9)], fill=(220, 176, 108, 255))
    for x in range(8, 26, 4):  # sugar crystals
        d.point((x, 21), fill=WHITE + (255,))
        d.point((x + 1, 19), fill=WHITE + (255,))
    d.line([(6, 24), (26, 24)], fill=(150, 110, 60, 255), width=2)
    return img


def pickup_rookworst():
    """+50 health. Still warm from the rotisserie."""
    img = new(PICKUP, PICKUP)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([3, 12, 29, 22], radius=5, fill=(140, 70, 52, 255))
    d.rounded_rectangle([5, 13, 27, 17], radius=3, fill=(176, 100, 76, 255))
    d.line([(3, 17), (2, 14)], fill=(110, 56, 40, 255), width=2)
    d.line([(29, 17), (30, 20)], fill=(110, 56, 40, 255), width=2)
    for x in range(8, 26, 6):  # steam
        d.point((x, 9), fill=(230, 230, 230, 160))
        d.point((x + 1, 6), fill=(230, 230, 230, 110))
    return img


def pickup_labels():
    """Ammo. A roll of price labels for the Prijspistool."""
    img = new(PICKUP, PICKUP)
    d = ImageDraw.Draw(img)
    d.ellipse([4, 6, 28, 28], fill=YELLOW + (255,))
    d.ellipse([4, 6, 28, 28], outline=(180, 140, 30, 255), width=1)
    d.ellipse([12, 14, 20, 22], fill=(190, 150, 40, 255))
    d.ellipse([14, 16, 18, 20], fill=OFFWHITE + (255,))
    # tail of labels peeling off
    d.rectangle([26, 8, 31, 12], fill=WHITE + (255,))
    d.rectangle([27, 9, 30, 10], fill=RED + (255,))
    for i in range(3):
        d.line([(6 + i * 7, 26), (8 + i * 7, 28)], fill=(180, 140, 30, 255))
    return img


def pickup_bonuskaart():
    """Armour. The card that protects you from full price."""
    img = new(PICKUP, PICKUP)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([3, 9, 29, 24], radius=2, fill=AH_BLUE + (255,))
    d.rounded_rectangle([3, 9, 29, 24], radius=2,
                        outline=AH_DARK + (255,), width=1)
    d.rectangle([5, 12, 27, 14], fill=WHITE + (255,))
    d.rectangle([5, 17, 16, 22], fill=YELLOW + (255,))  # magstripe / chip
    d.rectangle([20, 18, 27, 21], fill=AH_LIGHT + (255,))
    return img


def pickup_keycard():
    """The bedrijfsleider's pass. Opens the magazijn."""
    img = new(PICKUP, PICKUP)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([6, 5, 26, 27], radius=2, fill=(230, 190, 50, 255))
    d.rounded_rectangle([6, 5, 26, 27], radius=2,
                        outline=(160, 120, 20, 255), width=1)
    d.rectangle([9, 8, 23, 16], fill=(250, 230, 150, 255))
    d.ellipse([13, 10, 19, 15], fill=(160, 120, 20, 255))  # tiny mugshot
    d.rectangle([9, 19, 23, 20], fill=(160, 120, 20, 255))
    d.rectangle([9, 22, 18, 23], fill=(160, 120, 20, 255))
    d.rectangle([14, 2, 18, 5], fill=STEEL + (255,))  # lanyard clip
    return img


# --------------------------------------------------------------------------
# Weapons — 3-frame strips: idle, fire/swing A, fire/swing B
# --------------------------------------------------------------------------

def stokbrood_frame(t):
    """Melee. A baguette. Infinite ammo, no dignity."""
    img = new(WEAPON_W, WEAPON_H)
    d = ImageDraw.Draw(img)
    # anchor: bottom-right, swinging up and to the left
    angles = [(150, 140, 60, 40), (120, 130, 20, 20), (100, 138, 8, 70)]
    x0, y0, x1, y1 = angles[t]
    d.line([(x0 + 8, y0 + 6), (x1 + 8, y1 + 6)], fill=(150, 108, 60, 255), width=18)
    d.line([(x0, y0), (x1, y1)], fill=(198, 150, 92, 255), width=16)
    d.line([(x0 - 2, y0 - 2), (x1 - 2, y1 - 2)], fill=(224, 184, 128, 255), width=8)
    # slashes across the crust
    for i in range(1, 5):
        px = x0 + (x1 - x0) * i / 5
        py = y0 + (y1 - y0) * i / 5
        d.line([(px - 4, py - 6), (px + 4, py + 2)],
               fill=(160, 116, 66, 255), width=2)
    # fist
    d.ellipse([x0 - 10, y0 - 6, x0 + 12, y0 + 18], fill=FLESH + (255,))
    if t == 2:  # impact
        for _ in range(18):
            sx = random.randint(x1 - 20, x1 + 20)
            sy = random.randint(y1 - 20, y1 + 12)
            d.point((sx, sy), fill=(235, 205, 160, 255))
    return img


def prijspistool_frame(t):
    """Hitscan. A price-label gun. Staples the price of freedom to their face."""
    img = new(WEAPON_W, WEAPON_H)
    d = ImageDraw.Draw(img)
    kick = [0, 6, 3][t]
    bx, by = 78, 66 + kick

    # body
    d.rounded_rectangle([bx, by, bx + 40, by + 34], radius=3,
                        fill=(70, 74, 82, 255))
    d.rounded_rectangle([bx + 3, by + 3, bx + 37, by + 16], radius=2,
                        fill=AH_BLUE + (255,))
    # label roll on top
    d.ellipse([bx + 8, by - 14, bx + 32, by + 6], fill=YELLOW + (255,))
    d.ellipse([bx + 16, by - 6, bx + 24, by + 2], fill=(190, 150, 40, 255))
    # muzzle / label head
    d.rectangle([bx + 14, by - 24, bx + 26, by - 12], fill=STEEL + (255,))
    # grip + hand
    d.polygon([(bx + 10, by + 34), (bx + 32, by + 34),
               (bx + 28, WEAPON_H), (bx + 6, WEAPON_H)],
              fill=(50, 54, 60, 255))
    d.ellipse([bx + 4, by + 30, bx + 34, WEAPON_H], fill=FLESH + (255,))
    d.ellipse([bx + 10, by + 36, bx + 28, by + 52], fill=(178, 158, 132, 255))

    if t >= 1:  # muzzle flash + ejected label
        d.polygon([(bx + 20, by - 40), (bx + 8, by - 20), (bx + 32, by - 20)],
                  fill=(255, 236, 160, 230))
        d.polygon([(bx + 20, by - 32), (bx + 13, by - 20), (bx + 27, by - 20)],
                  fill=WHITE + (255,))
        d.rectangle([bx + 44, by - 6, bx + 56, by + 2], fill=WHITE + (255,))
        d.rectangle([bx + 46, by - 4, bx + 54, by - 2], fill=RED + (255,))
    return img


# --------------------------------------------------------------------------
# Expansion set (handover 007). Only ever ADD below this line and call the new
# functions at the END of main(): the originals must keep drawing the exact
# same random stream so the frozen competition assets stay byte-identical.
# --------------------------------------------------------------------------

BOSS = 96  # the bedrijfsleider gets a bigger cell; everyone else stays 64


def beveiliger_frame(kind, t=0):
    """Possessed security guard. Advancing single-shot marksman."""
    img = new(ENEMY, ENEMY)
    d = ImageDraw.Draw(img)
    if kind == "die":
        drop = [8, 20, 30][t]
        # keels over sideways, radio still crackling
        body_top = min(32 + drop, 54)
        d.rectangle([14, body_top, 50, 62], fill=(28, 30, 38, 255))
        d.rectangle([14, body_top, 50, body_top + 6], fill=YELLOW + (255,))
        d.ellipse([18 + t * 6, 26 + drop, 34 + t * 6, 42 + drop],
                  fill=shade(ROT, 0.85) + (255,))
        if t >= 1:
            d.rectangle([8, 54, 20, 58], fill=(28, 30, 38, 255))  # dropped cap
        if t == 2:
            for _ in range(18):
                d.point((random.randint(10, 54), random.randint(52, 62)),
                        fill=BLOOD + (255,))
        return img

    sway = 0 if kind != "walk" else (-1 if t == 0 else 1)
    aiming = kind == "attack"

    # legs — black uniform trousers
    d.rectangle([24 - sway, 46, 30 - sway, 62], fill=(24, 26, 32, 255))
    d.rectangle([34 + sway, 46, 40 + sway, 62], fill=(24, 26, 32, 255))

    # torso — dark shirt under a hi-vis vest
    d.rectangle([20, 26, 44, 48], fill=(34, 36, 44, 255))
    d.rectangle([22, 28, 42, 46], fill=(228, 176, 36, 255))       # the vest
    d.rectangle([22, 33, 42, 36], fill=(200, 204, 210, 255))      # reflector band
    d.rectangle([22, 41, 42, 43], fill=(200, 204, 210, 255))

    # head — rotten, peaked cap
    d.ellipse([24, 10, 40, 27], fill=shade(ROT, 0.92) + (255,))
    d.rectangle([22, 8, 42, 14], fill=(24, 26, 32, 255))          # cap
    d.rectangle([24, 14, 40, 15], fill=(60, 64, 74, 255))         # visor
    d.ellipse([27, 17, 30, 20], fill=NEARBLACK + (255,))
    d.ellipse([34, 17, 37, 20], fill=NEARBLACK + (255,))
    d.point((28, 18), fill=RED + (255,))
    d.point((35, 18), fill=RED + (255,))
    d.rectangle([29, 23, 35, 24], fill=NEARBLACK + (255,))

    if aiming:
        # both arms out front, pistol levelled at you
        d.rectangle([28, 30, 58, 34], fill=shade(ROT, 0.9) + (255,))
        d.rectangle([52, 27, 60, 33], fill=(50, 54, 62, 255))     # the sidearm
        d.point((60, 30), fill=RED + (255,))                       # laser diode
    else:
        d.rectangle([14 + sway, 28, 20 + sway, 46], fill=shade(ROT, 0.9) + (255,))
        d.rectangle([44 - sway, 28, 50 - sway, 46], fill=shade(ROT, 0.9) + (255,))
        # shouldered radio
        d.rectangle([20, 27, 24, 33], fill=(50, 54, 62, 255))
        d.point((22, 28), fill=GREEN + (255,))
    return img


def bedrijfsleider_frame(kind, t=0):
    """De Bedrijfsleider. The manager. The level-3 boss. He never closed a shift
    and he is not starting tonight."""
    img = new(BOSS, BOSS)
    d = ImageDraw.Draw(img)
    if kind == "die":
        drop = [12, 30, 46][t]
        # the big man goes down like a pallet of crates
        d.rectangle([20, 44 + drop, 76, 92], fill=(238, 240, 242, 255))
        d.rectangle([20, 44 + drop, 76, 52 + drop], fill=AH_BLUE + (255,))
        d.ellipse([28 + t * 8, 36 + drop, 52 + t * 8, 60 + drop],
                  fill=shade(FLESH, 0.8) + (255,))
        if t >= 1:
            for _ in range(10):  # scattered paperwork
                px = random.randint(14, 82)
                py = random.randint(80, 92)
                d.rectangle([px, py, px + 5, py + 3], fill=WHITE + (255,))
        if t == 2:
            for _ in range(30):
                d.point((random.randint(14, 82), random.randint(78, 94)),
                        fill=BLOOD + (255,))
            # the pass glints where he fell — this is what you came for
            d.rectangle([44, 84, 52, 90], fill=(230, 190, 50, 255))
        return img

    sway = 0 if kind != "walk" else (-2 if t == 0 else 2)
    raging = kind == "attack"

    # legs — grey slacks, wide stance
    d.rectangle([32 - sway, 68, 42 - sway, 94], fill=(70, 72, 80, 255))
    d.rectangle([54 + sway, 68, 64 + sway, 94], fill=(70, 72, 80, 255))

    # torso — white shirt straining, AH-blue tie, rolled sleeves
    d.rectangle([26, 34, 70, 70], fill=(238, 240, 242, 255))
    d.polygon([(48, 36), (44, 52), (48, 66), (52, 52)], fill=AH_BLUE + (255,))
    d.rectangle([26, 34, 70, 38], fill=(216, 218, 222, 255))      # collar shadow
    d.rectangle([40, 40, 46, 46], fill=AH_BLUE + (255,))          # name badge
    d.rectangle([41, 41, 45, 43], fill=WHITE + (255,))

    # head — flushed with rage, manager haircut
    face = (214, 120, 96) if raging else (204, 156, 128)
    d.ellipse([34, 8, 62, 36], fill=face + (255,))
    d.rectangle([34, 8, 62, 14], fill=(88, 66, 48, 255))          # side parting
    d.rectangle([34, 8, 46, 18], fill=(88, 66, 48, 255))
    if raging:
        d.line([(38, 18), (46, 22)], fill=NEARBLACK + (255,), width=2)   # knotted brows
        d.line([(58, 18), (50, 22)], fill=NEARBLACK + (255,), width=2)
        d.ellipse([40, 22, 45, 27], fill=WHITE + (255,))
        d.ellipse([51, 22, 56, 27], fill=WHITE + (255,))
        d.point((42, 24), fill=RED + (255,))
        d.point((53, 24), fill=RED + (255,))
        d.ellipse([42, 29, 54, 35], fill=NEARBLACK + (255,))      # bellowing
    else:
        d.rectangle([40, 22, 45, 25], fill=NEARBLACK + (255,))
        d.rectangle([51, 22, 56, 25], fill=NEARBLACK + (255,))
        d.rectangle([42, 30, 54, 32], fill=NEARBLACK + (255,))

    if raging:
        # one arm hurling stock, the other clenched
        d.rectangle([66, 20, 76, 44], fill=(238, 240, 242, 255))
        d.ellipse([66, 12, 80, 26], fill=face + (255,))
        d.rectangle([70, 4, 82, 16], fill=(200, 60, 50, 255))     # soup, of course
        d.rectangle([70, 8, 82, 10], fill=OFFWHITE + (255,))
        d.rectangle([18, 40, 28, 62], fill=(238, 240, 242, 255))
        d.ellipse([16, 58, 30, 70], fill=face + (255,))
    else:
        d.rectangle([16 + sway, 38, 26 + sway, 64], fill=(238, 240, 242, 255))
        d.rectangle([70 - sway, 38, 80 - sway, 64], fill=(238, 240, 242, 255))
        d.ellipse([16 + sway, 60, 26 + sway, 70], fill=face + (255,))
        d.ellipse([70 - sway, 60, 80 - sway, 70], fill=face + (255,))
    # keycard on a lanyard — the drop you are owed
    d.line([(48, 38), (48, 50)], fill=(160, 120, 20, 255))
    d.rectangle([45, 50, 51, 55], fill=(230, 190, 50, 255))
    return img


def statiegeldkanon_frame(t):
    """Scattergun. A bottle-return intake that fires the deposit back."""
    img = new(WEAPON_W, WEAPON_H)
    d = ImageDraw.Draw(img)
    kick = [0, 8, 4][t]
    bx, by = 70, 62 + kick

    # the intake drum — statiegeld-machine green, ringed
    d.ellipse([bx, by - 26, bx + 56, by + 30], fill=(52, 120, 62, 255))
    d.ellipse([bx + 6, by - 20, bx + 50, by + 24], fill=(70, 150, 80, 255))
    d.ellipse([bx + 14, by - 12, bx + 42, by + 16], fill=NEARBLACK + (255,))  # bore
    for i in range(3):  # bottle throats inside the bore
        ang_x = bx + 20 + i * 8
        d.ellipse([ang_x, by - 6 + (i % 2) * 6, ang_x + 7, by + 1 + (i % 2) * 6],
                  fill=(40, 44, 50, 255))
    # deposit sticker
    d.rectangle([bx + 4, by + 18, bx + 30, by + 26], fill=YELLOW + (255,))
    d.rectangle([bx + 6, by + 20, bx + 28, by + 24], fill=(52, 120, 62, 255))

    # body / hopper it is torn from, and the carrying arm
    d.rectangle([bx + 10, by + 26, bx + 46, WEAPON_H], fill=(60, 64, 72, 255))
    d.ellipse([bx + 16, by + 34, bx + 44, WEAPON_H + 8], fill=FLESH + (255,))

    if t >= 1:
        # a fan of glass and foam out of the bore
        for i in range(7):
            ex = bx + 28 + random.randint(-26, 26)
            ey = by - 34 - random.randint(0, 22)
            d.line([(bx + 28, by - 8), (ex, ey)],
                   fill=(220, 240, 230, 220), width=2)
        d.polygon([(bx + 28, by - 36), (bx + 12, by - 12), (bx + 44, by - 12)],
                  fill=(255, 236, 160, 235))
        # one whole bottle tumbling out
        d.rectangle([bx + 40, by - 44, bx + 46, by - 30], fill=(140, 190, 120, 230))
        d.rectangle([bx + 42, by - 48, bx + 44, by - 44], fill=STEEL + (255,))
    return img


def vuurwerkpijl_frame(t):
    """Rocket launcher. An illegal New Year's vuurwerkpijl, hand-launched."""
    img = new(WEAPON_W, WEAPON_H)
    d = ImageDraw.Draw(img)
    kick = [0, 10, 5][t]
    bx, by = 92, 58 + kick

    # launch tube: a length of drainpipe, held from below
    d.polygon([(bx - 6, by + 30), (bx + 34, by - 26),
               (bx + 46, by - 18), (bx + 6, by + 38)], fill=(120, 124, 132, 255))
    d.polygon([(bx + 34, by - 26), (bx + 46, by - 18), (bx + 43, by - 26),
               (bx + 37, by - 30)], fill=(80, 84, 92, 255))

    if t == 0:
        # the rocket waiting in the tube: red cone, stick out of the back
        d.polygon([(bx + 36, by - 34), (bx + 30, by - 22), (bx + 44, by - 22)],
                  fill=RED + (255,))
        d.rectangle([bx + 33, by - 24, bx + 41, by - 16], fill=(208, 120, 40, 255))
        d.line([(bx - 2, by + 26), (bx + 20, by - 4)], fill=BROWN + (255,), width=3)
    else:
        # gone — flame out of both ends of the pipe
        d.polygon([(bx + 40, by - 22 - 26), (bx + 26, by - 18), (bx + 52, by - 12)],
                  fill=(255, 200, 60, 240))
        d.polygon([(bx + 40, by - 22 - 14), (bx + 32, by - 18), (bx + 47, by - 14)],
                  fill=WHITE + (255,))
        for _ in range(12):  # sparks
            sx = bx + 30 + random.randint(-10, 26)
            sy = by - 44 + random.randint(-16, 20)
            d.point((sx, sy), fill=YELLOW + (255,))
        d.polygon([(bx - 10, by + 36), (bx + 2, by + 26), (bx + 10, by + 40)],
                  fill=(255, 160, 60, 200))

    # hands: one on the pipe, one bracing
    d.ellipse([bx + 2, by + 8, bx + 26, by + 32], fill=FLESH + (255,))
    d.ellipse([bx + 26, by + 30, bx + 50, WEAPON_H + 6], fill=FLESH + (255,))
    return img


def pickup_flessen():
    """Scattergun ammo: a crate of empty deposit bottles."""
    img = new(PICKUP, PICKUP)
    d = ImageDraw.Draw(img)
    d.rectangle([3, 14, 29, 28], fill=(52, 120, 62, 255))
    d.rectangle([3, 14, 29, 28], outline=(30, 84, 40, 255), width=1)
    d.rectangle([5, 19, 27, 21], fill=(30, 84, 40, 255))  # crate slot
    for i, x in enumerate(range(6, 27, 5)):  # necks poking out
        d.rectangle([x, 8 - (i % 2), x + 2, 15], fill=(140, 190, 120, 255))
        d.rectangle([x, 6 - (i % 2), x + 2, 8 - (i % 2)], fill=STEEL + (255,))
    return img


def pickup_vuurwerk():
    """Rocket ammo: a bundle of vuurwerkpijlen wrapped in cellophane."""
    img = new(PICKUP, PICKUP)
    d = ImageDraw.Draw(img)
    for i, x in enumerate([8, 14, 20]):
        top = 6 + (i % 2) * 2
        d.polygon([(x + 2, top), (x, top + 6), (x + 4, top + 6)], fill=RED + (255,))
        d.rectangle([x, top + 6, x + 4, top + 16], fill=(208, 120, 40, 255))
        d.line([(x + 2, top + 16), (x + 2, 28)], fill=BROWN + (255,))
    d.rectangle([6, 18, 26, 20], fill=YELLOW + (255,))  # warning band
    d.point((10, 19), fill=NEARBLACK + (255,))
    d.point((16, 19), fill=NEARBLACK + (255,))
    d.point((22, 19), fill=NEARBLACK + (255,))
    return img


def pickup_statiegeldkanon():
    """Weapon pickup: the statiegeldkanon on the floor."""
    img = new(PICKUP, PICKUP)
    d = ImageDraw.Draw(img)
    d.ellipse([4, 10, 24, 26], fill=(52, 120, 62, 255))
    d.ellipse([8, 13, 20, 23], fill=(70, 150, 80, 255))
    d.ellipse([11, 15, 17, 21], fill=NEARBLACK + (255,))
    d.rectangle([22, 14, 30, 22], fill=(60, 64, 72, 255))  # stub of hopper
    d.rectangle([6, 24, 22, 26], fill=YELLOW + (255,))     # deposit sticker
    return img


def pickup_vuurwerkpijl():
    """Weapon pickup: the launch pipe with a rocket loaded."""
    img = new(PICKUP, PICKUP)
    d = ImageDraw.Draw(img)
    d.polygon([(4, 26), (22, 8), (27, 12), (9, 30)], fill=(120, 124, 132, 255))
    d.polygon([(24, 2), (20, 10), (28, 10)], fill=RED + (255,))
    d.rectangle([22, 10, 26, 14], fill=(208, 120, 40, 255))
    d.line([(6, 24), (16, 12)], fill=BROWN + (255,), width=2)
    return img


def proj_vuurwerkpijl():
    """The rocket in flight, 16x16, billboarded like the soup can."""
    img = new(16, 16)
    d = ImageDraw.Draw(img)
    d.polygon([(12, 2), (9, 7), (15, 7)], fill=RED + (255,))     # cone
    d.rectangle([10, 7, 14, 11], fill=(208, 120, 40, 255))       # motor
    d.line([(11, 11), (5, 15)], fill=BROWN + (255,))             # stick
    d.polygon([(10, 12), (4, 14), (8, 8)], fill=(255, 200, 60, 255))  # exhaust
    d.point((5, 12), fill=WHITE + (255,))
    return img


def boss_sheet(fn):
    return strip([
        fn("walk", 0), fn("walk", 1), fn("attack", 0),
        fn("die", 0), fn("die", 1), fn("die", 2),
    ])


# --------------------------------------------------------------------------
# Multiplayer set (handover 010). Same appendix rule as the 007 block: only
# ever ADD here and call it at the very END of main(), so every earlier asset
# keeps drawing from the same random stream.
# --------------------------------------------------------------------------

def player_klant_frame(kind, t=0):
    """A rival shopper — the other players in the arena. Alive, unlike the
    staff. Drawn in near-whites on purpose: the renderer multiplies each
    player's palette colour over the sprite, so the outfit is the canvas."""
    img = new(ENEMY, ENEMY)
    d = ImageDraw.Draw(img)
    if kind == "die":
        drop = [6, 18, 28][t]
        # folding up over the basket
        d.rectangle([16, 30 + drop, 48, 58], fill=WHITE + (255,))
        d.ellipse([20 + t * 4, 24 + drop, 36 + t * 4, 40 + drop],
                  fill=FLESH + (255,))
        if t >= 1:
            d.rectangle([10, 52, 54, 58], fill=OFFWHITE + (255,))
        if t == 2:
            d.rectangle([6, 54, 16, 61], fill=(70, 74, 82, 255))   # dropped basket
            for _ in range(20):
                d.point((random.randint(8, 56), random.randint(52, 62)),
                        fill=BLOOD + (255,))
        return img

    sway = 0 if kind != "walk" else (-1 if t == 0 else 1)
    aiming = kind == "attack"

    # legs — pale jeans
    d.rectangle([24 - sway, 46, 30 - sway, 62], fill=(196, 200, 208, 255))
    d.rectangle([34 + sway, 46, 40 + sway, 62], fill=(196, 200, 208, 255))

    # the hoodie: the tint's canvas
    d.rectangle([20, 26, 44, 48], fill=WHITE + (255,))
    d.rectangle([20, 26, 44, 30], fill=OFFWHITE + (255,))   # hood bunched at the neck
    d.rectangle([31, 31, 33, 48], fill=OFFWHITE + (255,))   # zip

    # head — a living face and a beanie that takes the tint too
    d.ellipse([24, 8, 40, 26], fill=FLESH + (255,))
    d.rectangle([23, 7, 41, 13], fill=WHITE + (255,))
    d.rectangle([27, 16, 30, 19], fill=NEARBLACK + (255,))
    d.rectangle([34, 16, 37, 19], fill=NEARBLACK + (255,))
    d.rectangle([29, 22, 35, 23], fill=(150, 90, 80, 255))

    if aiming:
        # prijspistool arm out at you; the basket hand never lets go
        d.rectangle([28, 30, 54, 34], fill=FLESH + (255,))
        d.rectangle([50, 26, 60, 34], fill=YELLOW + (255,))
        d.rectangle([57, 29, 60, 31], fill=RED + (255,))
        d.rectangle([14, 28, 20, 44], fill=WHITE + (255,))       # off arm, sleeve
    else:
        d.rectangle([14 + sway, 28, 20 + sway, 46], fill=WHITE + (255,))
        d.rectangle([44 - sway, 28, 50 - sway, 46], fill=WHITE + (255,))

    # the shopping basket, always: they're here to loot the place too
    d.rectangle([8 + sway, 42, 22 + sway, 52], fill=(70, 74, 82, 255))
    d.line([(9 + sway, 44), (21 + sway, 44)], fill=(110, 114, 122, 255))
    d.line([(9 + sway, 47), (21 + sway, 47)], fill=(110, 114, 122, 255))
    d.line([(12 + sway, 42), (14 + sway, 38)], fill=(70, 74, 82, 255), width=2)

    return img


# --------------------------------------------------------------------------
# HUD
# --------------------------------------------------------------------------

def hud_face(state):
    """Doom status-bar mugshot: ok / hurt / dead. Sweatier as it goes."""
    img = new(FACE_W, FACE_H)
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, FACE_W - 1, FACE_H - 1], fill=(40, 42, 48, 255))

    if state == "dead":
        d.ellipse([10, 16, 38, 44], fill=(150, 140, 130, 255))
        d.line([(14, 24), (22, 32)], fill=NEARBLACK + (255,), width=2)
        d.line([(22, 24), (14, 32)], fill=NEARBLACK + (255,), width=2)
        d.line([(26, 24), (34, 32)], fill=NEARBLACK + (255,), width=2)
        d.line([(34, 24), (26, 32)], fill=NEARBLACK + (255,), width=2)
        d.rectangle([18, 37, 30, 39], fill=NEARBLACK + (255,))
        for _ in range(20):
            d.point((random.randint(8, 40), random.randint(40, 54)),
                    fill=BLOOD + (255,))
        return img

    hurt = state == "hurt"
    skin = (190, 150, 130) if hurt else FLESH
    # AH cap
    d.rectangle([8, 8, 40, 18], fill=AH_BLUE + (255,))
    d.rectangle([6, 17, 42, 20], fill=AH_DARK + (255,))
    d.rectangle([20, 10, 22, 16], fill=WHITE + (255,))
    d.rectangle([26, 10, 28, 16], fill=WHITE + (255,))
    d.rectangle([20, 12, 28, 14], fill=WHITE + (255,))
    # face
    d.ellipse([10, 18, 38, 46], fill=skin + (255,))
    d.rectangle([14, 26, 20, 30], fill=WHITE + (255,))
    d.rectangle([28, 26, 34, 30], fill=WHITE + (255,))
    pupil_y = 28 if not hurt else 27
    d.rectangle([16, pupil_y, 18, pupil_y + 2], fill=NEARBLACK + (255,))
    d.rectangle([30, pupil_y, 32, pupil_y + 2], fill=NEARBLACK + (255,))
    if hurt:
        d.line([(13, 24), (21, 22)], fill=NEARBLACK + (255,), width=2)  # scowl
        d.line([(35, 24), (27, 22)], fill=NEARBLACK + (255,), width=2)
        d.arc([16, 32, 32, 44], start=200, end=340, fill=NEARBLACK + (255,), width=2)
        for _ in range(12):  # blood / sweat
            d.point((random.randint(12, 36), random.randint(20, 44)),
                    fill=BLOOD + (255,))
    else:
        d.rectangle([18, 36, 30, 38], fill=NEARBLACK + (255,))
        d.line([(13, 23), (20, 23)], fill=NEARBLACK + (255,), width=1)
        d.line([(28, 23), (35, 23)], fill=NEARBLACK + (255,), width=1)
    # sweat bead
    d.ellipse([37, 22, 40, 27], fill=AH_LIGHT + (255,))
    return img


def hud_panel():
    """320x48 status-bar background. Stretch it across the screen bottom."""
    img = new(320, 48, (52, 54, 60, 255))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, 319, 1], fill=AH_BLUE + (255,))
    d.rectangle([0, 2, 319, 3], fill=AH_DARK + (255,))
    d.rectangle([0, 46, 319, 47], fill=(30, 32, 36, 255))
    # recessed cells for HEALTH / AMMO / ARMOR / KEY
    for x in (8, 88, 168, 248):
        d.rectangle([x, 10, x + 64, 40], fill=(36, 38, 44, 255))
        d.rectangle([x, 10, x + 64, 40], outline=(80, 84, 92, 255), width=1)
    return noise(img, 4)


# --------------------------------------------------------------------------

def main():
    random.seed(SEED)
    OUT.mkdir(exist_ok=True)
    print("walls / floor / ceiling")
    save(wall_shelf(True), "wall_shelf_full.png")
    save(wall_shelf(False), "wall_shelf_empty.png")
    save(wall_freezer(), "wall_freezer.png")
    save(wall_plain(), "wall_plain.png")
    save(wall_checkout(), "wall_checkout.png")
    save(wall_magazijn(), "wall_magazijn.png")
    save(door_keycard(), "door_keycard.png")
    save(door_exit(), "door_exit.png")
    save(floor_tiles(), "floor.png")
    save(ceiling_tiles(), "ceiling.png")

    print("enemies (6-frame strips)")
    save(enemy_sheet(winkelwagen_frame), "enemy_winkelwagen.png")
    save(enemy_sheet(vakkenvuller_frame), "enemy_vakkenvuller.png")
    save(enemy_sheet(zelfscanner_frame), "enemy_zelfscanner.png")
    save(soepblik(), "proj_soepblik.png")

    print("pickups")
    save(pickup_appelflap(), "pickup_appelflap.png")
    save(pickup_rookworst(), "pickup_rookworst.png")
    save(pickup_labels(), "pickup_labels.png")
    save(pickup_bonuskaart(), "pickup_bonuskaart.png")
    save(pickup_keycard(), "pickup_keycard.png")

    print("weapons (3-frame strips)")
    save(strip([stokbrood_frame(i) for i in range(3)]), "weapon_stokbrood.png")
    save(strip([prijspistool_frame(i) for i in range(3)]), "weapon_prijspistool.png")

    print("hud")
    save(strip([hud_face("ok"), hud_face("hurt"), hud_face("dead")]), "hud_face.png")
    save(hud_panel(), "hud_panel.png")

    # Everything below was added by handover 007 and MUST stay below: the frozen
    # originals above consume the random stream first and stay byte-identical.
    print("expansion enemies")
    save(enemy_sheet(beveiliger_frame), "enemy_beveiliger.png")
    save(boss_sheet(bedrijfsleider_frame), "enemy_bedrijfsleider.png")

    print("expansion weapons")
    save(strip([statiegeldkanon_frame(i) for i in range(3)]),
         "weapon_statiegeldkanon.png")
    save(strip([vuurwerkpijl_frame(i) for i in range(3)]),
         "weapon_vuurwerkpijl.png")

    print("expansion pickups + projectile")
    save(pickup_flessen(), "pickup_flessen.png")
    save(pickup_vuurwerk(), "pickup_vuurwerk.png")
    save(pickup_statiegeldkanon(), "pickup_statiegeldkanon.png")
    save(pickup_vuurwerkpijl(), "pickup_vuurwerkpijl.png")
    save(proj_vuurwerkpijl(), "proj_vuurwerkpijl.png")

    # Handover 010 appendix — multiplayer. Stays last, same stream rule.
    print("multiplayer")
    save(enemy_sheet(player_klant_frame), "player_klant.png")
    print("\ndone.")


if __name__ == "__main__":
    main()
