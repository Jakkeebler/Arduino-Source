# KantoGoals anchor-row validation

Scope: every `KantoGoals::*` constant in `PokemonFRLG_KantoMapNavigator.h`
(referenced by `PokemonFRLG_GrindHealLocations.cpp`), checked against commit
`47d07c51e` ("the player anchor row is 4.5, not 4"), which moved
`PLAYER_ANCHOR_PX_Y` by one row (-16px / +1 row for live detector readings).

**Outcome: no coordinates changed.** Confidence is moderate-to-high from git
provenance, and the check against the map image itself could **not** be done
(see "Not verified").

## Method

1. Read the full `KantoGoals` namespace and the enum -> goal mapping in
   `PokemonFRLG_GrindHealLocations.cpp` (11 PC entrances, 15 grass spots; all
   26 map 1:1 to a `KantoGoals` constant; `Route1NorthGrassCorner` is unused by
   the dropdowns).
2. `git log -p` on the header, `git log -S` on each constant name/coordinate
   across all branches, and read the full message of every commit that
   touched them: `1e06d02f0`, `fb346980e`, `60a5a1954`, `c67e90ac6`.
3. Looked for commit messages / comments citing error reports, live captures
   or "observed at" as the source of a coordinate.
4. Looked for independent corroboration elsewhere in the tree.

## Findings

| Group | Constants | Provenance | Detector-derived? |
|---|---|---|---|
| PC entrances (10 new) | Pewter, Route4, Cerulean, Route10, Celadon, Lavender, Saffron, Vermilion, Fuchsia, Cinnabar | `60a5a1954`: "derived ... from the combined Kanto map image ... template-matching the shared PC building sprite. Goal tile is (door_x, door_y + 1)" | No - offline template match on the PNG |
| PC entrance (Viridian) | `{74,207}` | `1e06d02f0`: hand-read from the image ("PC building footprint ... courtyard tile south of the PC's center"). `60a5a1954` states the template match "reproduces ... (74,207) exactly". `PokemonFRLG_KantoMapPathfinder.cpp` MASK_OVERRIDES independently says the door is (74,206), "verified against Kanto-Combined.png", so goal = door_y + 1 = 207. | No |
| Grass spots (15) | Route1N/S, Route2N/S, Route3W/E, Route4, 6, 7, 8, 9, 10, 16, Route22E/W | `60a5a1954`: tall-grass tile matched across the map image, grouped into patches, walkable tile with the most grass in a 5x5 window chosen | No - offline image match |
| Legacy | `Route1NorthGrassCorner {62,222}` | `1e06d02f0`, hand-chosen; later shown NOT to be grass | No (and unused by grind dropdowns) |

No commit message or comment attributes any goal coordinate to a live
detector reading, an error-report screenshot, or an on-hardware capture. The
only live-detector-based coordinates in the history are in `47d07c51e`'s own
analysis (Route 22 ledge gap, tile (40,205) / (45,206)) and those are not
KantoGoals constants. Route22EastGrass (36,201) / Route22WestGrass (17,201)
are image-derived like the rest; they are 4 rows north of that ledge gap and
nothing ties them to it.

The `60a5a1954` message also says all goals were checked against `KANTO_MASK`
for walkability and that PC goals "have a PC sprite above them" - image-based
checks, consistent with the commit-47d07c51e claim that goals are true map
coordinates and need no shift.

## Not verified (explicit gaps)

- `Resources/PokemonFRLG/Maps/Kanto-Combined.png` is **not present** in this
  checkout, in the sibling `Packages/Resources/PokemonFRLG/` (no `Maps/` dir
  at all), or in `../_Resources` (searched by name). A whole-disk search timed
  out, so it is not proven absent from the machine. So I could **not** do step 3
  of the task: no pixel check that PC door tiles / grass tiles land where the
  comments say. This validation is therefore provenance-only.
- No Python imaging libs (Pillow/OpenCV) are installed in the sandbox either.
- I did not re-run the `60a5a1954` template matches; I relied on its commit
  message for method.
- Other row-sensitive code was out of scope: the MASK_OVERRIDES and ledge
  tables in `PokemonFRLG_KantoMapPathfinder.cpp` cite "verified against the
  map image" but a few of their comments cite live observations; a reader
  should confirm those were authored from artwork rows, not from pre-fix
  reads.

## Recommended follow-up

1. Hardware pass: for each of the 26 goals, run `KantoMapPositionTest` (or a
   navigate-to) and confirm the post-fix reported tile equals the goal and
   that PC goals end with the door directly north (A-press enters).
2. Or, with the image in place, run `Scripts/PokemonFRLG/region_match.py`
   / `kanto_dump_region.py` over each goal and confirm door above / grass
   below as stated.
