r"""This module is used by vtkPythonCalculator. It encapsulates the logic
implemented by the vtkPythonCalculator to operate on datasets to compute
derived quantities.
"""

try:
  import numpy as np
except ImportError:
  raise RuntimeError, "'numpy' module is not found. numpy is needed for "\
    "this functionality to work. Please install numpy and try again."

import paraview
import vtk.numpy_interface.dataset_adapter as dsa
from vtk.numpy_interface.algorithms import *

def get_arrays(attribs):
    """Returns a 'dict' referring to arrays in dsa.DataSetAttributes or
    dsa.CompositeDataSetAttributes instance."""
    if not isinstance(attribs, dsa.DataSetAttributes) and \
        not isinstance(attribs, dsa.CompositeDataSetAttributes):
            raise ValueError, \
                "Argument must be DataSetAttributes or CompositeDataSetAttributes."
    arrays = dict()
    for key in attribs.keys():
        varname = paraview.make_name_valid(key)
        arrays[varname] = attribs[key]
    return arrays


def compute(inputs, expression, ns=None):
    #  build the locals environment used to eval the expression.
    mylocals = dict()
    if ns:
        mylocals.update(ns)
    mylocals["inputs"] = inputs
    try:
        mylocals["points"] = inputs[0].Points
    except AttributeError: pass
    retVal = eval(expression, globals(), mylocals)
    return retVal

def execute(self, expression):
    """
    **Internal Method**
    Called by vtkPythonCalculator in its RequestData(...) method. This is not
    intended for use externally except from within
    vtkPythonCalculator::RequestData(...).
    """

    # Add inputs.
    inputs = []

    for index in range(self.GetNumberOfInputConnections(0)):
        # wrap all input data objects using vtk.numpy_interface.dataset_adapter
        inputs.append(dsa.WrapDataObject(self.GetInputDataObject(0, index)))

    # Setup output.
    output = dsa.WrapDataObject(self.GetOutputDataObject(0))

    if self.GetCopyArrays():
        output.GetPointData().PassData(inputs[0].GetPointData())
        output.GetCellData().PassData(inputs[0].GetCellData())

    # get a dictionary for arrays in the dataset attributes. We pass that
    # as the variables in the eval namespace for compute.
    variables = get_arrays(inputs[0].GetAttributes(self.GetArrayAssociation()))
    retVal = compute(inputs, expression, ns=variables)
    if retVal is not None:
        output.GetAttributes(self.GetArrayAssociation()).append(\
            retVal, self.GetArrayName())
