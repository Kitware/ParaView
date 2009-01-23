import os

if os.name == "posix":
    from libvtkGenericFilteringPython import *
else:
    from vtkGenericFilteringPython import *
