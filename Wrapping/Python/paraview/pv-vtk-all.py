import paraview
from . import logger

from vtkmodules.vtkCommonCore import *
from vtkmodules.vtkCommonDataModel import *
from vtkmodules.vtkCommonExecutionModel import *
from vtkmodules.vtkFiltersProgrammable import *
from vtkmodules.vtkParallelCore import *
from vtkmodules.vtkCommonMisc import *
from vtkmodules.vtkCommonMath import *
from vtkmodules.vtkCommonSystem import *
from vtkmodules.vtkCommonTransforms import *

# now import modules that need not be present
try:
    from vtkmodules.vtkCommonComputationalGeometry import *
except ImportError:
    logger.debug("Error: Could not import vtkCommonComputationalGeometry")
try:
    from vtkmodules.vtkRenderingCore import vtkCamera
except ImportError:
    logger.debug("Error: Could not import vtkRenderingCore")
try:
    from vtkmodules.vtkFiltersCore import *
except ImportError:
    logger.debug("Error: Could not import vtkFiltersCore")
