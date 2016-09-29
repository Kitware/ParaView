import paraview

try:
    from paraview.vtk.vtkCommonComputationalGeometry import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonComputationalGeometry")
from paraview.vtk.vtkCommonCore import *
from paraview.vtk.vtkCommonDataModel import *
from paraview.vtk.vtkCommonExecutionModel import *
try:
    from paraview.vtk.vtkCommonMath import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonMath")
try:
    from paraview.vtk.vtkCommonMisc import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonMisc")
try:
    from paraview.vtk.vtkCommonSystem import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonSystem")
try:
    from paraview.vtk.vtkCommonTransforms import *
except ImportError:
    paraview.print_error("Error: Could not import vtkCommonTransforms")
from paraview.vtk.vtkFiltersProgrammable import *
from paraview.vtk.vtkParallelCore import *
try:
    from paraview.vtk.vtkRenderingCore import vtkCamera
except ImportError:
    paraview.print_error("Error: Could not import vtkRenderingCore")
try:
    from paraview.vtk.vtkFiltersCore import *
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
