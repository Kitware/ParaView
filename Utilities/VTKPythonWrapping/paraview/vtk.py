import os

if os.name == "posix":
    from libvtkCommonPython import *
    from libvtkFilteringPython import *
    from libvtkGraphicsPython import *
else:
    from vtkCommonPython import *
    from vtkFilteringPython import *
    from vtkGraphicsPython import *
