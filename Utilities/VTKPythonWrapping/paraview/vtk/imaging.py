import os

if os.name == "posix":
    from libvtkImagingPython import *
else:
    from vtkImagingPython import *
