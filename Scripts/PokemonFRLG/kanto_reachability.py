import re, os, sys
from collections import deque
BASE = r"D:\Programming\Pokemon\Arduino-Source\SerialPrograms\Source\PokemonFRLG\Inference\Map"
W, H = 408, 400
txt = open(os.path.join(BASE,"PokemonFRLG_KantoMapMasks_Generated.h"), encoding="utf-8", errors="replace").read()
m = re.search(r"KANTO_MASK\s*\[[^\]]*\]\s*\[[^\]]*\]\s*=\s*\{(.*?)\n\};", txt, re.S)
MASK = [[int(v) for v in re.findall(r"-?\d+", r)] for r in re.findall(r"\{([^{}]*)\}", m.group(1))]
ftxt = open(os.path.join(BASE,"PokemonFRLG_KantoMaskCorrections_Generated.h"), encoding="utf-8", errors="replace").read()
FB = set(int(v) for v in re.findall(r"\b(\d{3,7})\b", ftxt.split("=",1)[1]))
ltxt = open(os.path.join(BASE,"PokemonFRLG_KantoLedges_Generated.h"), encoding="utf-8", errors="replace").read()
LG = set(int(v) for v in re.findall(r"\b(\d{3,7})\b", ltxt.split("=",1)[1]))

OVR = [(74,206,74,206,True),(66,215,69,221,False),(74,215,82,221,False),
       (46,200,54,208,False),(40,198,41,203,False)]

def is_ledge(x,y): return (y*W+x) in LG
def walkable(x,y):
    if x<0 or y<0 or x>=W or y>=H: return False
    if is_ledge(x,y): return False
    for (x0,y0,x1,y1,w) in OVR:
        if x0<=x<=x1 and y0<=y<=y1: return w
    if (y*W+x) in FB: return False
    return MASK[y][x]==0

def hop_target(x,y):
    # standing at (x,y); tile below is a ledge; keep going south past consecutive ledges
    yy=y+1
    if not is_ledge(x,yy): return None
    while is_ledge(x,yy): yy+=1
    return yy if walkable(x,yy) else None

def flood(sx,sy):
    seen={(sx,sy)}; q=deque([(sx,sy)])
    while q:
        x,y=q.popleft()
        for dx,dy in ((0,-1),(0,1),(1,0),(-1,0)):
            nx,ny=x+dx,y+dy
            if walkable(nx,ny) and (nx,ny) not in seen:
                seen.add((nx,ny)); q.append((nx,ny))
        t=hop_target(x,y)
        if t is not None and (x,t) not in seen:
            seen.add((x,t)); q.append((x,t))
    return seen

for start in [(40,204),(45,206),(43,207),(36,201),(74,207),(74,206)]:
    if not walkable(*start):
        print(start,"START NOT WALKABLE"); continue
    r=flood(*start)
    print("from %-10s reachable=%6d  PC(74,207) in set: %s" % (str(start), len(r), (74,207) in r))

r=flood(40,204)
ys=sorted(set(y for x,y in r))
print("\nregion from (40,204): x %d..%d  y %d..%d" % (min(x for x,_ in r),max(x for x,_ in r),min(ys),max(ys)))
