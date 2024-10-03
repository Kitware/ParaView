from paraview.simple import *
PythonbasedSuperquadricSourceExample()
Show()
rv = Render()

import os
import sys
try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print("Could not get baseline directory. Test failed.")
baseline_file = os.path.join(baselinePath, "TestPV_PLUGIN_PATH.png")
from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(rv.GetRenderWindow(), baseline_file)
Testing.interact()
