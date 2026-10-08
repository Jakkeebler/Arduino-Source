import json, io, os, shutil, sys

P = r'C:\Users\Jeron\Documents\Programming\Pokemon\Arduino-Source\build-msvc-qt683\RelWithDebInfo\UserSettings\SerialPrograms-Settings.json'

GROUPS = {
    "GRIND_SETUP": ["GRIND_LOCATION", "NAVIGATE_TO_GRIND_ON_START"],
    "PARTY_SETUP": ["ROTATION_MODE", "FIGHTER_SLOT", "PARTY_SIZE"],
    "HEALING":     ["AUTO_HEAL_LOCATION", "HEAL_LOCATION", "TRAVEL_METHOD",
                    "HEAL_BEFORE_START", "HEAL_ON_FAINT", "HEAL_ON_OUT_OF_PP",
                    "BATTLES_PER_HEAL_TRIP"],
    "TEAM_SETUP":  ["LANGUAGE", "AUTO_SCAN_ON_START", "IMPORT_TEAM_FILE",
                    "IMPORT_TEAM_BUTTON", "AUTO_RANK_MOVES", "AUTOFILL_DESIRED_MOVES",
                    "HOLD_EVOLUTION_FOR_MOVES", "STOP_WHEN_TEAM_COMPLETE", "TEAM_TABLE"],
}

raw = io.open(P, encoding='utf-8-sig').read()
d = json.loads(raw)

node = d.get("99-Panels", {}).get("PokemonFRLG:XPGrinder")
if node is None:
    print("XPGrinder settings not found - nothing to migrate")
    sys.exit(0)

already = [g for g in GROUPS if g in node]
if already:
    print("already migrated (found %s) - leaving alone" % ", ".join(already))
    sys.exit(0)

bak = P + ".bak-pregroups"
if not os.path.exists(bak):
    shutil.copy2(P, bak)
    print("backup written:", bak)

moved, missing = [], []
for group, keys in GROUPS.items():
    obj = {}
    for k in keys:
        if k in node:
            obj[k] = node.pop(k)
            moved.append("%s -> %s" % (k, group))
        else:
            missing.append(k)
    node[group] = obj

with io.open(P, 'w', encoding='utf-8-sig', newline='\n') as f:
    f.write(json.dumps(d, indent=4, ensure_ascii=False))

print("moved %d option(s) into %d group(s)" % (len(moved), len(GROUPS)))
for m in moved:
    print("   ", m)
if missing:
    print("not present in settings (will use defaults):", ", ".join(missing))

chk = json.loads(io.open(P, encoding='utf-8-sig').read())
n2 = chk["99-Panels"]["PokemonFRLG:XPGrinder"]
print("\nverify:")
print("   GRIND_SETUP.GRIND_LOCATION =", n2["GRIND_SETUP"].get("GRIND_LOCATION"))
print("   PARTY_SETUP.ROTATION_MODE  =", n2["PARTY_SETUP"].get("ROTATION_MODE"))
print("   PARTY_SETUP.FIGHTER_SLOT   =", n2["PARTY_SETUP"].get("FIGHTER_SLOT"))
print("   HEALING.TRAVEL_METHOD      =", n2["HEALING"].get("TRAVEL_METHOD"))
print("   TEAM_SETUP.TEAM_TABLE rows =", len(n2["TEAM_SETUP"].get("TEAM_TABLE", [])))
print("   top-level keys left        =", sorted(k for k in n2 if k not in GROUPS))
