import sys
from paraview.simple import *

w = Wavelet()

Show(w)

s = Slice(Input=w)
s.SliceType.Origin = [0, 0, 3]
s.SliceType.Normal = [0, 0, 1]

sliceRep = Show(s)

view = GetActiveView()
view.ResetCamera()
view.ViewSize = [400, 400]

Render()

pt1 = [0, 0, 0]
pt2 = [0, 0, 0]
view.ConvertDisplayToPointOnSurface([100, 100], pt1)
view.ConvertDisplayToPointOnSurface([245, 215], pt2)

print("Point 1:")
print(pt1)
print("Point 2:")
print(pt2)

pt1Goal = [-8.563830933033852, -8.563830933033852, 3.0]
pt2Goal = [3.853723919865235, 1.284574639955077, 3.0]

for i in range(3):
    if abs(pt1[i] - pt1Goal[i]) > 1e-4:
        print("Error, point 1 should be:")
        print(pt1Goal)
    if abs(pt2[i] - pt2Goal[i]) > 1e-4:
        print("Error, point 2 should be:")
        print(pt2Goal)
