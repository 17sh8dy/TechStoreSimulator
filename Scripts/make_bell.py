"""Synthesizes the shop door bell: a small brass bell struck twice ("ding-ding"). Pure Python, no dependencies.
Writes Art/Audio/shop_bell.wav (mono, 16-bit, 44.1 kHz). Import it into the game with Scripts/import_bell.py.

Run: python -I Scripts/make_bell.py
"""
import math
import os
import random
import struct
import wave

SR = 44100
DUR = 1.5
FUND = 1568.0                        # G6, a small bright bell
PARTIALS = [(1.00, 1.00, 4.5), (2.32, 0.50, 6.5), (4.25, 0.28, 9.0), (6.63, 0.12, 13.0)]   # ratio, amplitude, decay per second
STRIKES = [(0.00, 1.00), (0.17, 0.72)]   # time, strength

random.seed(7)
n = int(SR * DUR)
buf = [0.0] * n
for start, strength in STRIKES:
    s0 = int(start * SR)
    for i in range(s0, n):
        t = (i - s0) / SR
        v = 0.0
        for ratio, amp, decay in PARTIALS:
            v += amp * math.exp(-decay * t) * math.sin(2.0 * math.pi * FUND * ratio * t)
        if t < 0.004:                       # a tiny click as the clapper hits
            v += (random.random() * 2 - 1) * 0.6 * (1 - t / 0.004)
        fade_in = min(1.0, t / 0.002)
        buf[i] += v * fade_in * strength

peak = max(abs(x) for x in buf)
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Art", "Audio")
os.makedirs(out, exist_ok=True)
path = os.path.join(out, "shop_bell.wav")
with wave.open(path, "wb") as w:
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(SR)
    w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, x / peak * 0.8)) * 32767)) for x in buf))
print("wrote", os.path.normpath(path), "%.2f s" % DUR)
