import cv2, os
ROOT = r"D:\Programming\Pokemon\Arduino-Source"
img = cv2.imread(os.path.join(ROOT,'Packages','Resources','PokemonFRLG','Maps','Kanto-Combined.png'), cv2.IMREAD_COLOR)
T=16
X0,X1,Y0,Y1 = 36,48,198,210
crop = img[Y0*T:(Y1+1)*T, X0*T:(X1+1)*T]
big = cv2.resize(crop, None, fx=6, fy=6, interpolation=cv2.INTER_NEAREST)
# draw tile grid + labels
for i in range(X1-X0+2):
    cv2.line(big,(i*T*6,0),(i*T*6,big.shape[0]),(0,0,255),1)
for j in range(Y1-Y0+2):
    cv2.line(big,(0,j*T*6),(big.shape[1],j*T*6),(0,0,255),1)
for i in range(X1-X0+1):
    cv2.putText(big,str(X0+i),(i*T*6+6,14),cv2.FONT_HERSHEY_SIMPLEX,0.38,(0,0,255),1)
for j in range(Y1-Y0+1):
    cv2.putText(big,str(Y0+j),(2,j*T*6+34),cv2.FONT_HERSHEY_SIMPLEX,0.38,(0,0,255),1)
out = os.path.join(ROOT,'route22_zoom.png')
cv2.imwrite(out,big)
print("wrote",out,big.shape)
