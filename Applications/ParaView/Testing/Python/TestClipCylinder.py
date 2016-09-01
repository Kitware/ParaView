#### import the simple module from the paraview
from paraview.simple import *

# Create a wavelet and clip it with a Cylinder.
wavelet = Wavelet()
Show()

clip = Clip()
clip.ClipType = "Cylinder"
clip.InsideOut = True

cylinder = clip.ClipType
cylinder.Axis = [-1, 1, -1]
cylinder.Center = [8, 4, -3]
cylinder.Radius = 3
Show()

Render()

# compare with baseline image
import os
import sys
try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print ("Could not get baseline directory. Test failed.")
  exit(1)
baseline_file = os.path.join(baselinePath, "TestClipCylinder.png")
import vtk.test.Testing
vtk.test.Testing.VTK_TEMP_DIR = vtk.util.misc.vtkGetTempDir()
vtk.test.Testing.compareImage(GetActiveView().GetRenderWindow(), baseline_file,
                              threshold=25)
vtk.test.Testing.interact()
