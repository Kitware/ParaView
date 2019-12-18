# Set up a basic scene for rendering.
from paraview.simple import *
import sys

script = """
import numpy

def setup_data(view):
  # Don't actually need any data
  pass

def render(view, width, height):
  cb = numpy.zeros((height, width, 3), dtype=numpy.uint8)
  for i in range(width):
    cb[:,i,0] = i%255

  for i in range(height):
    cb[i,:,1] = i%255

  from paraview.python_view import numpy_to_image

  return numpy_to_image(cb)
"""

view = CreateView("PythonView")
view.Script = script

Render()

try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print ("Could not get baseline directory. Test failed.")

import os
baseline_file = os.path.join(baselinePath, "TestPythonViewNumpyScript.png")
print("baseline_file == ", baseline_file)

from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir
Testing.VTK_TEMP_DIR = vtkGetTempDir
Testing.compareImage(view.GetRenderWindow(), baseline_file, threshold=25)
Testing.interact()
