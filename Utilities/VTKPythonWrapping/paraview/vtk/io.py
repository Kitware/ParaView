import os

if os.name == "posix":
    from libvtkIOPython import *
else:
    from vtkIOPython import *
