"""This module is used by vtkPythonSelector to perform query-based
selections. It relies on the python-calculator (vtkPythonCalculator),
specifically, the Python code used by that class, to compute a mask array from
the query expression. Once the mask array is obtained, this module will
mark those elements as requested.
"""
from __future__ import absolute_import, print_function
try:
  import numpy as np
except ImportError:
  raise RuntimeError ("'numpy' module is not found. numpy is needed for "\
    "this functionality to work. Please install numpy and try again.")

import re
import vtkmodules.numpy_interface.dataset_adapter as dsa
import vtkmodules.numpy_interface.algorithms as algos
from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.util import vtkConstants
from . import calculator

# import wrapping module for `vtkPythonSelector`
from paraview.modules import vtkPVClientServerCoreCore

import sys
if sys.hexversion < 0x03000000:
    import itertools
    izip = itertools.izip
else:
    izip = zip

def _create_id_array(dataobject, attributeType):
    """Returns a VTKArray or VTKCompositeDataArray for the ids"""
    if not dataobject:
        raise RuntimeError ("dataobject cannot be None")
    if dataobject.IsA("vtkCompositeDataSet"):
        ids = []
        for ds in dataobject:
            ids.append(_create_id_array(ds, attributeType))
        return dsa.VTKCompositeDataArray(ids)
    else:
        return dsa.VTKArray(\
                np.arange(dataobject.GetNumberOfElements(attributeType)))

def maskarray_is_valid(maskArray):
    """Validates that the maskArray is either a VTKArray or a
    VTKCompositeDataArrays or a NoneArray other returns false."""
    return maskArray is dsa.NoneArray or \
        isinstance(maskArray, dsa.VTKArray) or \
        isinstance(maskArray, dsa.VTKCompositeDataArray)

def execute(inputDO, selectionNode, insidednessArrayName, outputDO):
    field_type = selectionNode.GetFieldType()
    if field_type == selectionNode.CELL:
        attributeType = vtkDataObject.CELL
    elif field_type == selectionNode.POINT:
        attributeType = vtkDataObject.POINT
    elif field_type == selectionNode.ROW:
        attributeType = vtkDataObject.ROW
    else:
        raise RuntimeError ("Unsupported field attributeType %r" % field_type)
    # Evaluate expression on the inputDO.
    # This is equivalent to executing the Python Calculator on the input dataset
    # to produce a mask array.

    inputs = []
    inputs.append(dsa.WrapDataObject(inputDO))

    query = selectionNode.GetQueryString()

    # Get a dictionary for arrays in the dataset attributes. We pass that
    # as the variables in the eval namespace for calculator.compute().
    elocals = calculator.get_arrays(inputs[0].GetAttributes(attributeType))
    if ("id" not in elocals) and re.search(r'\bid\b', query):
        # Add "id" array if the query string refers to id.
        # This is a temporary fix. We should look into
        # accelerating id-based selections in the future.
        elocals["id"] = _create_id_array(inputs[0], attributeType)
    try:
        maskArray = calculator.compute(inputs, query, ns=elocals)
    except:
        from sys import stderr
        print ("Error: Failed to evaluate Expression '%s'. "\
            "The following exception stack should provide additional developer "\
            "specific information. This typically implies a malformed "\
            "expression. Verify that the expression is valid.\n" % query, file=stderr)
        raise

    if not maskarray_is_valid(maskArray):
        raise RuntimeError(
            "Expression '%s' did not produce a valid mask array. The value "\
            "produced is of the type '%s'. This typically implies a malformed "\
            "expression. Verify that the expression is valid." % \
            (query, type(maskArray)))

    # Preserve topology. Just add the mask array as vtkSignedCharArray to the
    # output.
    # Note: we must force the data type to VTK_SIGNED_CHAR or the array will
    # be ignored by the freeze selection operation
    from paraview.vtk.util import numpy_support
    output = dsa.WrapDataObject(outputDO)
    if type(maskArray) is not dsa.VTKNoneArray:
        if isinstance(maskArray, dsa.VTKCompositeDataArray):
            for ds, array in izip(output, maskArray.Arrays):
                if array is not None:
                    insidedness = numpy_support.numpy_to_vtk(array, deep=1, array_type=vtkConstants.VTK_SIGNED_CHAR)
                    insidedness.SetName(insidednessArrayName)
                    ds.GetAttributes(attributeType).VTKObject.AddArray(insidedness)
        else:
            insidedness = numpy_support.numpy_to_vtk(maskArray, deep=1, array_type=vtkConstants.VTK_SIGNED_CHAR)
            insidedness.SetName(insidednessArrayName)
            output.GetAttributes(attributeType).VTKObject.AddArray(insidedness)
