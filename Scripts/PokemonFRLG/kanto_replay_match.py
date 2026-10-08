"""Replay KantoMapDetector::locate() on a saved screenshot, offline.

Reproduces the exact pipeline: letterbox crop -> INTER_AREA resize to 240x160
-> TM_CCOEFF_NORMED against the deployed combined map. Reports the top distinct
peaks so a mis-localization can be seen instead of argued about.

Usage: python kanto_replay_match.py <screenshot.png> [tile_x tile_y ...]
Extra tile pairs are candidate player positions to score explicitly.
"""
import cv2, numpy as np, sys, os

ROOT = r"D:\Programming\Pokemon\Arduino-Source"
MAP  = os.path.join(ROOT, "build-msvc-qt683", "RelWithDebInfo",
                    "Resources", "PokemonFRLG", "Maps", "Kanto-Combined.png")
T = 16
VW, VH = 15, 10
#  Same anchors as KantoMapDetector.cpp. Y is 80, not 72: FRLG's camera offsets
#  the tile grid half a tile vertically (calibrated 2026-08-21, see the .cpp).
ANCHOR_X, ANCHOR_Y = 120.0, 80.0
SUPPRESS = 4 * T

shot_path = sys.argv[1]
cands = [(int(sys.argv[i]), int(sys.argv[i+1])) for i in range(2, len(sys.argv)-1, 2)]

shot = cv2.imread(shot_path, cv2.IMREAD_COLOR)
gray = cv2.cvtColor(shot, cv2.COLOR_BGR2GRAY)
thr = 10
cols = np.where((gray > thr).any(axis=0))[0]
rows = np.where((gray > thr).any(axis=1))[0]
x0, x1, y0, y1 = cols[0], cols[-1], rows[0], rows[-1]
content = shot[y0:y1+1, x0:x1+1]
print("letterbox crop: x=%d..%d y=%d..%d  (%dx%d)" % (x0, x1, y0, y1,
      content.shape[1], content.shape[0]))

templ = cv2.resize(content, (VW*T, VH*T), interpolation=cv2.INTER_AREA)
m = cv2.imread(MAP, cv2.IMREAD_COLOR)
print("map: %dx%d px" % (m.shape[1], m.shape[0]))

res = cv2.matchTemplate(m, templ, cv2.TM_CCOEFF_NORMED)

def peak_to_player(px, py):
    #  Round to nearest tile centre, exactly as the C++ does.
    tx = int(round((px + ANCHOR_X - T / 2.0) / T))
    ty = int(round((py + ANCHOR_Y - T / 2.0) / T))
    return tx, ty

work = res.copy()
print("\ntop 8 distinct peaks (suppression radius %d px):" % SUPPRESS)
for i in range(8):
    _, mx, _, loc = cv2.minMaxLoc(work)
    tx, ty = peak_to_player(loc[0], loc[1])
    print("  #%d  conf=%.4f  top-left px=(%d,%d)  -> player tile (%d,%d)" %
          (i+1, mx, loc[0], loc[1], tx, ty))
    xa, ya = max(0, loc[0]-SUPPRESS), max(0, loc[1]-SUPPRESS)
    xb, yb = min(work.shape[1], loc[0]+SUPPRESS+1), min(work.shape[0], loc[1]+SUPPRESS+1)
    work[ya:yb, xa:xb] = -1.0

if cands:
    print("\nexplicit candidate scores:")
    for (tx, ty) in cands:
        ex = int(tx * T + T / 2 - ANCHOR_X)
        ey = int(ty * T + T / 2 - ANCHOR_Y)
        if 0 <= ex < res.shape[1] and 0 <= ey < res.shape[0]:
            print("  player (%3d,%3d): conf=%.4f" % (tx, ty, res[ey, ex]))
        else:
            print("  player (%3d,%3d): outside result" % (tx, ty))
