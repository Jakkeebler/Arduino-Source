"""Search a restricted map region for the best template match of a screenshot.
Usage: region_match.py <shot> <tx0> <tx1> <ty0> <ty1>   (tile bounds of region)
"""
import cv2, numpy as np, sys, os
ROOT = r"D:\Programming\Pokemon\Arduino-Source"
MAP  = os.path.join(ROOT, "build-msvc-qt683", "RelWithDebInfo",
                    "Resources", "PokemonFRLG", "Maps", "Kanto-Combined.png")
T = 16
ANCHOR_X, ANCHOR_Y = 120.0, 80.0

shot = cv2.imread(sys.argv[1], cv2.IMREAD_COLOR)
tx0, tx1, ty0, ty1 = map(int, sys.argv[2:6])
gray = cv2.cvtColor(shot, cv2.COLOR_BGR2GRAY)
cols = np.where((gray > 10).any(axis=0))[0]
rows = np.where((gray > 10).any(axis=1))[0]
content = shot[rows[0]:rows[-1]+1, cols[0]:cols[-1]+1]
templ = cv2.resize(content, (15*T, 10*T), interpolation=cv2.INTER_AREA)
m = cv2.imread(MAP, cv2.IMREAD_COLOR)
region = m[ty0*T:(ty1+1)*T, tx0*T:(tx1+1)*T]
if region.shape[0] < templ.shape[0] or region.shape[1] < templ.shape[1]:
    print("region smaller than template"); sys.exit(1)
res = cv2.matchTemplate(region, templ, cv2.TM_CCOEFF_NORMED)
work = res.copy()
print("top 6 peaks inside region x=%d..%d y=%d..%d:" % (tx0, tx1, ty0, ty1))
for i in range(6):
    _, mx, _, loc = cv2.minMaxLoc(work)
    gx = loc[0] + tx0*T
    gy = loc[1] + ty0*T
    ptx = int(round((gx + ANCHOR_X - 8) / T))
    pty = int(round((gy + ANCHOR_Y - 8) / T))
    print("  conf=%.4f  global top-left px=(%d,%d)  -> player tile (%d,%d)" %
          (mx, gx, gy, ptx, pty))
    xa, ya = max(0, loc[0]-32), max(0, loc[1]-32)
    xb, yb = min(work.shape[1], loc[0]+33), min(work.shape[0], loc[1]+33)
    work[ya:yb, xa:xb] = -1.0
