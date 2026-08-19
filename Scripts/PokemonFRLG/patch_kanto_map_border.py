"""Fill the unrendered border void east of Route 1 in the combined Kanto map.

WHY THIS EXISTS
---------------
`Kanto-Combined.png` is a stitch of several sub-maps. Everything outside them is
left pure white. The game does not draw white there -- it draws the map's border
block, a repeating tree fill -- so any viewport near a sub-map edge is matched
against a template that is partly meaningless.

That is not just a confidence problem. TM_CCOEFF_NORMED prefers the placement
with the *least* blank template, so the void actively biases the reported
position away from the boundary. Observed 2026-08-19: the player stood at
(81,226) hard against the Route 1 tree wall; the detector reported (77,235) at
conf 0.266 on sixteen consecutive polls and the run died. After this fill, the
same frame matches (81,226) at conf 0.777 -- correct tile, cold full-map search.

WHY IT IS A SCRIPT AND NOT A COMMITTED IMAGE
--------------------------------------------
`Packages/` is gitignored and populated separately by the clone_packages build
target, so the patched map cannot be tracked and a package refresh silently
reverts it. Re-run this after any refresh.

The fill motif (`kanto_border_motif_route1.png`) is a 5x10-tile sample of the
border block taken from the game's own output -- an error-report screenshot,
downscaled to the GBA's native 240x160 -- so it is what FRLG actually renders,
not an invention.

SCOPE
-----
Route 1 rows only (y=215..260). Other sub-map boundaries have the same void, but
their border blocks are not evidenced by that frame, and guessing one would
replace honest white with a *confidently wrong* template -- strictly worse,
because white at least scores low. Extend only with a screenshot to derive from.

USAGE
    python patch_kanto_map_border.py [--check]

    --check   report what would change and exit without writing
"""

import argparse
import os
import shutil
import sys

try:
    import cv2
except ImportError:
    sys.exit("this script needs opencv-python:  pip install opencv-python")

TILE = 16

#  Viewport geometry, mirroring PokemonFRLG_KantoMapDetector.cpp.
VIEWPORT_W_TILES = 15
PLAYER_TILE_COL = 7
PLAYER_TILE_ROW = 4

#  Ground truth for the frame the motif came from: the player was at (81,226),
#  established by correlating only the left (valid) columns of that frame, which
#  agreed at conf 0.997 for every crop width from 4 to 10 columns.
SAMPLE_PLAYER_X = 81
SAMPLE_PLAYER_Y = 226
FIRST_VOID_COL = 10          # first viewport column that landed on a void tile

FILL_Y0, FILL_Y1 = 215, 260  # Route 1 rows
FILL_WIDTH_TILES = 25        # far enough east to cover any viewport from the grass

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MAP = os.path.join(REPO_ROOT, "Packages", "Resources", "PokemonFRLG", "Maps",
                   "Kanto-Combined.png")
BACKUP = os.path.join(os.path.dirname(MAP), "Kanto-Combined.unpatched.png")
MOTIF = os.path.join(os.path.dirname(__file__), "kanto_border_motif_route1.png")


def tile_of(img, tx, ty):
    return img[ty * TILE:(ty + 1) * TILE, tx * TILE:(tx + 1) * TILE]


def is_void(img, tx, ty):
    """A tile is unrendered only if every pixel is (near) pure white.

    Deliberately strict: a tile wrongly called void would be overwritten with
    border trees, which is far worse than leaving it alone.
    """
    return bool((tile_of(img, tx, ty) >= 250).all())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="report what would change without writing")
    args = ap.parse_args()

    if not os.path.exists(MAP):
        sys.exit("map not found: {}\n(run the clone_packages build target first)".format(MAP))
    if not os.path.exists(MOTIF):
        sys.exit("border motif not found: {}".format(MOTIF))

    #  Always work from the pristine original so this is idempotent.
    if os.path.exists(BACKUP):
        source = BACKUP
    else:
        source = MAP
        if not args.check:
            shutil.copy2(MAP, BACKUP)
            print("preserved pristine map -> {}".format(os.path.basename(BACKUP)))

    mapimg = cv2.imread(source, cv2.IMREAD_COLOR)
    motif = cv2.imread(MOTIF, cv2.IMREAD_COLOR)
    if mapimg is None or motif is None:
        sys.exit("failed to read the map or the motif")

    width_tiles = mapimg.shape[1] // TILE
    motif_w = motif.shape[1] // TILE
    motif_h = motif.shape[0] // TILE

    anchor_x = SAMPLE_PLAYER_X - PLAYER_TILE_COL + FIRST_VOID_COL
    anchor_y = SAMPLE_PLAYER_Y - PLAYER_TILE_ROW
    fill_x1 = min(width_tiles - 1, anchor_x + FILL_WIDTH_TILES - 1)

    filled = untouched = 0
    for ty in range(FILL_Y0, FILL_Y1 + 1):
        for tx in range(anchor_x, fill_x1 + 1):
            if not is_void(mapimg, tx, ty):
                untouched += 1
                continue
            filled += 1
            if args.check:
                continue
            sx = (tx - anchor_x) % motif_w
            sy = (ty - anchor_y) % motif_h
            mapimg[ty * TILE:(ty + 1) * TILE, tx * TILE:(tx + 1) * TILE] = \
                tile_of(motif, sx, sy)

    print("region x={}..{}  y={}..{}".format(anchor_x, fill_x1, FILL_Y0, FILL_Y1))
    print("  {} void tiles {}, {} rendered tiles left untouched".format(
        filled, "would be filled" if args.check else "filled", untouched))

    if args.check:
        return

    #  OpenCV's default PNG compression turns this 2.8 MB file into 13 MB.
    cv2.imwrite(MAP, mapimg, [cv2.IMWRITE_PNG_COMPRESSION, 9])
    print("wrote {} ({:,} bytes)".format(MAP, os.path.getsize(MAP)))
    print("restart SerialPrograms to pick it up (the map is loaded once at first use)")


if __name__ == "__main__":
    main()
