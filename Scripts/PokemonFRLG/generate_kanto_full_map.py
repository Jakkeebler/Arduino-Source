"""Generate the full Kanto combined map + masks + ledges + goals + regions.

From: https://github.com/PokemonAutomation/

Unlike the old visual-classification pipeline, this builds everything directly
from a local checkout of the pokefirered decompilation (https://github.com/
pret/pokefirered), which has every map's real tile graphics, collision bits,
metatile behavior (for ledges), warp events (for door goals) and heal
locations (for fly goals) as ground truth. Nothing here is guessed from pixels.

Usage:
    git clone --depth 1 https://github.com/pret/pokefirered.git /tmp/pokefirered
    POKEFIRERED_PATH=/tmp/pokefirered python3 Scripts/PokemonFRLG/generate_kanto_full_map.py

Requires: Pillow (`pip install pillow`).

What it does:
  1. Loads every outdoor Kanto map's layout (data/layouts/layouts.json) and
     connection graph (data/maps/*/map.json "connections"), and BFS-places
     every one from Pallet Town outward using the game's own connection
     offsets -- this is exact, not a visual alignment guess.
  2. Renders each map's pixels from its tileset (tiles.png + metatiles.bin +
     palettes/*.pal) and its real per-tile collision bit + metatile behavior
     byte (map.bin + metatile_attributes.bin).
  3. Packs a curated set of interior/dungeon maps (Viridian Forest, Mt. Moon,
     Diglett's Cave, Rock Tunnel, the Underground Paths, Cerulean Cave, the
     walled-off Saffron City) into the unused void space of the canvas, the
     same way the original hand-built map packed Viridian Forest into the
     bottom-right corner.
  4. Writes:
       - Packages/Resources/PokemonFRLG/Maps/Kanto-Combined.png
       - SerialPrograms/.../PokemonFRLG_KantoMapMasks_Generated.h   (ground-truth walkable mask)
       - SerialPrograms/.../PokemonFRLG_KantoLedges_Generated.h     (MB_JUMP_SOUTH ledges)
       - SerialPrograms/.../PokemonFRLG_KantoMaskCorrections_Generated.h (now empty -- ground truth)
       - SerialPrograms/.../PokemonFRLG_KantoGoals_Extended.h       (every door + fly goal)
       - SerialPrograms/.../PokemonFRLG_KantoRegions_Generated.h    (log-label region table)

Re-run after changing EXTRA_INTERIOR_MAPS or after a pokefirered update.
"""

import json, os, re, struct, sys, collections
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
MAP_DIR = os.path.join(ROOT, 'SerialPrograms', 'Source', 'PokemonFRLG', 'Inference', 'Map')
PNG_OUT = os.path.join(ROOT, 'Packages', 'Resources', 'PokemonFRLG', 'Maps', 'Kanto-Combined.png')

POKEFIRERED: str = os.environ.get('POKEFIRERED_PATH') or ''
if not POKEFIRERED or not os.path.isdir(POKEFIRERED):
    print('Set POKEFIRERED_PATH to a local pokefirered decomp checkout.')
    sys.exit(1)

MB_JUMP_SOUTH = 0x3B

# Extra interior/dungeon maps packed into void space alongside the outdoor
# overworld. Extend this list to bring in more dungeons (S.S. Anne, Pokemon
# Tower, Silph Co., Pokemon Mansion, Seafoam Islands, Victory Road, ...).
EXTRA_INTERIOR_MAPS = [
    'ViridianForest',
    'MtMoon_1F', 'MtMoon_B1F', 'MtMoon_B2F',
    'DiglettsCave_NorthEntrance', 'DiglettsCave_B1F', 'DiglettsCave_SouthEntrance',
    'RockTunnel_1F', 'RockTunnel_B1F',
    'UndergroundPath_NorthSouthTunnel', 'UndergroundPath_EastWestTunnel',
    'CeruleanCave_1F', 'CeruleanCave_B1F', 'CeruleanCave_2F',
    'SaffronCity',
]


def load_palette(path):
    lines = open(path).read().splitlines()
    return [tuple(int(x) for x in line.split()) for line in lines[3:3 + 16]]


def load_tiles(png_path):
    im = Image.open(png_path)
    assert im.mode == 'P', f'expected indexed PNG, got {im.mode}'
    w, h = im.size
    px = im.load()
    tiles = []
    for ty in range(h // 8):
        for tx in range(w // 8):
            tiles.append([[px[tx * 8 + x, ty * 8 + y] for x in range(8)] for y in range(8)])
    return tiles


def load_metatiles(path):
    data = open(path, 'rb').read()
    out = []
    for i in range(len(data) // 16):
        entries = []
        for j in range(8):
            val = struct.unpack_from('<H', data, i * 16 + j * 2)[0]
            entries.append((val & 0x3FF, (val >> 10) & 1, (val >> 11) & 1, (val >> 12) & 0xF))
        out.append(entries)
    return out


def load_attributes(path):
    data = open(path, 'rb').read()
    return [struct.unpack_from('<I', data, i * 4)[0] for i in range(len(data) // 4)]


class TilesetData:
    def __init__(self, path):
        self.path = path
        self.tiles = load_tiles(os.path.join(path, 'tiles.png'))
        self.metatiles = load_metatiles(os.path.join(path, 'metatiles.bin'))
        self.attrs = load_attributes(os.path.join(path, 'metatile_attributes.bin'))
        self._palettes = {}

    def get_palette(self, idx):
        if idx not in self._palettes:
            self._palettes[idx] = load_palette(os.path.join(self.path, 'palettes', f'{idx:02d}.pal'))
        return self._palettes[idx]


_TS_CACHE = {}


def get_tileset(name):
    if name not in _TS_CACHE:
        base = name.replace('gTileset_', '')
        snake = re.sub(r'(?<!^)(?=[A-Z])', '_', base).lower()
        for sub in ('primary', 'secondary'):
            cand = os.path.join(POKEFIRERED, 'data/tilesets', sub, snake)
            if os.path.isdir(cand):
                _TS_CACHE[name] = TilesetData(cand)
                break
        else:
            raise FileNotFoundError(f'tileset dir not found for {name} (tried {snake})')
    return _TS_CACHE[name]


def render_tile(tiles_get, tile_id, hflip, vflip, pal):
    pixels, colors = tiles_get(tile_id, pal)
    img = Image.new('RGBA', (8, 8))
    for y in range(8):
        for x in range(8):
            idx = pixels[7 - y if vflip else y][7 - x if hflip else x]
            img.putpixel((x, y), (0, 0, 0, 0) if idx == 0 else (*colors[idx], 255))
    return img


def make_metatile_image(primary, secondary, metatile_global_id):
    mt = primary.metatiles[metatile_global_id] if metatile_global_id < 640 else secondary.metatiles[metatile_global_id - 640]

    def tiles_get(tile_id, pal):
        return (primary.tiles[tile_id], primary.get_palette(pal)) if tile_id < 640 else \
               (secondary.tiles[tile_id - 640], secondary.get_palette(pal))

    img = Image.new('RGBA', (16, 16), (255, 255, 255, 0))
    positions = [(0, 0), (8, 0), (0, 8), (8, 8)]
    for layer_start in (0, 4):
        for i, (tile_id, hflip, vflip, pal) in enumerate(mt[layer_start:layer_start + 4]):
            img.alpha_composite(render_tile(tiles_get, tile_id, hflip, vflip, pal), positions[i])
    return img


def render_map_full(layout):
    """Returns (image, collision_grid, behavior_grid)."""
    w, h = layout['width'], layout['height']
    primary = get_tileset(layout['primary_tileset'])
    secondary = get_tileset(layout['secondary_tileset'])
    blockdata = open(os.path.join(POKEFIRERED, layout['blockdata_filepath']), 'rb').read()
    canvas = Image.new('RGB', (w * 16, h * 16))
    collision = [[0] * w for _ in range(h)]
    behavior = [[0] * w for _ in range(h)]
    mt_cache = {}
    for i in range(w * h):
        val = struct.unpack_from('<H', blockdata, i * 2)[0]
        mt_id, coll = val & 0x3FF, (val >> 10) & 0x3
        x, y = i % w, i // w
        collision[y][x] = coll
        attr = primary.attrs[mt_id] if mt_id < 640 else secondary.attrs[mt_id - 640]
        behavior[y][x] = attr & 0x1FF
        if mt_id not in mt_cache:
            mt_cache[mt_id] = make_metatile_image(primary, secondary, mt_id).convert('RGB')
        canvas.paste(mt_cache[mt_id], (x * 16, y * 16))
    return canvas, collision, behavior


def main():
    layouts = {l['id']: l for l in json.load(open(os.path.join(POKEFIRERED, 'data/layouts/layouts.json')))['layouts'] if 'id' in l}
    map_dirs = [d for d in os.listdir(os.path.join(POKEFIRERED, 'data/maps'))
                if os.path.isdir(os.path.join(POKEFIRERED, 'data/maps', d))]
    map_jsons = {}
    for name in map_dirs:
        p = os.path.join(POKEFIRERED, 'data/maps', name, 'map.json')
        if os.path.exists(p):
            map_jsons[name] = json.load(open(p))
    id_to_name = {d['id']: name for name, d in map_jsons.items()}

    def layout_for(name):
        return layouts[map_jsons[name]['layout']]

    # ---- BFS-place every outdoor map using the game's own connection offsets ----
    offsets = {'PalletTown': (0, 0)}
    queue = collections.deque(['PalletTown'])
    while queue:
        cur = queue.popleft()
        gx, gy = offsets[cur]
        cw, ch = layout_for(cur)['width'], layout_for(cur)['height']
        for conn in map_jsons[cur].get('connections', []):
            nb_name = id_to_name.get(conn['map'])
            if not nb_name or nb_name in offsets:
                continue
            nb_layout = layout_for(nb_name)
            nw, nh = nb_layout['width'], nb_layout['height']
            d, off = conn['direction'], conn['offset']
            if d == 'up':
                ngx, ngy = gx + off, gy - nh
            elif d == 'down':
                ngx, ngy = gx + off, gy + ch
            elif d == 'left':
                ngx, ngy = gx - nw, gy + off
            elif d == 'right':
                ngx, ngy = gx + cw, gy + off
            else:
                continue
            offsets[nb_name] = (ngx, ngy)
            queue.append(nb_name)

    min_x = min(x for x, y in offsets.values())
    min_y = min(y for x, y in offsets.values())
    norm = {n: (x - min_x, y - min_y) for n, (x, y) in offsets.items()}
    CW = max(norm[n][0] + layout_for(n)['width'] for n in norm)
    CH = max(norm[n][1] + layout_for(n)['height'] for n in norm)

    # ---- pack extra interior/dungeon maps into void space ----
    occ = [bytearray(CW) for _ in range(CH)]
    for n, (gx, gy) in norm.items():
        w, h = layout_for(n)['width'], layout_for(n)['height']
        for yy in range(gy, gy + h):
            for xx in range(gx, gx + w):
                occ[yy][xx] = 1

    def find_free_rect(w, h):
        for y in range(0, CH - h + 1):
            ok = [all(occ[y + dy][x] == 0 for dy in range(h)) for x in range(CW)]
            run = 0
            for x in range(CW):
                if ok[x]:
                    run += 1
                    if run >= w:
                        return (x - w + 1, y)
                else:
                    run = 0
        return None

    all_offsets = dict(norm)
    for m in EXTRA_INTERIOR_MAPS:
        if m not in map_jsons:
            print('WARNING: extra interior map not found in decomp, skipping:', m)
            continue
        lid = map_jsons[m]['layout']
        w, h = layouts[lid]['width'], layouts[lid]['height']
        pos = find_free_rect(w + 1, h + 1)
        if pos is None:
            print('WARNING: no free space for', m)
            continue
        x, y = pos
        all_offsets[m] = (x, y)
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                occ[yy][xx] = 1

    # ---- render everything ----
    canvas = Image.new('RGB', (CW * 16, CH * 16), (20, 20, 20))
    blocked = [bytearray([1] * CW) for _ in range(CH)]
    ledge_south = set()
    region_entries = []
    for name, (gx, gy) in all_offsets.items():
        lid = map_jsons[name]['layout']
        img, coll, beh = render_map_full(layouts[lid])
        canvas.paste(img, (gx * 16, gy * 16))
        h, w = len(coll), len(coll[0])
        region_entries.append((name, gx, gy, gx + w, gy + h))
        for yy in range(h):
            for xx in range(w):
                blocked[gy + yy][gx + xx] = 1 if coll[yy][xx] != 0 else 0
                if beh[yy][xx] == MB_JUMP_SOUTH:
                    ledge_south.add((gy + yy) * CW + (gx + xx))

    os.makedirs(os.path.dirname(PNG_OUT), exist_ok=True)
    canvas.save(PNG_OUT)
    print('wrote', PNG_OUT, canvas.size)

    # ---- goals: every warp door + every fly/heal location ----
    def sanitize_ident(s):
        return ''.join(p.capitalize() for p in re.sub(r'^MAP_', '', s).split('_') if p)

    goals_seen = set()
    door_entries = []
    for name, (gx, gy) in all_offsets.items():
        for warp in map_jsons[name].get('warp_events', []):
            dest = warp.get('dest_map', 'UNKNOWN')
            if dest == 'UNKNOWN' or dest not in id_to_name:
                continue
            gxx, gyy = gx + warp['x'], gy + warp['y']
            key = (dest, gxx, gyy)
            if key in goals_seen:
                continue
            goals_seen.add(key)
            door_entries.append((sanitize_ident(dest) + 'Door', gxx, gyy, dest))
    name_counts = collections.Counter(e[0] for e in door_entries)
    seen_names = collections.Counter()
    final_doors = []
    for ident, gx, gy, dest in door_entries:
        seen_names[ident] += 1
        final_doors.append((ident if name_counts[ident] == 1 else f'{ident}{seen_names[ident]}', gx, gy, dest))

    heal = json.load(open(os.path.join(POKEFIRERED, 'src/data/heal_locations.json')))['heal_locations']
    fly_entries = []
    for loc in heal:
        name = id_to_name.get(loc['map'])
        if name is None or name not in all_offsets:
            continue
        gx, gy = all_offsets[name]
        fly_entries.append((sanitize_ident(loc['id'].replace('HEAL_LOCATION_', 'FLY_')), gx + loc['x'], gy + loc['y'], loc['id']))

    # ---- write C++ headers ----
    def hdr(path, body):
        with open(path, 'w', newline='\n') as f:
            f.write(body)
        print('wrote', path)

    mask_lines = '\n'.join('    { ' + ', '.join(str(v) for v in row) + ' },' for row in blocked)
    hdr(os.path.join(MAP_DIR, 'PokemonFRLG_KantoMapMasks_Generated.h'), f"""/*  Kanto Walkable Mask (generated)
 *
 *  GENERATED by Scripts/PokemonFRLG/generate_kanto_full_map.py -- do not hand edit.
 *  Ground truth from the pokefirered decomp's map.bin collision bits.
 *  0 = walkable, 1 = blocked. Tile size = 16 px.
 */

#ifndef PokemonAutomation_PokemonFRLG_KantoMapMasks_Generated_H
#define PokemonAutomation_PokemonFRLG_KantoMapMasks_Generated_H

#include <cstdint>

namespace PokemonAutomation{{
namespace NintendoSwitch{{
namespace PokemonFRLG{{

constexpr int KANTO_MASK_ROWS = {CH};
constexpr int KANTO_MASK_COLS = {CW};
constexpr uint8_t KANTO_MASK[{CH}][{CW}] = {{
{mask_lines}
}};

}}
}}
}}
#endif
""")

    ledge_sorted = sorted(ledge_south)
    ledge_lines = '\n'.join('    ' + ', '.join(str(k) for k in ledge_sorted[i:i + 12]) + ',' for i in range(0, len(ledge_sorted), 12))
    hdr(os.path.join(MAP_DIR, 'PokemonFRLG_KantoLedges_Generated.h'), f"""/*  Kanto South-Facing Ledges (generated)
 *
 *  GENERATED by Scripts/PokemonFRLG/generate_kanto_full_map.py -- do not hand edit.
 *  Ground truth from metatile behavior MB_JUMP_SOUTH (0x3B).
 *  Entries are (y * {CW} + x), sorted, for binary search.
 */

#ifndef PokemonAutomation_PokemonFRLG_KantoLedges_Generated_H
#define PokemonAutomation_PokemonFRLG_KantoLedges_Generated_H

#include <cstdint>

namespace PokemonAutomation{{
namespace NintendoSwitch{{
namespace PokemonFRLG{{

constexpr int KANTO_LEDGE_COLS = {CW};

constexpr uint32_t KANTO_LEDGE_SOUTH[] = {{
{ledge_lines}
}};
constexpr int KANTO_LEDGE_SOUTH_COUNT = {len(ledge_sorted)};

}}
}}
}}
#endif
""")

    hdr(os.path.join(MAP_DIR, 'PokemonFRLG_KantoMaskCorrections_Generated.h'), f"""/*  Kanto Walkable-Mask Corrections (generated)
 *
 *  GENERATED by Scripts/PokemonFRLG/generate_kanto_full_map.py -- do not hand edit.
 *  The mask is ground truth from the decomp now, so there is nothing to correct.
 *  Kept (empty) so the pathfinder's correction-application code still has a
 *  table, and so a future proven-wrong case has somewhere to go.
 */

#ifndef PokemonAutomation_PokemonFRLG_KantoMaskCorrections_Generated_H
#define PokemonAutomation_PokemonFRLG_KantoMaskCorrections_Generated_H

#include <cstdint>

namespace PokemonAutomation{{
namespace NintendoSwitch{{
namespace PokemonFRLG{{

constexpr int KANTO_CORRECTION_COLS = {CW};

constexpr uint32_t KANTO_FORCE_BLOCKED[] = {{
    0,
}};
constexpr int KANTO_FORCE_BLOCKED_COUNT = 0;

}}
}}
}}
#endif
""")

    door_lines = '\n'.join('constexpr KantoGoal %-40s {%4d, %4d, 1};  // -> %s' % e for e in final_doors)
    fly_lines = '\n'.join('constexpr KantoGoal %-40s {%4d, %4d, 1};  // %s' % e for e in fly_entries)
    hdr(os.path.join(MAP_DIR, 'PokemonFRLG_KantoGoals_Extended.h'), f"""/*  Kanto Extended Goals (generated)
 *
 *  GENERATED by Scripts/PokemonFRLG/generate_kanto_full_map.py -- do not hand edit.
 *  Every building/cave/gym/pokecenter door (warp_events) and every Fly
 *  destination (heal_locations.json), in global combined-map tile coords.
 *  Door goal count: {len(final_doors)}. Fly goal count: {len(fly_entries)}.
 */

#ifndef PokemonAutomation_PokemonFRLG_KantoGoals_Extended_H
#define PokemonAutomation_PokemonFRLG_KantoGoals_Extended_H

#include "PokemonFRLG_KantoMapNavigator.h"

namespace PokemonAutomation{{
namespace NintendoSwitch{{
namespace PokemonFRLG{{
namespace KantoGoals{{
namespace Extended{{

//  ---- Every building / cave / gym / pokecenter door (from warp_events) ----
{door_lines}

//  ---- Every Fly destination (from heal_locations.json) ----
{fly_lines}

}}
}}
}}
}}
}}
#endif
""")

    region_lines = '\n'.join('    { "%s", %4d, %4d, %4d, %4d },' % e for e in sorted(region_entries, key=lambda e: (e[2], e[1])))
    hdr(os.path.join(MAP_DIR, 'PokemonFRLG_KantoRegions_Generated.h'), f"""/*  Kanto Sub-Region Boundaries (generated)
 *
 *  GENERATED by Scripts/PokemonFRLG/generate_kanto_full_map.py -- do not hand edit.
 *  Exact tile bounds for every outdoor city/route and packed-in interior,
 *  derived from the decomp's own connection offsets. Used only for log labels.
 */

#ifndef PokemonAutomation_PokemonFRLG_KantoRegions_Generated_H
#define PokemonAutomation_PokemonFRLG_KantoRegions_Generated_H

namespace PokemonAutomation{{
namespace NintendoSwitch{{
namespace PokemonFRLG{{

struct KantoRegionBounds{{
    const char* name;
    int x0, y0, x1, y1;
}};

constexpr KantoRegionBounds KANTO_REGIONS[] = {{
{region_lines}
}};
constexpr int KANTO_REGIONS_COUNT = {len(region_entries)};

}}
}}
}}
#endif
""")

    print(f'Done. {len(all_offsets)} regions, {CW}x{CH} tiles, {len(final_doors)} door goals, {len(fly_entries)} fly goals, {len(ledge_sorted)} ledge cells.')


if __name__ == '__main__':
    main()
