import paraview

try:
    from vtk.vtkCommonComputationalGeometry import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonComputationalGeometry")
from vtk.vtkCommonCore import *
from vtk.vtkCommonDataModel import *
from vtk.vtkCommonExecutionModel import *
try:
    from vtk.vtkCommonMath import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonMath")
try:
    from vtk.vtkCommonMisc import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonMisc")
try:
    from vtk.vtkCommonSystem import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonSystem")
try:
    from vtk.vtkCommonTransforms import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonTransforms")
from vtk.vtkFiltersProgrammable import *
from vtk.vtkParallelCore import *
try:
    from vtk.vtkRenderingCore import vtkCamera
except ImportError:
    paraview.print_error("Error: Could not import vtkRenderingCore")
try:
    from vtk.vtkFiltersCore import *
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
