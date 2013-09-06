import paraview

try:
    from vtkCommonComputationalGeometryPython import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonComputationalGeometryPython")
from vtkCommonCorePython import *
from vtkCommonDataModelPython import *
from vtkCommonExecutionModelPython import *
try:
    from vtkCommonMathPython import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonMathPython")
try:
    from vtkCommonMiscPython import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonMiscPython")
try:
    from vtkCommonSystemPython import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonSystemPython")
try:
    from vtkCommonTransformsPython import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonTransformsPython")
from vtkFiltersProgrammablePython import *
from vtkParallelCorePython import *
try:
    from vtkRenderingCorePython import vtkCamera
except ImportError:
    paraview.print_error("Error: Could not import vtkRenderingCorePython")
try:
    from vtkFiltersCorePython import *
except ImportError:
    paraview.print_error("Error: Could not import vtkFiltersCorePython")

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
