#### import the simple module from the paraview
from paraview.simple import *

# Create a wavelet
wavelet = Wavelet()
Show()

# create a new 'Random Vectors'
randomVectors = RandomVectors(Input=wavelet)

# create a new 'Warp By Vector'
warpByVector = WarpByVector(Input=randomVectors)
warpByVector.Vectors = ['POINTS', 'BrownianVectors']
Show()
Render()

# store the default scale factor
warpByVector.SMProxy.GetProperty("ScaleFactor").ResetToDefault()
defaultScaleFactor = warpByVector.ScaleFactor

# modify the scale factor
warpByVector.ScaleFactor = 2

# try to retrieve the default scale factor
ResetProperty("ScaleFactor")

if warpByVector.ScaleFactor != defaultScaleFactor:
    print ("Reset scale factor test failed.")
    exit(1)
