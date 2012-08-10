#==============================================================================
#
#  Program:   ParaView
#  Module:    extract_selection.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================
r"""
This module is used by vtkPythonExtractSelection filter.
"""
import paraview
from paraview.vtk import dataset_adapter
from numpy import *
from paraview.vtk.algorithms import *
from paraview import servermanager
if servermanager.progressObserverTag:
    servermanager.ToggleProgressPrinting()

def __vtk_in1d(a, b):
    return array([item in b for item in a])

try:
    contains = in1d
except NameError:
    # older versions of numpy don't have in1d function.
    # in1d was introduced in numpy 1.4.0.
    contains = __vtk_in1d

#class _array(object):
#    """used to wrap numpy array to add support for == operator
#       that compares two array to generate a mask using numpy.in1d() method"""
#    def __init__(self, array):
#        self.array = array
#
#    def __eq__(self, other):
#        if type(other) == ndarray or type(other) == list:
#          return in1d(self.array, other)
#        return self.array == other

def PassBlock(self, iterCD, selection_node):
    """Test if the block passes the block-criteria, if any"""
    props = selection_node.GetProperties()

    if iterCD.IsA("vtkHierarchicalBoxDataIterator"):
        if props.Has(selection_node.HIERARCHICAL_INDEX()):
            if iterCD.GetCurrentIndex() != props.Get(selection_node.HIERARCHICAL_INDEX()):
                return False

        if props.Has(selection_node.HIERARCHICAL_LEVEL()):
            if iterCD.GetCurrentLevel() != props.Get(selection_node.HIERARCHICAL_LEVEL()):
                return False
    elif iterCD.IsA("vtkCompositeDataIterator"):
        if props.Has(selection_node.COMPOSITE_INDEX()):
            if iterCD.GetCurrentFlatIndex() != props.Get(selection_node.COMPOSITE_INDEX()):
                return False

    return True

def ExtractElements(self, inputDS, selection, mask):
    if mask is None:
        # nothing was selected
        return None
    elif type(mask) == bool:
        if mask:
            # FIXME: We need to add the "vtkOriginalIds" array.
            return inputDS
        else:
            # nothing was extracted.
            return None
    else:
        # mask must be an array. Process it.
        mask_array = dataset_adapter.numpyTovtkDataArray(int8(mask), "_mask_array")
        retVal = self.ExtractElements(inputDS, selection, mask_array)
        if retVal:
            retVal.UnRegister(None)
            return retVal
    return None

def ExecData(self, inputDS, selection):
    """inputDS is either a non-composite data object"""

    selection_node = selection.GetNode(0)
    array_association = 1

    # convert from vtkSelectionNode's field type to vtkDataObject's field association
    if(selection_node.GetFieldType() == 0):
      array_association = 1
    elif(selection_node.GetFieldType() == 1):
      array_association = 0

    # wrap the data objects. makes them easier to use.
    do = dataset_adapter.WrapDataObject(inputDS)
    dsa = dataset_adapter.DataSetAttributes(
      inputDS.GetAttributes(array_association),
      do, array_association)

    new_locals = {}
    # define global variables for all the arrays.
    for arrayname in dsa.keys():
        name = paraview.make_name_valid(arrayname)
        new_locals[name] = dsa[arrayname]

    new_locals["cell"] = do
    new_locals["dataset"] = do
    new_locals["input"] = do
    new_locals["element"] = do
    new_locals["id"] = arange(inputDS.GetNumberOfElements(
        array_association))

    # evaluate the query expression. The expression should return a mask which
    # is either an array or a boolean value.
    mask = None

    try:
      mask = eval(selection_node.GetQueryString(), globals(), new_locals)
    except NameError:
      pass

    # extract the elements from the input dataset using the mask.
    extracted_ds = ExtractElements(self, inputDS, selection, mask)

    del mask
    del new_locals
    del do
    del dsa
    return extracted_ds

def Exec(self, inputDO, selection, outputDO):
    selection_node = selection.GetNode(0)

    if inputDO.IsA("vtkCompositeDataSet"):
        outputDO.CopyStructure(inputDO)

        # For composite datasets, iterate over the tree and call ExecData() only
        # for this nodes that pass the block-criteria, if any.
        iterCD = inputDO.NewIterator()
        iterCD.UnRegister(None)
        while not iterCD.IsDoneWithTraversal():
            if PassBlock(self, iterCD, selection_node):
                ds = ExecData(self, iterCD.GetCurrentDataObject(), selection)
                outputDO.SetDataSet(iterCD, ds)
                del ds
            iterCD.GoToNextItem()
        del iterCD
    else:
      ds = ExecData(self, inputDO, selection)
      if ds:
        outputDO.ShallowCopy(ds)
      del ds
    return True
