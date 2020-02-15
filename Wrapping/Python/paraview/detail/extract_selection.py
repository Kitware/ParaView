r"""This module is used by vtkPythonExtractSelection to extract query-based
selections. It relies on `paraview.detail.calculator`
to compute a mask array from the query expression. Once the mask array is obtained,
this filter will either extract the selected ids, or mark those elements as requested.
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

# this module is needed to ensure that python wrapping for
# `vtkPythonExtractSelection` is setup correctly.
from paraview.modules import vtkRemotingCore

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

def execute(self):
    inputDO = self.GetInputDataObject(0, 0)
    inputSEL = self.GetInputDataObject(1, 0)
    outputDO = self.GetOutputDataObject(0)

    assert inputSEL.GetNumberOfNodes() >= 1

    selectionNode = inputSEL.GetNode(0)
    field_type = selectionNode.GetFieldType()
    if field_type == selectionNode.CELL:
        attributeType = vtkDataObject.CELL
    elif field_type == selectionNode.POINT:
        attributeType = vtkDataObject.POINT
    elif field_type == selectionNode.ROW:
        attributeType = vtkDataObject.ROW
    else:
        raise RuntimeError ("Unsupported field attributeType %r" % field_type)

    # evaluate expression on the inputDO.
    # this is equivalent to executing the Python Calculator on the input dataset
    # to produce a mask array.
    inputs = []
    inputs.append(dsa.WrapDataObject(inputDO))

    query = selectionNode.GetQueryString()

    # get a dictionary for arrays in the dataset attributes. We pass that
    # as the variables in the eval namespace for calculator.compute().
    elocals = calculator.get_arrays(inputs[0].GetAttributes(attributeType))
    if ("id" not in elocals) and re.search(r'\bid\b', query):
        # add "id" array if the query string refers to id.
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

    # if inverse selection is requested, just logical_not the mask array.
    if selectionNode.GetProperties().Has(selectionNode.INVERSE()) and \
        selectionNode.GetProperties().Get(selectionNode.INVERSE()) == 1:
          maskArray = algos.logical_not(maskArray)

    output = dsa.WrapDataObject(outputDO)
    if self.GetPreserveTopology():
        # when preserving topology, just add the mask array as
        # vtkSignedCharArray to the output. vtkPythonExtractSelection should
        # have already ensured that the input is shallow copied over properly
        # before this method gets called.

        # Note: we must force the data type to VTK_SIGNED_CHAR or the array will
        # be ignored by the freeze selection operation
        from vtkmodules.util.numpy_support import numpy_to_vtk
        if type(maskArray) is not dsa.VTKNoneArray:
            insidedness = numpy_to_vtk(maskArray, deep=1, array_type=vtkConstants.VTK_SIGNED_CHAR)
            insidedness.SetName("vtkInsidedness")
            output.GetAttributes(attributeType).VTKObject.AddArray(insidedness)
    else:
        # handle extraction.
        # flatnonzero() will give is array of indices where the arrays is
        # non-zero (or non-False in our case). We then pass that to
        # vtkPythonExtractSelection to extract the selected ids.
        nonzero_indices =  algos.flatnonzero(maskArray)
        output.FieldData.append(nonzero_indices, "vtkSelectedIds");
        #print (output.FieldData["vtkSelectedIds"])
        self.ExtractElements(attributeType, inputDO, outputDO)
        del nonzero_indices
    del maskArray
