import re, os
BASE = r"D:\Programming\Pokemon\Arduino-Source\SerialPrograms\Source\PokemonFRLG\Inference\Map"
W = 408
txt = open(os.path.join(BASE, "PokemonFRLG_KantoMapMasks_Generated.h"), encoding="utf-8", errors="replace").read()
m = re.search(r"KANTO_MASK\s*\[[^\]]*\]\s*\[[^\]]*\]\s*=\s*\{(.*?)\n\};", txt, re.S)
MASK = [[int(v) for v in re.findall(r"-?\d+", r)] for r in re.findall(r"\{([^{}]*)\}", m.group(1))]
ftxt = open(os.path.join(BASE, "PokemonFRLG_KantoMaskCorrections_Generated.h"), encoding="utf-8", errors="replace").read()
fb = set(int(v) for v in re.findall(r"\b(\d{3,7})\b", ftxt.split("=",1)[1]))
ltxt = open(os.path.join(BASE, "PokemonFRLG_KantoLedges_Generated.h"), encoding="utf-8", errors="replace").read()
lg = set(int(v) for v in re.findall(r"\b(\d{3,7})\b", ltxt.split("=",1)[1]))

def grid(X0,X1,Y0,Y1,use_corr):
    print("     " + "".join(str((x//10)%10) for x in range(X0,X1+1)))
    print("     " + "".join(str(x%10) for x in range(X0,X1+1)))
    for y in range(Y0,Y1+1):
        s=""
        for x in range(X0,X1+1):
            k=y*W+x
            if use_corr and k in lg: c="L"
            elif use_corr and k in fb: c="F"
            elif MASK[y][x]==0: c="."
            else: c="#"
            s+=c
        print("%4d %s"%(y,s))

print("=== EFFECTIVE (L=ledge, F=force-blocked correction, #=mask solid, .=walkable) ===")
grid(32,92,195,222,True)
print()
print("=== RAW MASK ONLY ===")
grid(32,92,195,222,False)
