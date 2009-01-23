import os

if os.name == "posix":
    from libvtkViewsPython import *
else:
    from vtkViewsPython import *
