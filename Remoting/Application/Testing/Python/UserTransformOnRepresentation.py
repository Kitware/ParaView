# Tests support for UserTransform on representations.
# There's no UI to set UserTransform and only mechanism is to use Python.

# This work was supported by CEA/DIF - Commissariat a l'Energie Atomique,
# Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France

from paraview import smtesting
from paraview.simple import *

smtesting.ProcessCommandLineArguments()
view = CreateRenderView()

# add a text source #1
Text(Text="Hello World")
Show()

# add empty text source
Text(Text="")
Show()

data = Sphere()
display = Show()

transform = servermanager.vtk.vtkTransform()
transform.Scale(2, 1, 1)
matrix = transform.GetMatrix()
print("-------------------------------")
print("Transformation Matrix: ")
print(matrix)

flattened_transform = []
for j in range(4):
  for i in range(4):
    flattened_transform.append(matrix.GetElement(i, j))


display.UserTransform = flattened_transform
Render()
view.OrientationAxesVisibility = 0
if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError('Test failed.')
