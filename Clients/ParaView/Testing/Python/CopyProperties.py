#### import the simple module from the paraview
from paraview.simple import *

s1=Sphere()

s1.Center=3,4,5
s1.ThetaResolution=16
s1.PhiResolution=32

s2=Sphere()
s2.Center=1,1,1

s2.Copy(s1)

if s2.Center[0] != 3 or s2.Center[1] != 4 or s2.Center[2] != 5:
    print("ERROR: Sphere2 has wrong center (" + str(s2.Center[0]) + ", " + str(s2.Center[1]) + ", " + str(s2.Center[2]) + "), expected (3, 4, 5)")
    sys.exit(1)

if s2.ThetaResolution != 16:
    print("ERROR: Sphere2 has wrong Theta Resolution (" + str(s2.ThetaResolution) + "), expected 16")
    sys.exit(1)

if s2.PhiResolution != 32:
    print("ERROR: Sphere2 has wrong Phi Resolution (" + str(s2.PhiResolution) + "), expected 32")
    sys.exit(1)
