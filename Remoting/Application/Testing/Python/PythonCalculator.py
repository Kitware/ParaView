# Tests the PythonCalculator with varies expressions.

import math
from paraview.simple import *
from vtkmodules.vtkCommonDataModel import vtkDataObject

# Field data output from point data array tests
source = Wavelet()
calculator = PythonCalculator(Input=source, ArrayAssociation='Field Data', ArrayName='Result')

# min operator
expression = "min(inputs[0].PointData['RTData'])"
calculator.Expression = expression
calculator.UpdatePipeline()

data_info = calculator.GetDataInformation()
array_info = data_info.GetArrayInformation('Result', vtkDataObject.FIELD)
assert array_info is not None, \
    "Expected array information for 'Result', but got None"
num_array_components = array_info.GetNumberOfComponents()
assert num_array_components == 1, \
    "Expected the 'Result' array to have 1 component, but got {}".format(num_array_components)
component_range = array_info.GetComponentRange(0)
assert math.isclose(component_range[0], 37.35310363769531), \
    "Expected the minimum value of the 'Result' array to be 37.35310363769531, but got {}".format(component_range[0])

# max operator
expression = "max(inputs[0].PointData['RTData'])"
calculator.Expression = expression
calculator.UpdatePipeline()

data_info = calculator.GetDataInformation()
array_info = data_info.GetArrayInformation('Result', vtkDataObject.FIELD)
assert array_info is not None, \
    "Expected array information for 'Result', but got None"
num_array_components = array_info.GetNumberOfComponents()
assert num_array_components == 1, \
    "Expected the 'Result' array to have 1 component, but got {}".format(num_array_components)
component_range = array_info.GetComponentRange(0)
assert math.isclose(component_range[1], 276.8288269042969), \
    "Expected the maximum value of the 'Result' array to be 276.8288269042969, but got {}".format(component_range[1])
