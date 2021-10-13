from paraview.simple import *
from paraview.vtk.util.misc import vtkGetDataRoot, vtkGetTempDir
from os.path import realpath, join, dirname
import sys, os.path

scriptdir = dirname(realpath(__file__))
statefile = join(scriptdir, "StateWithTextures.pvsm")
data_dir = vtkGetDataRoot() + "/Testing/Data/"

LoadState(statefile,
        data_directory=data_dir,
        restrict_to_data_directory=True)

view = GetRenderView()
Render()

try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print("Could not get baseline directory. Test failed.")

baseline_file = os.path.join(baselinePath, "LoadStateWithTextures.png")
from paraview.vtk.test import Testing
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(view.GetRenderWindow(), baseline_file, threshold=10)
Testing.interact()
