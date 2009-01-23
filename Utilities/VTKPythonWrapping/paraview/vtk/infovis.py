import os

if os.name == "posix":
    from libvtkInfovisPython import *
else:
    from vtkInfovisPython import *
