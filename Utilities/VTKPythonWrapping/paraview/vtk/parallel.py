import os

if os.name == "posix":
    from libvtkParallelPython import *
else:
    from vtkParallelPython import *
