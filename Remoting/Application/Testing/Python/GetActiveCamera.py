from paraview.simple import *
from paraview import vtk

SUPPORTED_METHODS=dir(vtk.vtkCamera())

sphere=Sphere()
Show()
Render()
camera=GetActiveCamera()

assert set(SUPPORTED_METHODS).issubset(set(dir(camera)))

print("Success: GetActiveCamera returns a vtkCamera object.")
