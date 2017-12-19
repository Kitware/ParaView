# Set up a basic scene for rendering.
from paraview.simple import *
import os
import sys

script = """
from paraview.vtk.util import numpy_support
import matplotlib.style
import matplotlib as mpl

mpl.style.use('classic')

# Utility to get next color
def getNextColor():
  colors = 'bgrcmykw'
  for c in colors:
    yield c

# This function must be defined. It is where specific data arrays are requested.
def setup_data(view):
  print ("Setting up data")

# This function must be defined. It is where the actual rendering commands for matplotlib go.
def render(view,width,height):
  from paraview import python_view
  figure = python_view.matplotlib_figure(width,height)

  ax = figure.add_subplot(111)
  numObjects = view.GetNumberOfVisibleDataObjects()
  print ("num visible objects: ", numObjects)
  for i, color in zip(range(0,numObjects), getNextColor()):
    dataObject = view.GetVisibleDataObjectForRendering(i)
    if dataObject:
      vtk_points = dataObject.GetPoints()
      if vtk_points:
        vtk_points_data = vtk_points.GetData()
        pts = numpy_support.vtk_to_numpy(vtk_points_data)
        x, y = pts[:,0], pts[:,1]
        ax.scatter(x, y, color=color)

  return python_view.figure_to_image(figure)
"""

view = CreateView("PythonView")
view.Script = script

cone = Cone()
Show(cone, view)

sphere = Sphere()
Show(sphere, view)

Render()

try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print ("Could not get baseline directory. Test failed.")

baseline_file = os.path.join(baselinePath, "TestPythonViewMatplotlibScript.png")

from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(view.GetRenderWindow(), baseline_file, threshold=25)
Testing.interact()

Delete(cone)
del cone

Delete(sphere)
del sphere
