"""Draws the US notes and coins, offline, with Pillow.

  python Scripts/make_cash.py     -> Art/Packaging/T_Cash_<value>.png, T_Coin_<name>.png (+ _cash_sheet.png)
  then import with Scripts/import_packaging.py (run inside the editor; it picks up T_Cash_* and T_Coin_* too).

These are PROPS FOR A VIDEO GAME, not copies of real currency, and are drawn to stay that way:
  - one side only (the front); there is no back design anywhere,
  - the notes are shown in game at 1.6x real size (never life size), the drawer's little piles at about 0.6x,
  - the portraits are plain stylised silhouettes, the guilloche/ornament is my own pattern, serials are random,
  - "GAME CURRENCY" is printed on every note.
Denominations, colours, names, mottos and sizes follow the real US system so the money reads as real at a glance.
"""
import math
import os
import random
from PIL import Image, ImageDraw, ImageFont

OUT = os.path.join(os.path.dirname(__file__), "..", "Art", "Packaging")
os.makedirs(OUT, exist_ok=True)
FONTS = r"C:/Windows/Fonts/"


def font(size, face="bahnschrift.ttf"):
    for f in (face, "arialbd.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype(FONTS + f, size)
        except Exception:
            continue
    return ImageFont.load_default()


def serif(size, bold=False):
    return font(size, "timesbd.ttf" if bold else "times.ttf")


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


# value, name, paper, ink, accent
NOTES = [
    (1,   "GEORGE WASHINGTON",   (214, 226, 205), (40, 90, 50),   (70, 130, 80)),
    (5,   "ABRAHAM LINCOLN",     (222, 214, 226), (70, 50, 110),  (140, 110, 190)),
    (10,  "ALEXANDER HAMILTON",  (236, 220, 196), (150, 70, 20),  (230, 140, 60)),
    (20,  "ANDREW JACKSON",      (206, 228, 208), (30, 100, 60),  (240, 170, 130)),
    (50,  "ULYSSES S. GRANT",    (232, 208, 216), (140, 40, 80),  (230, 120, 160)),
    (100, "BENJAMIN FRANKLIN",   (208, 222, 238), (30, 70, 140),  (90, 150, 230)),
]

W, H = 1024, 435          # drawn at the true 15.6 : 6.63 shape, then stretched to the 1024 x 512 texture


def guilloche(d, box, ink, seed, rings=14):
    """A rosette of interlocking curves: my own ornament, not a copy of any real security pattern."""
    cx, cy = (box[0] + box[2]) / 2, (box[1] + box[3]) / 2
    rx, ry = (box[2] - box[0]) / 2, (box[3] - box[1]) / 2
    for k in range(rings):
        pts = []
        for i in range(0, 361, 3):
            a = math.radians(i)
            wob = 1 + 0.06 * math.sin(a * (5 + seed % 4) + k * 0.7)
            f = (k + 2) / (rings + 2)
            pts.append((cx + rx * f * wob * math.cos(a), cy + ry * f * wob * math.sin(a)))
        d.line(pts, fill=lerp((255, 255, 255), ink, 0.35), width=1)


def bust(d, cx, cy, h, col):
    """A generic stylised bust in a collar: a silhouette, not a likeness of anyone."""
    head = h * 0.42
    d.ellipse([cx - head * 0.55, cy - h * 0.46, cx + head * 0.55, cy - h * 0.46 + head], fill=col)
    d.polygon([(cx - head * 0.22, cy - h * 0.46 + head * 0.9), (cx + head * 0.22, cy - h * 0.46 + head * 0.9),
               (cx + head * 0.3, cy - h * 0.02), (cx - head * 0.3, cy - h * 0.02)], fill=col)
    d.polygon([(cx - h * 0.42, cy + h * 0.5), (cx - h * 0.3, cy + h * 0.02), (cx - head * 0.3, cy - h * 0.04),
               (cx + head * 0.3, cy - h * 0.04), (cx + h * 0.3, cy + h * 0.02), (cx + h * 0.42, cy + h * 0.5)], fill=col)


def make_note(value, name, paper, ink, accent):
    rnd = random.Random(value * 31)
    img = Image.new("RGB", (W, H), paper)
    d = ImageDraw.Draw(img)
    # wash of colour toward one end
    for x in range(W):
        t = x / W
        d.line([(x, 0), (x, H)], fill=lerp(paper, lerp(paper, accent, 0.45), 0.5 + 0.5 * math.sin(t * math.pi * 2)))
    guilloche(d, (0, 0, W, H), ink, value, rings=18)

    # frame
    d.rectangle([14, 14, W - 15, H - 15], outline=ink, width=5)
    d.rectangle([28, 28, W - 29, H - 29], outline=lerp(paper, ink, 0.55), width=2)
    for i in range(0, W - 60, 12):   # tick border
        d.line([(30 + i, 30), (30 + i, 38)], fill=ink, width=1)
        d.line([(30 + i, H - 39), (30 + i, H - 31)], fill=ink, width=1)

    # titles
    d.text((W // 2, 62), "THE UNITED STATES OF AMERICA", font=serif(34, True), fill=ink, anchor="mm")
    d.text((W // 2, 98), "FEDERAL RESERVE NOTE", font=serif(24), fill=ink, anchor="mm")

    # portrait medallion (centre)
    cx, cy, rw, rh = W // 2, int(H * 0.575), 88, 104
    d.ellipse([cx - rw - 10, cy - rh - 10, cx + rw + 10, cy + rh + 10], fill=lerp(paper, ink, 0.18), outline=ink, width=4)
    d.ellipse([cx - rw, cy - rh, cx + rw, cy + rh], fill=lerp(paper, (255, 255, 255), 0.5), outline=ink, width=2)
    bust(d, cx, cy + 10, rh * 1.5, lerp(ink, paper, 0.12))
    d.text((cx, cy + rh + 24), name, font=serif(20, True), fill=ink, anchor="mm")

    # big numerals in all four corners
    label = str(value)
    for (px, py, anc) in ((70, 80, "mm"), (W - 70, 80, "mm"), (70, H - 80, "mm"), (W - 70, H - 80, "mm")):
        d.ellipse([px - 50, py - 36, px + 50, py + 36], fill=lerp(paper, ink, 0.12), outline=ink, width=3)
        d.text((px, py), label, font=font(52 if len(label) < 3 else 42), fill=ink, anchor=anc)
    # word amount, left and right of the portrait
    words = {1: "ONE", 5: "FIVE", 10: "TEN", 20: "TWENTY", 50: "FIFTY", 100: "ONE HUNDRED"}[value]
    d.text((int(W * 0.2), int(H * 0.5)), words, font=serif(30, True), fill=ink, anchor="mm")
    d.text((int(W * 0.8), int(H * 0.5)), "DOLLARS", font=serif(30, True), fill=ink, anchor="mm")
    d.text((int(W * 0.2), int(H * 0.62)), "IN GOD WE TRUST", font=serif(16), fill=ink, anchor="mm")

    # a generic seal and a random serial number
    sx, sy = int(W * 0.73), int(H * 0.8)
    d.ellipse([sx - 36, sy - 36, sx + 36, sy + 36], outline=accent, width=4)
    for k in range(12):
        a = math.radians(k * 30)
        d.ellipse([sx + 25 * math.cos(a) - 3, sy + 25 * math.sin(a) - 3, sx + 25 * math.cos(a) + 3, sy + 25 * math.sin(a) + 3], fill=accent)
    d.text((sx, sy), "$", font=serif(30, True), fill=accent, anchor="mm")
    serial = "%s%08d%s" % (chr(65 + rnd.randrange(12)), rnd.randrange(10 ** 8), chr(65 + rnd.randrange(26)))
    d.text((int(W * 0.17), int(H * 0.86)), serial, font=font(22), fill=(150, 20, 20), anchor="mm")
    d.text((int(W * 0.5), int(H * 0.935)), "GAME CURRENCY  -  NOT LEGAL TENDER", font=font(14), fill=ink, anchor="mm")
    return img.resize((1024, 512), Image.LANCZOS)


COINS = [
    # name, colour, rim, relief, top text, bottom text
    ("Penny",   (176, 98, 56),   (140, 74, 40),  (206, 128, 80),  "LIBERTY", "ONE CENT"),
    ("Nickel",  (170, 176, 182), (130, 136, 142), (205, 210, 216), "LIBERTY", "FIVE CENTS"),
    ("Dime",    (196, 202, 208), (150, 156, 162), (226, 230, 234), "LIBERTY", "ONE DIME"),
    ("Quarter", (196, 202, 208), (150, 156, 162), (226, 230, 234), "LIBERTY", "QUARTER DOLLAR"),
    ("Dollar",  (210, 172, 70),  (170, 130, 40),  (238, 208, 120), "LIBERTY", "ONE DOLLAR"),
]


def make_coin(name, base, rim, relief, top, bottom):
    S = 512
    img = Image.new("RGB", (S, S), base)
    d = ImageDraw.Draw(img)
    c = S // 2
    d.ellipse([6, 6, S - 7, S - 7], fill=rim)
    d.ellipse([20, 20, S - 21, S - 21], fill=base)
    # reeded edge (not on every coin in life, but reads as "coin")
    for k in range(0, 360, 4):
        a = math.radians(k)
        d.line([(c + (c - 8) * math.cos(a), c + (c - 8) * math.sin(a)), (c + (c - 18) * math.cos(a), c + (c - 18) * math.sin(a))], fill=lerp(rim, base, 0.4), width=2)
    # relief: a profile silhouette
    d.ellipse([c - 60, c - 112, c + 40, c - 4], fill=relief)                 # head
    d.polygon([(c + 30, c - 70), (c + 66, c - 44), (c + 34, c - 34)], fill=relief)   # nose
    d.polygon([(c - 26, c - 16), (c + 22, c - 16), (c + 30, c + 30), (c - 34, c + 30)], fill=relief)   # neck
    d.polygon([(c - 110, c + 130), (c - 70, c + 40), (c + 70, c + 40), (c + 100, c + 130)], fill=relief)   # shoulders
    d.polygon([(c - 24, c + 40), (c, c + 76), (c + 24, c + 40)], fill=base)   # collar
    # lettering around the rim
    def arc_text(text, radius, mid_deg, flip=False):
        f = font(40)
        n = len(text)
        span = n * 7.0
        for i, ch in enumerate(text):
            a = math.radians(mid_deg + (i - (n - 1) / 2) * (6.4 if not flip else -6.4))
            tx = Image.new("RGBA", (60, 60), (0, 0, 0, 0))
            ImageDraw.Draw(tx).text((30, 30), ch, font=f, fill=relief, anchor="mm")
            tx = tx.rotate(-(math.degrees(a) + 90) + (180 if flip else 0), resample=Image.BICUBIC)
            img.paste(tx, (int(c + radius * math.cos(a) - 30), int(c + radius * math.sin(a) - 30)), tx)
    arc_text(top, 205, -90)
    arc_text(bottom, 205, 90, flip=True)
    d.text((c - 150, c + 40), "IN GOD", font=font(26), fill=relief, anchor="mm")
    d.text((c - 150, c + 70), "WE TRUST", font=font(26), fill=relief, anchor="mm")
    d.text((c + 128, c + 62), "2024", font=font(30), fill=relief, anchor="mm")
    return img


if __name__ == "__main__":
    sheet = Image.new("RGB", (6 * 330, 3 * 240), (30, 30, 34))
    for i, n in enumerate(NOTES):
        im = make_note(*n)
        im.save(os.path.join(OUT, "T_Cash_%d.png" % n[0]))
        sheet.paste(im.resize((320, 160)), (i * 330 + 5, 5))
    for i, cn in enumerate(COINS):
        im = make_coin(*cn)
        im.save(os.path.join(OUT, "T_Coin_%s.png" % cn[0]))
        sheet.paste(im.resize((220, 220)), (i * 330 + 5, 250))
    sheet.save(os.path.join(OUT, "_cash_sheet.png"))
    print("wrote", len(NOTES), "notes and", len(COINS), "coins to", os.path.abspath(OUT))
