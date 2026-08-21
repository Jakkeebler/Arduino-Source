import cv2, numpy as np, os, sys
ROOT = r"D:\Programming\Pokemon\Arduino-Source"
MAP  = os.path.join(ROOT, "build-msvc-qt683", "RelWithDebInfo",
                    "Resources", "PokemonFRLG", "Maps", "Kanto-Combined.png")
img = cv2.imread(MAP, cv2.IMREAD_COLOR)
T = 16
ROWS, COLS = img.shape[0]//T, img.shape[1]//T
X0, X1 = int(sys.argv[1]), int(sys.argv[2])
Y0, Y1 = int(sys.argv[3]), int(sys.argv[4])
print("V = void (pure white), . = rendered")
print("     " + "".join(str((x//10)%10) for x in range(X0, X1+1)))
print("     " + "".join(str(x%10) for x in range(X0, X1+1)))
for y in range(Y0, Y1+1):
    line = ""
    for x in range(X0, X1+1):
        tile = img[y*T:(y+1)*T, x*T:(x+1)*T]
        line += "V" if (tile > 249).all() else "."
    print("%4d %s" % (y, line))
