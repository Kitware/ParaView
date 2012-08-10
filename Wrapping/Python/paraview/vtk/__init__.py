from vtkCommonCorePython import *
from vtkCommonMathPython import *
from vtkCommonMiscPython import *
from vtkCommonSystemPython import *
from vtkCommonTransformsPython import *
from vtkCommonDataModelPython import *
from vtkCommonExecutionModelPython import *
from vtkCommonComputationalGeometryPython import *
from vtkRenderingCorePython import vtkCamera


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
