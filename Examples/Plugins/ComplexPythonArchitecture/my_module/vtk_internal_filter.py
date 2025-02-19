# This is a pure VTK file that define a filter.
# It can be used by the other `my_module` component but is not
# exposed outside (see __init__.py)

# base class for cany VTK Python filter.
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

# VTK wrapper to ease the use of numpy
from vtkmodules.numpy_interface import dataset_adapter as dsa

import numpy as np

class vtkMyInternalNumpyFilter(VTKPythonAlgorithmBase):
    """Compute derivated values from an input 'Elevation' array on a vtkPolyData.
    The computation is done using numpy and automatic wrapping of VTK objects"""
    def __init__(self):
        # define a standard filter with 1 input, 1 output and working on vtkPolyData
        super().__init__(nInputPorts=1, nOutputPorts=1, inputType="vtkPolyData", outputType="vtkPolyData")
        self._method = "gradient"

    # Standard entry point for actual data manipulation. Called by parent class
    def RequestData(self, request, inInfo, outInfo) -> int:
        """Compute derivated values: either gradient or cumulative sum, using numpy"""
        inData = self.GetInputData(inInfo, 0, 0)
        outData = self.GetOutputData(outInfo, 0)
        outData.ShallowCopy(inData)

        # Wrap them for numpy compatibility
        wrappedIn = dsa.WrapDataObject(inData)
        wrappedOut = dsa.WrapDataObject(outData)

        if "Elevation" not in wrappedIn.PointData.keys():
            print("elevation Array not found!")
            return 0

        elev = wrappedIn.PointData["Elevation"]

        valOfInterest = elev
        if self._method == "gradient":
            elevGradient = np.gradient(elev)
        else:
            valOfInterest = np.cumsum(elev)

        wrappedOut.PointData.append(valOfInterest, self._method)

        return 1

    # no decorators: we are doing pure VTK here
    def SetMethod(self, method: str):
        """Specify the method to use. Should be 'gradient' or 'cumsum'"""
        self._method = method
        self.Modified()

    # Utility method to reduce the verbosity on the caller side
    def GetOutput(self):
        """Return the output vtkPolyData"""
        return self.GetOutputDataObject(0)
