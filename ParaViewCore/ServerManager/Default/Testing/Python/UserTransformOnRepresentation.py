# Tests support for UserTransform on representations.
# There's no UI to set UserTransform and only mechanism is to use Python.

import SMPythonTesting
from paraview.simple import *

SMPythonTesting.ProcessCommandLineArguments()

data = Sphere()
display = Show()

transform = servermanager.vtk.vtkTransform()
transform.Scale(2, 1, 1)
matrix = transform.GetMatrix()
print "-------------------------------"
print "Transformation Matrix: "
print matrix

flattened_trasform = []
for j in range(4):
  for i in range(4):
    flattened_trasform.append(matrix.GetElement(i, j))


display.UserTransform = flattened_trasform
view = Render()
if not SMPythonTesting.DoRegressionTesting(view.SMProxy):
    raise SMPythonTesting.TestError, 'Test failed.'
