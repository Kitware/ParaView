r"""This module is used by vtkPythonExtractSelection to extract query-based
selections. It relies on the python-calculator (vtkPythonCalculator),
specifically, the Python code used by that class, to compute a mask array from
the query expression. Once the mask array is obtained, this filter will either
extract the selected ids, or mark those elements as requested.
"""
try:
  import numpy as np
except ImportError:
  raise RuntimeError, "'numpy' module is not found. numpy is needed for "\
    "this functionality to work. Please install numpy and try again."

import vtk
import vtk.numpy_interface.dataset_adapter as dsa
import vtk.numpy_interface.algorithms as algos
from paraview import calculator

def execute(self):
    inputDO = self.GetInputDataObject(0, 0)
    inputSEL = self.GetInputDataObject(1, 0)
    outputDO = self.GetOutputDataObject(0)

    assert inputSEL.GetNumberOfNodes() >= 1

    selectionNode = inputSEL.GetNode(0)
    field_type = selectionNode.GetFieldType()
    if field_type == selectionNode.CELL:
        attributeType = vtk.vtkDataObject.CELL
    elif field_type == selectionNode.POINT:
        attributeType = vtk.vtkDataObject.POINT
    elif field_type == selectionNode.ROW:
        attributeType = vtk.vtkDataObject.ROW
    else:
        raise RuntimeError, "Unsupported field attributeType %r" % field_type

    # evaluate expression on the inputDO.
    # this is equivalent to executing the Python Calculator on the input dataset
    # to produce a mask array.
    inputs = []
    inputs.append(dsa.WrapDataObject(inputDO))
    maskArray = calculator.compute(inputs, attributeType, selectionNode.GetQueryString())
    # TODO: add validation to ensure that maskArray is NoneArray or array(s) of
    # booleans.
    # print maskArray

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

        # note: since mask array is a bool-array, we multiply it by int8(1) to
        # make it a type of array that can be represented as vtkSignedCharArray.
        output.GetAttributes(attributeType).append(maskArray * np.int8(1), "vtkInsidedness")
    else:
        # handle extraction.
        # flatnonzero() will give is array of indices where the arrays is
        # non-zero (or non-False in our case). We then pass that to
        # vtkPythonExtractSelection to extract the selected ids.
        nonzero_indices =  algos.flatnonzero(maskArray)
        output.FieldData.append(nonzero_indices, "vtkSelectedIds");
        #print output.FieldData["vtkSelectedIds"]
        self.ExtractElements(attributeType, inputDO, outputDO)
        del nonzero_indices
    del maskArray
