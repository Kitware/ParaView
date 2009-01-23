import os

if os.name == "posix":
    from libvtkHybridPython import *
else:
    from vtkHybridPython import *
