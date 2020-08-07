from paraview.simple import *
from paraview.vtk.util.misc import vtkGetDataRoot
from paraview.vtk.util.misc import vtkGetTempDir
import os
import sys

wavelet = Wavelet()
SetActiveSource(wavelet)
view = GetActiveViewOrCreate('RenderView')
disp = GetDisplayProperties(wavelet, view)
disp.Representation = "Surface With Edges"
Show(wavelet, view)
direction = [0.5, 1, 0.5]

ResetCameraToDirection(view.CameraFocalPoint, direction)

try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print ("Could not get baseline directory. Test failed.")
  exit(1)
baseline_file = os.path.join(baselinePath, "TestResetCameraToDirection.png")

from paraview.vtk.test import Testing
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(GetActiveView().GetRenderWindow(), baseline_file,
                     threshold=25)
Testing.interact()
