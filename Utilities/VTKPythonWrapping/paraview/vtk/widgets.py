import os

if os.name == "posix":
    from libvtkWidgetsPython import *
else:
    from vtkWidgetsPython import *
