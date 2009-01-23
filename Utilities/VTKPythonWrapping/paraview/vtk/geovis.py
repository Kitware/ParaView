import os

if os.name == "posix":
    from libvtkCommonPython import *
else:
    from vtkCommonPython import *
