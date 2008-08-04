import os

if os.name == "posix":
    from libvtkCommonPython import *
    from libvtkFilteringPython import *
    from libvtkGraphicsPython import *
    from libvtkRenderingPython import vtkCamera
else:
    from vtkCommonPython import *
    from vtkFilteringPython import *
    from vtkGraphicsPython import *
    from vtkRenderingPython import vtkCamera
