import os
from paraview import vtk 
if os.name == "posix":
    from libvtkPVFiltersPython import *
else:
    from vtkPVFiltersPython import *
