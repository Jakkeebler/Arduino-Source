"""Replay KantoMapDetector's template match against an error-report screenshot.

The fastest way to test a change to the combined Kanto map, or to the detector's
geometry, without running the game: point it at any error report and it tells you
what the detector would have concluded and how confident it was.

It mirrors PokemonFRLG_KantoMapDetector.cpp: crop the letterbox, resize to the
GBA's native 240x160 with INTER_AREA, TM_CCOEFF_NORMED against the map.

    python verify_kanto_map_match.py <Screenshot.png> [--hint X Y] [--radius N]

When a void or a bad hint corrupts a normal match, `--truepos` finds the real
position by correlating only the leftmost N tile-columns for a range of N -- the
columns that fall on real map content. If those agree across widths, that is the
answer, and the disagreement with the full-width match is the size of the problem.

    python verify_kanto_map_match.py <Screenshot.png> --truepos
"""

import argparse
import os
import sys

try:
    import cv2
    import numpy as np
except ImportError:
    sys.exit("this script needs opencv-python and numpy:  pip install opencv-python numpy")

TILE = 16
VIEWPORT_W_TILES = 15
VIEWPORT_H_TILES = 10
PLAYER_TILE_COL = 7
PLAYER_TILE_ROW = 4

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MAP = os.path.join(REPO_ROOT, "Packages", "Resources", "PokemonFRLG", "Maps",
                   "Kanto-Combined.png")


def letterbox_roi(bgr, thresh=24):
    """Crop the black bars, the way the detector does before scaling."""
    gray = cv2.cvtColor(bgr, cv2.COLOR_BGR2GRAY)
    mask = gray > thresh
    cols = np.where(mask.any(axis=0))[0]
    rows = np.where(mask.any(axis=1))[0]
    return int(cols[0]), int(rows[0]), int(cols[-1] - cols[0] + 1), int(rows[-1] - rows[0] + 1)


def build_template(path):
    bgr = cv2.imread(path, cv2.IMREAD_COLOR)
    if bgr is None:
        sys.exit("could not read screenshot: {}".format(path))
    x, y, w, h = letterbox_roi(bgr)
    content = bgr[y:y + h, x:x + w]
    return cv2.resize(content,
                      (VIEWPORT_W_TILES * TILE, VIEWPORT_H_TILES * TILE),
                      interpolation=cv2.INTER_AREA)


def match(mapimg, templ, hint=None, radius=4, label=""):
    if hint is not None:
        ex = (hint[0] - PLAYER_TILE_COL) * TILE
        ey = (hint[1] - PLAYER_TILE_ROW) * TILE
        r = radius * TILE
        x0, y0 = max(0, ex - r), max(0, ey - r)
        x1 = min(mapimg.shape[1], ex + templ.shape[1] + r)
        y1 = min(mapimg.shape[0], ey + templ.shape[0] + r)
        window = mapimg[y0:y1, x0:x1]
    else:
        x0 = y0 = 0
        window = mapimg
    if window.shape[0] < templ.shape[0] or window.shape[1] < templ.shape[1]:
        print("  {:24s} window too small".format(label))
        return None
    res = cv2.matchTemplate(window, templ, cv2.TM_CCOEFF_NORMED)
    _, maxv, _, maxloc = cv2.minMaxLoc(res)
    tx = (maxloc[0] + x0) // TILE + PLAYER_TILE_COL
    ty = (maxloc[1] + y0) // TILE + PLAYER_TILE_ROW
    print("  {:24s} conf={:.3f}  player tile=({},{})".format(label, maxv, tx, ty))
    return maxv, tx, ty


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("screenshot")
    ap.add_argument("--hint", nargs=2, type=int, metavar=("X", "Y"))
    ap.add_argument("--radius", type=int, default=4)
    ap.add_argument("--truepos", action="store_true",
                    help="locate using left-cropped templates (ignores void columns)")
    args = ap.parse_args()

    mapimg = cv2.imread(MAP, cv2.IMREAD_COLOR)
    if mapimg is None:
        sys.exit("map not found: {}".format(MAP))
    templ = build_template(args.screenshot)

    if args.truepos:
        print("Locating with left-cropped templates (excludes any border-fill region):")
        for ncols in range(4, VIEWPORT_W_TILES - 2):
            match(mapimg, templ[:, :ncols * TILE], hint=args.hint, radius=12,
                  label="left {:2d} cols".format(ncols))
        return

    if args.hint:
        match(mapimg, templ, hint=tuple(args.hint), radius=args.radius,
              label="hinted r={}".format(args.radius))
    match(mapimg, templ, hint=None, label="full-map (cold)")


if __name__ == "__main__":
    main()
