# Set up a basic scene for rendering.
from paraview.simple import *
import sys

script = """
import vtk

def setup_data(view):
  # Don't actually need any data
  pass

def render(view, width, height):
  canvas = vtk.vtkImageCanvasSource2D()
  canvas.SetExtent(0, width-1, 0, height-1, 0, 0)
  canvas.SetNumberOfScalarComponents(3)
  canvas.SetScalarTypeToUnsignedChar()
  canvas.SetDrawColor(0, 0, 0)
  canvas.FillBox(0,width-1,0,height-1)
  canvas.SetDrawColor(255, 255, 0)
  canvas.DrawCircle(int(width/2), int(height/2), 10)
  canvas.SetDrawColor(255, 0, 0)
  canvas.FillTube(10, 10, 30, 200, 7)
  canvas.SetDrawColor(0, 0, 255)
  canvas.FillTriangle(width-20, height-20,
                      int(width/2)+10, int(height/2)-10,
                      width - 100, height - 150)
  canvas.Update()

  image = vtk.vtkImageData()
  image.DeepCopy(canvas.GetOutput())

  print (image)

  return image
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
baseline_file = os.path.join(baselinePath, "TestPythonViewScript.png")

from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(view.GetRenderWindow(), baseline_file, threshold=25)
Testing.interact()
