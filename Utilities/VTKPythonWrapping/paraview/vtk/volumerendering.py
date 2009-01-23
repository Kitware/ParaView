import os

if os.name == "posix":
    from libvtkVolumeRenderingPython import *
else:
    from vtkVolumeRenderingPython import *
