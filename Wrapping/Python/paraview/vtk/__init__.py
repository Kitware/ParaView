import paraview

try:
    from vtkCommonComputationalGeometry import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonComputationalGeometry")
from vtkCommonCore import *
from vtkCommonDataModel import *
from vtkCommonExecutionModel import *
try:
    from vtkCommonMath import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonMath")
try:
    from vtkCommonMisc import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonMisc")
try:
    from vtkCommonSystem import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonSystem")
try:
    from vtkCommonTransforms import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonTransforms")
from vtkFiltersProgrammable import *
from vtkParallelCore import *
try:
    from vtkRenderingCore import vtkCamera
except ImportError:
    paraview.print_error("Error: Could not import vtkRenderingCore")
try:
    from vtkFiltersCore import *
except ImportError:
    paraview.print_error("Error: Could not import vtkFiltersCore")

# --------------------------------------

# useful macro for getting type names
__vtkTypeNameDict = {VTK_VOID:"void",
                     VTK_DOUBLE:"double",
                     VTK_FLOAT:"float",
                     VTK_LONG:"long",
                     VTK_UNSIGNED_LONG:"unsigned long",
                     VTK_INT:"int",
                     VTK_UNSIGNED_INT:"unsigned int",
                     VTK_SHORT:"short",
                     VTK_UNSIGNED_SHORT:"unsigned short",
                     VTK_CHAR:"char",
                     VTK_UNSIGNED_CHAR:"unsigned char",
                     VTK_SIGNED_CHAR:"signed char",
                     VTK_LONG_LONG:"long long",
                     VTK_UNSIGNED_LONG_LONG:"unsigned long long",
                     VTK___INT64:"__int64",
                     VTK_UNSIGNED___INT64:"unsigned __int64",
                     VTK_ID_TYPE:"vtkIdType",
                     VTK_BIT:"bit"}
