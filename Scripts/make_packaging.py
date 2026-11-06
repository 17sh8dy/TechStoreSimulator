"""Draws the retail packaging art for every product, offline, with Pillow.

  python Scripts/make_packaging.py        -> Art/Packaging/T_Pack_<Name>.png  (+ a contact sheet)
  then import them with Scripts/import_packaging.py (run inside the editor).

All brands, models and logos here are INVENTED. Nothing resembles a real company's trademark:
LUMIO (phones/tablets), ZENTO (computers/displays), VEXA (audio), KYRA (gaming/wearables), TERNO (accessories/speakers).
Each image is drawn with the same aspect ratio as the box face it wraps, then resized to a square so UVs 0..1 do not stretch it.
"""
import os
from PIL import Image, ImageDraw, ImageFont

OUT = os.path.join(os.path.dirname(__file__), "..", "Art", "Packaging")
os.makedirs(OUT, exist_ok=True)
FONTS = r"C:/Windows/Fonts/"
def font(size, bold=True, face="bahnschrift.ttf"):
    try:
        return ImageFont.truetype(FONTS + (face if bold else "arial.ttf"), size)
    except Exception:
        return ImageFont.load_default()

S = 40  # drawing pixels per centimetre of box face

BRANDS = {
    "LUMIO": dict(c1=(24, 28, 70), c2=(60, 70, 190), accent=(120, 200, 255), mark="circle"),
    "ZENTO": dict(c1=(20, 40, 44), c2=(30, 120, 120), accent=(120, 255, 220), mark="hex"),
    "VEXA":  dict(c1=(14, 14, 16), c2=(60, 40, 20), accent=(255, 150, 40), mark="chevron"),
    "KYRA":  dict(c1=(26, 12, 56), c2=(120, 40, 160), accent=(255, 90, 220), mark="diamond"),
    "TERNO": dict(c1=(16, 40, 24), c2=(40, 130, 70), accent=(190, 255, 120), mark="tri"),
}

# name, brand, model, face W x H (cm), specs, silhouette
PRODUCTS = [
    ("Phone",      "LUMIO", "L12",       9.0, 17.0, ["6.1\" OLED", "128 GB", "5G"],         "phone"),
    ("Laptop",     "ZENTO", "Slim 14",  36.0, 25.0, ["14\" display", "16 GB RAM", "512 GB SSD"], "laptop"),
    ("Headphones", "VEXA",  "H300",     19.0, 22.0, ["Noise cancelling", "40 h battery"],   "headphones"),
    ("Console",    "KYRA",  "Ember X",  36.0, 28.0, ["4K gaming", "1 TB", "2 controllers"], "console"),
    ("Monitor",    "ZENTO", "V27",      46.0, 34.0, ["27\" QHD", "144 Hz", "IPS"],          "monitor"),
    ("Keyboard",   "TERNO", "K75",      44.0, 15.0, ["Mechanical", "RGB", "Wireless"],      "keyboard"),
    ("Tablet",     "LUMIO", "Tab 10",   22.0, 28.0, ["10.5\" display", "64 GB", "Stylus"],  "tablet"),
    ("Earbuds",    "VEXA",  "Buds S",    7.0, 11.0, ["True wireless", "24 h"],              "earbuds"),
    ("Smartwatch", "KYRA",  "Watch 2",  10.0, 12.0, ["AMOLED", "GPS", "7 day battery"],     "watch"),
    ("Speaker",    "TERNO", "Boom 2",   14.0, 22.0, ["Waterproof", "360 sound", "20 h"],    "speaker"),
]

def lerp(a, b, t): return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))

def gradient(w, h, top, bottom):
    img = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(img)
    for y in range(h):
        d.line([(0, y), (w, y)], fill=lerp(top, bottom, y / max(1, h - 1)))
    return img

def logo(d, brand, x, y, r, col):
    kind = BRANDS[brand]["mark"]
    if kind == "circle":
        d.ellipse([x - r, y - r, x + r, y + r], outline=col, width=max(2, r // 4))
        d.ellipse([x - r // 3, y - r // 3, x + r // 3, y + r // 3], fill=col)
    elif kind == "hex":
        import math
        pts = [(x + r * math.cos(math.radians(60 * i + 30)), y + r * math.sin(math.radians(60 * i + 30))) for i in range(6)]
        d.polygon(pts, outline=col, width=max(2, r // 4))
    elif kind == "chevron":
        d.polygon([(x - r, y - r), (x, y + r), (x + r, y - r), (x + r * 0.5, y - r), (x, y + r * 0.35), (x - r * 0.5, y - r)], fill=col)
    elif kind == "diamond":
        d.polygon([(x, y - r), (x + r, y), (x, y + r), (x - r, y)], outline=col, width=max(2, r // 4))
        d.polygon([(x, y - r // 2), (x + r // 2, y), (x, y + r // 2), (x - r // 2, y)], fill=col)
    else:
        d.polygon([(x, y - r), (x + r, y + r), (x - r, y + r)], fill=col)

def barcode(d, x, y, w, h, col=(235, 235, 235)):
    import random
    rnd = random.Random(int(x * 7 + y))
    d.rectangle([x, y, x + w, y + h], fill=(250, 250, 250))
    cx = x + 4
    while cx < x + w - 4:
        bw = rnd.choice([1, 2, 3, 4])
        d.rectangle([cx, y + 3, cx + bw - 1, y + h - 3], fill=(20, 20, 20))
        cx += bw + rnd.choice([1, 2, 3])

# ---- silhouettes: drawn in a box (cx, cy, size) with an accent colour --------------------------------
def rr(d, box, r, **kw):
    d.rounded_rectangle(box, radius=r, **kw)

def sil_phone(d, cx, cy, s, acc):
    w, h = s * 0.5, s
    rr(d, [cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2], w * 0.14, fill=(12, 12, 18), outline=(200, 200, 215), width=4)
    rr(d, [cx - w / 2 + 8, cy - h / 2 + 8, cx + w / 2 - 8, cy + h / 2 - 8], w * 0.1, fill=lerp((30, 60, 160), (140, 60, 200), 0.5))
    d.ellipse([cx - w * 0.07, cy - h / 2 + 14, cx + w * 0.07, cy - h / 2 + 14 + w * 0.14], fill=(10, 10, 14))
    for i in range(3):
        d.ellipse([cx - w * 0.3 + i * w * 0.07, cy + h * 0.12, cx - w * 0.3 + i * w * 0.07 + w * 0.06, cy + h * 0.12 + w * 0.06], fill=acc)

def sil_laptop(d, cx, cy, s, acc):
    w = s
    lid_h = w * 0.55
    d.polygon([(cx - w * 0.42, cy - lid_h * 0.55), (cx + w * 0.42, cy - lid_h * 0.55), (cx + w * 0.46, cy + lid_h * 0.2), (cx - w * 0.46, cy + lid_h * 0.2)], fill=(200, 205, 215))
    d.polygon([(cx - w * 0.39, cy - lid_h * 0.5), (cx + w * 0.39, cy - lid_h * 0.5), (cx + w * 0.42, cy + lid_h * 0.15), (cx - w * 0.42, cy + lid_h * 0.15)], fill=lerp((20, 90, 110), (90, 200, 190), 0.5))
    d.polygon([(cx - w * 0.5, cy + lid_h * 0.25), (cx + w * 0.5, cy + lid_h * 0.25), (cx + w * 0.44, cy + lid_h * 0.36), (cx - w * 0.44, cy + lid_h * 0.36)], fill=(170, 175, 188))
    d.rectangle([cx - w * 0.08, cy + lid_h * 0.27, cx + w * 0.08, cy + lid_h * 0.3], fill=(120, 125, 140))

def sil_headphones(d, cx, cy, s, acc):
    r = s * 0.42
    d.arc([cx - r, cy - r * 1.05, cx + r, cy + r * 0.95], 190, 350, fill=(230, 230, 235), width=int(s * 0.07))
    for sx in (-1, 1):
        rr(d, [cx + sx * r - s * 0.13, cy - s * 0.02, cx + sx * r + s * 0.13, cy + s * 0.36], s * 0.1, fill=(25, 25, 30), outline=acc, width=4)
        d.ellipse([cx + sx * r - s * 0.06, cy + s * 0.1, cx + sx * r + s * 0.06, cy + s * 0.22], fill=acc)

def sil_console(d, cx, cy, s, acc):
    w, h = s * 0.95, s * 0.32
    rr(d, [cx - w / 2, cy - h / 2 - s * 0.18, cx + w / 2, cy + h / 2 - s * 0.18], h * 0.25, fill=(235, 235, 240))
    d.rectangle([cx - w * 0.38, cy - s * 0.28, cx + w * 0.38, cy - s * 0.24], fill=(40, 40, 50))
    d.ellipse([cx + w * 0.3, cy - s * 0.24, cx + w * 0.36, cy - s * 0.2], fill=acc)
    # controller
    rr(d, [cx - s * 0.28, cy + s * 0.12, cx + s * 0.28, cy + s * 0.34], s * 0.09, fill=(30, 30, 40), outline=(200, 200, 215), width=3)
    d.ellipse([cx - s * 0.2, cy + s * 0.18, cx - s * 0.12, cy + s * 0.26], fill=(90, 90, 110))
    d.ellipse([cx + s * 0.12, cy + s * 0.18, cx + s * 0.2, cy + s * 0.26], fill=acc)

def sil_monitor(d, cx, cy, s, acc):
    w, h = s * 0.95, s * 0.58
    rr(d, [cx - w / 2, cy - h / 2 - s * 0.1, cx + w / 2, cy + h / 2 - s * 0.1], 10, fill=(20, 20, 26), outline=(190, 195, 205), width=5)
    d.rectangle([cx - w / 2 + 12, cy - h / 2 - s * 0.1 + 12, cx + w / 2 - 12, cy + h / 2 - s * 0.1 - 12], fill=lerp((20, 70, 120), (200, 90, 160), 0.45))
    d.polygon([(cx - s * 0.04, cy + h / 2 - s * 0.1), (cx + s * 0.04, cy + h / 2 - s * 0.1), (cx + s * 0.08, cy + h / 2 + s * 0.12), (cx - s * 0.08, cy + h / 2 + s * 0.12)], fill=(150, 155, 165))
    rr(d, [cx - s * 0.2, cy + h / 2 + s * 0.12, cx + s * 0.2, cy + h / 2 + s * 0.16], 5, fill=(170, 175, 185))

def sil_keyboard(d, cx, cy, s, acc):
    w, h = s * 1.0, s * 0.34
    rr(d, [cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2], 10, fill=(30, 30, 36), outline=(150, 150, 165), width=3)
    rows, cols = 4, 14
    kw = (w - 30) / cols
    kh = (h - 24) / rows
    for r_ in range(rows):
        for c_ in range(cols):
            if r_ == 3 and 3 < c_ < 10:
                continue
            col = acc if (r_ == 0 and c_ in (0, 13)) else (205, 205, 215)
            d.rectangle([cx - w / 2 + 15 + c_ * kw + 1, cy - h / 2 + 12 + r_ * kh + 1, cx - w / 2 + 15 + (c_ + 1) * kw - 2, cy - h / 2 + 12 + (r_ + 1) * kh - 2], fill=col)
    d.rectangle([cx - w * 0.2, cy - h / 2 + 12 + 3 * kh + 1, cx + w * 0.2, cy - h / 2 + 12 + 4 * kh - 2], fill=(205, 205, 215))

def sil_tablet(d, cx, cy, s, acc):
    w, h = s * 0.62, s * 0.85
    rr(d, [cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2], w * 0.07, fill=(18, 18, 24), outline=(205, 205, 220), width=4)
    rr(d, [cx - w / 2 + 12, cy - h / 2 + 12, cx + w / 2 - 12, cy + h / 2 - 12], w * 0.04, fill=lerp((40, 80, 190), (150, 80, 210), 0.5))
    rr(d, [cx + w / 2 + 6, cy - h * 0.3, cx + w / 2 + 14, cy + h * 0.35], 4, fill=(220, 220, 230))

def sil_earbuds(d, cx, cy, s, acc):
    rr(d, [cx - s * 0.32, cy + s * 0.02, cx + s * 0.32, cy + s * 0.42], s * 0.14, fill=(235, 235, 240), outline=(170, 170, 185), width=3)
    d.ellipse([cx - s * 0.04, cy + s * 0.2, cx + s * 0.04, cy + s * 0.28], fill=acc)
    for sx in (-1, 1):
        d.ellipse([cx + sx * s * 0.2 - s * 0.1, cy - s * 0.34, cx + sx * s * 0.2 + s * 0.1, cy - s * 0.12], fill=(240, 240, 245), outline=(160, 160, 175), width=3)
        rr(d, [cx + sx * s * 0.2 - s * 0.03, cy - s * 0.14, cx + sx * s * 0.2 + s * 0.03, cy + s * 0.0], 5, fill=(240, 240, 245), outline=(160, 160, 175), width=2)

def sil_watch(d, cx, cy, s, acc):
    rr(d, [cx - s * 0.16, cy - s * 0.5, cx + s * 0.16, cy - s * 0.2], 8, fill=(40, 40, 52))
    rr(d, [cx - s * 0.16, cy + s * 0.2, cx + s * 0.16, cy + s * 0.5], 8, fill=(40, 40, 52))
    rr(d, [cx - s * 0.27, cy - s * 0.27, cx + s * 0.27, cy + s * 0.27], s * 0.1, fill=(25, 25, 32), outline=(205, 205, 220), width=4)
    d.ellipse([cx - s * 0.2, cy - s * 0.2, cx + s * 0.2, cy + s * 0.2], fill=lerp((30, 20, 70), (160, 60, 190), 0.5))
    d.arc([cx - s * 0.15, cy - s * 0.15, cx + s * 0.15, cy + s * 0.15], 200, 330, fill=acc, width=5)

def sil_speaker(d, cx, cy, s, acc):
    w, h = s * 0.55, s * 0.8
    rr(d, [cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2], w * 0.35, fill=(24, 30, 26), outline=(190, 200, 190), width=4)
    for yy in range(-4, 5):
        for xx in range(-2, 3):
            d.ellipse([cx + xx * w * 0.14 - 3, cy + yy * h * 0.075 - 3, cx + xx * w * 0.14 + 3, cy + yy * h * 0.075 + 3], fill=(90, 100, 92))
    rr(d, [cx - w * 0.3, cy + h * 0.33, cx + w * 0.3, cy + h * 0.39], 5, fill=acc)

SIL = dict(phone=sil_phone, laptop=sil_laptop, headphones=sil_headphones, console=sil_console, monitor=sil_monitor,
           keyboard=sil_keyboard, tablet=sil_tablet, earbuds=sil_earbuds, watch=sil_watch, speaker=sil_speaker)

def make(name, brand, model, wcm, hcm, specs, sil):
    W, H = int(wcm * S), int(hcm * S)
    B = BRANDS[brand]
    img = gradient(W, H, B["c1"], B["c2"])
    d = ImageDraw.Draw(img)
    u = min(W, H)                       # a unit that scales with the face
    pad = int(u * 0.06)
    wide = W / H > 2.2                  # keyboard-style boxes get a horizontal layout

    r = int(u * 0.045) if not wide else int(u * 0.09)
    logo(d, brand, pad + r, pad + r, r, B["accent"])
    d.text((pad + r * 2 + int(u * 0.03), pad + r), brand, font=font(int(u * (0.075 if not wide else 0.13))), fill=(245, 245, 250), anchor="lm")

    # barcode: always top-right, out of the way of everything else
    bw, bh = int(u * (0.28 if not wide else 0.34)), int(u * (0.09 if not wide else 0.16))
    barcode(d, W - bw - pad, pad, bw, bh)

    if not wide:
        cx, cy = W // 2, int(H * 0.47)
        for i in range(6):
            gr = int(u * (0.50 - i * 0.05))
            d.ellipse([cx - gr, cy - gr, cx + gr, cy + gr], outline=lerp(B["c2"], B["accent"], 0.08 + i * 0.03), width=2)
        SIL[sil](d, cx, cy, u * 0.72, B["accent"])
        d.text((W // 2, int(H * 0.78)), model, font=font(int(u * 0.12)), fill=(255, 255, 255), anchor="mm")
        # spec chips, shrunk until they fit the box face
        fs = int(u * 0.045)
        while True:
            total = sum(d.textlength(sp, font=font(fs)) + fs * 1.4 for sp in specs) + (len(specs) - 1) * fs * 0.4
            if total <= W * 0.92 or fs < 10:
                break
            fs -= 1
        chip_y = int(H * 0.88)
        chip_h = int(fs * 1.7)
        x = (W - total) / 2
        for sp in specs:
            tw = d.textlength(sp, font=font(fs))
            rr(d, [x, chip_y - chip_h / 2, x + tw + fs * 1.2, chip_y + chip_h / 2], chip_h / 2, outline=B["accent"], width=2)
            d.text((x + fs * 0.6, chip_y), sp, font=font(fs), fill=(240, 240, 245), anchor="lm")
            x += tw + fs * 1.6
    else:
        # wide: silhouette in the centre, model and specs to the right of it
        SIL[sil](d, int(W * 0.40), int(H * 0.54), H * 1.35, B["accent"])
        d.text((int(W * 0.80), int(H * 0.42)), model, font=font(int(H * 0.28)), fill=(255, 255, 255), anchor="mm")
        for i, sp in enumerate(specs):
            d.text((int(W * 0.80), int(H * 0.62 + i * H * 0.13)), sp, font=font(int(H * 0.09)), fill=B["accent"], anchor="mm")
    d.text((pad, H - pad), "DEMO PRODUCT - NOT REAL", font=font(int(u * 0.03), bold=False), fill=(200, 200, 210), anchor="ls")
    img = img.resize((512, 512), Image.LANCZOS)
    img.save(os.path.join(OUT, "T_Pack_%s.png" % name))
    return img

if __name__ == "__main__":
    sheet = Image.new("RGB", (5 * 260, 2 * 260), (30, 30, 34))
    for i, p in enumerate(PRODUCTS):
        im = make(*p).resize((250, 250))
        sheet.paste(im, ((i % 5) * 260 + 5, (i // 5) * 260 + 5))
    sheet.save(os.path.join(OUT, "_contact_sheet.png"))
    print("wrote", len(PRODUCTS), "textures to", os.path.abspath(OUT))
