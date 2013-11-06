# Set up a basic scene for rendering.
from paraview.simple import *
import sys

script = """
import paraview.numpy_support

# Utility to get next color
def getNextColor():
  colors = 'bgrcmykw'
  for c in colors:
    yield c

# This function must be defined. It is where specific data arrays are requested.
def setup_data(view):
  print "Setting up data"

# This function must be defined. It is where the actual rendering commands for matplotlib go.
def render(view,figure):
  ax = figure.add_subplot(111)
  ax.hold = True
  numObjects = view.GetNumberOfVisibleDataObjects()
  print "num visible objects: ", numObjects
  for i, color in zip(xrange(0,numObjects), getNextColor()):
    dataObject = view.GetVisibleDataObjectForRendering(i)
    if dataObject:
      vtk_points = dataObject.GetPoints()
      if vtk_points:
        vtk_points_data = vtk_points.GetData()
        pts = paraview.numpy_support.vtk_to_numpy(vtk_points_data)
        x, y = pts[:,0], pts[:,1]
        ax.scatter(x, y, color=color)

  ax.hold = False
"""

if len(sys.argv) > 2:
  server = sys.argv[2]
  if not Connect(server):
    print "Could not connect to server", server

view = CreateView("PythonView")
view.Script = script

cone = Cone()
Show(cone, view)

sphere = Sphere()
Show(sphere, view)

Render()

WriteImage("pvpython.png", view)

Delete(cone)
del cone

Delete(sphere)
del sphere
