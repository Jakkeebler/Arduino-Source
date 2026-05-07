#!/usr/bin/env python3
"""
Fetch FRLG move / species / learnset data from PokéAPI and write the three
JSON files under Packages/Resources/PokemonFRLG/Data/.

This is a one-shot maintenance script; it is NOT run by the build. Run it
when you want to refresh or expand the data, then commit the resulting JSON.

Usage:
    python Scripts/PokemonFRLG/fetch_pokeapi_data.py
    python Scripts/PokemonFRLG/fetch_pokeapi_data.py --max-dex 151
    python Scripts/PokemonFRLG/fetch_pokeapi_data.py --moves-only

Requires: requests (pip install requests).

The script preserves any existing entries when merging so you can hand-edit
overrides without losing them on the next refresh.
"""

import argparse
import json
import sys
import time
from pathlib import Path

try:
    import requests
except ImportError:
    print("requests not installed. Run: pip install requests", file=sys.stderr)
    sys.exit(1)

POKEAPI = "https://pokeapi.co/api/v2"
DATA_DIR = Path(__file__).resolve().parents[2] / "Packages" / "Resources" / "PokemonFRLG" / "Data"

# PokéAPI's "version_group" name for FireRed/LeafGreen.
FRLG_VERSION_GROUP = "firered-leafgreen"


def slug_from_url(url: str) -> str:
    """Extract the trailing path segment of a PokéAPI URL."""
    return url.rstrip("/").rsplit("/", 1)[-1]


def load_existing(path: Path):
    if not path.exists():
        return None
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def write_json(path: Path, data) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print(f"  wrote {path.relative_to(Path.cwd())}")


def fetch(url: str, retries: int = 3) -> dict:
    for attempt in range(retries):
        r = requests.get(url, timeout=30)
        if r.status_code == 200:
            return r.json()
        time.sleep(1 + attempt * 2)
    r.raise_for_status()


def fetch_moves(max_dex: int) -> dict:
    """Fetch all moves available via Gen 3 (FRLG) learnsets up through max_dex."""
    print(f"Fetching moves used by Pokedex 1..{max_dex} ...")
    needed = set()
    for dex in range(1, max_dex + 1):
        try:
            data = fetch(f"{POKEAPI}/pokemon/{dex}")
        except Exception as e:
            print(f"  skip dex {dex}: {e}", file=sys.stderr)
            continue
        for m in data["moves"]:
            for v in m["version_group_details"]:
                if v["version_group"]["name"] == FRLG_VERSION_GROUP and v["move_learn_method"]["name"] == "level-up":
                    #  Use m["move"]["name"] (the real slug like "pound") rather
                    #  than slug_from_url which would return the numeric ID
                    #  from URLs of the form /api/v2/move/1/.
                    needed.add(m["move"]["name"])
        print(f"  dex {dex}: {data['name']} ({len(needed)} moves so far)")
    print(f"Fetching detail for {len(needed)} moves ...")
    out = {}
    for slug in sorted(needed):
        d = fetch(f"{POKEAPI}/move/{slug}")
        eng = next((n["name"] for n in d["names"] if n["language"]["name"] == "en"), slug)
        out[slug] = {
            "displays": {"eng": eng},
            "type": d["type"]["name"],
            "pp": d["pp"] or 0,
            "category": d["damage_class"]["name"] if d["damage_class"] else "status",
        }
    return out


def fetch_species(max_dex: int) -> list:
    print(f"Building species list 1..{max_dex} ...")
    out = []
    for dex in range(1, max_dex + 1):
        try:
            d = fetch(f"{POKEAPI}/pokemon-species/{dex}")
        except Exception as e:
            print(f"  skip dex {dex}: {e}", file=sys.stderr)
            continue
        out.append({"slug": d["name"], "dex": dex})
    return out


def fetch_learnsets(species_list: list) -> dict:
    print(f"Fetching learnsets for {len(species_list)} species ...")
    out = {}
    for s in species_list:
        slug = s["slug"]
        try:
            d = fetch(f"{POKEAPI}/pokemon/{slug}")
        except Exception as e:
            print(f"  skip {slug}: {e}", file=sys.stderr)
            continue
        rows = []
        for m in d["moves"]:
            for v in m["version_group_details"]:
                if v["version_group"]["name"] == FRLG_VERSION_GROUP and v["move_learn_method"]["name"] == "level-up":
                    #  Use m["move"]["name"] (real slug) rather than the
                    #  numeric ID slug_from_url returns for /move/N/ URLs.
                    rows.append((v["level_learned_at"], m["move"]["name"]))
        rows.sort()
        if rows:
            out[slug] = [[lvl, mv] for lvl, mv in rows]
    return out


def _walk_evolution_chain(node) -> list:
    """Recursively flatten an evolution_chain.chain tree into a list of slugs."""
    result = [node["species"]["name"]]
    for child in node.get("evolves_to", []):
        result.extend(_walk_evolution_chain(child))
    return result


def fetch_evolutions(species_list: list) -> dict:
    """For each species, fetch its evolution chain and emit {slug: [chain]}.
    Every species in a chain maps to the same full chain list."""
    print(f"Fetching evolution chains for {len(species_list)} species ...")
    seen_chains = {}     # chain_id -> list of slugs
    chain_for_slug = {}  # slug -> list of slugs
    for s in species_list:
        slug = s["slug"]
        if slug in chain_for_slug:
            continue
        try:
            sp = fetch(f"{POKEAPI}/pokemon-species/{slug}")
        except Exception as e:
            print(f"  skip species {slug}: {e}", file=sys.stderr)
            continue
        chain_url = sp["evolution_chain"]["url"]
        chain_id = slug_from_url(chain_url)
        if chain_id not in seen_chains:
            try:
                chain_data = fetch(chain_url)
            except Exception as e:
                print(f"  skip chain {chain_id}: {e}", file=sys.stderr)
                continue
            seen_chains[chain_id] = _walk_evolution_chain(chain_data["chain"])
        chain = seen_chains[chain_id]
        for member in chain:
            chain_for_slug[member] = chain
    return chain_for_slug


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--max-dex", type=int, default=151,
                    help="Max National Dex number to fetch (default 151 = Kanto).")
    ap.add_argument("--moves-only", action="store_true", help="Only refresh Moves.json.")
    ap.add_argument("--species-only", action="store_true", help="Only refresh Species.json.")
    ap.add_argument("--learnsets-only", action="store_true", help="Only refresh Learnsets.json.")
    ap.add_argument("--evolutions-only", action="store_true", help="Only refresh Evolutions.json.")
    args = ap.parse_args()

    do_all = not (args.moves_only or args.species_only or args.learnsets_only or args.evolutions_only)

    if do_all or args.species_only:
        species = fetch_species(args.max_dex)
        write_json(DATA_DIR / "Species.json", species)
    else:
        species = load_existing(DATA_DIR / "Species.json") or []

    if do_all or args.moves_only:
        moves = fetch_moves(args.max_dex)
        write_json(DATA_DIR / "Moves.json", moves)

    if do_all or args.learnsets_only:
        learnsets = fetch_learnsets(species)
        write_json(DATA_DIR / "Learnsets.json", learnsets)

    if do_all or args.evolutions_only:
        evolutions = fetch_evolutions(species)
        write_json(DATA_DIR / "Evolutions.json", evolutions)

    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
