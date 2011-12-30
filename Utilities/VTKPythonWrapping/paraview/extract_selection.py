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

def PassBlock(self, iterCD):
    """Test if the block passes the block-criteria, if any"""
    return True

def ExtractElements(self, inputDS, mask):
    if type(mask) == bool:
        if mask:
            # FIXME: We need to add the "vtkOriginalIds" array.
            return inputDS
        else:
            # nothing was extracted.
            return None
    else:
        # mask must be an array. Process it.
        mask_array = dataset_adapter.numpyTovtkDataArray(int8(mask), "_mask_array")
        retVal = self.ExtractElements(inputDS, mask_array)
        if retVal:
            retVal.UnRegister(None)
            return retVal
    return None

def ExecData(self, inputDS):
    """inputDS is either a non-composite data object"""
    # wrap the data objects. makes them easier to use.
    do = dataset_adapter.WrapDataObject(inputDS)
    dsa = dataset_adapter.DataSetAttributes(
      inputDS.GetAttributes(self.GetArrayAssociation()),
      do, self.GetArrayAssociation())

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
        self.GetArrayAssociation()))

    # evaluate the query expression. The expression should return a mask which
    # is either an array or a boolean value.
    mask = eval(self.GetExpression(), globals(), new_locals)

    print mask

    # extract the elements from the input dataset using the mask.
    extracted_ds = ExtractElements(self, inputDS, mask)

    del mask
    del new_locals
    del do
    del dsa
    return extracted_ds

def Exec(self, inputDO, outputDO):
    if inputDO.IsA("vtkCompositeDataSet"):
        outputDO.CopyStructure(inputDO)

        # For composite datasets, iterate over the tree and call ExecData() only
        # for this nodes that pass the block-criteria, if any.
        iterCD = inputDO.NewIterator()
        iterCD.UnRegister(None)
        while not iterCD.IsDoneWithTraversal():
            if PassBlock(self, iterCD):
                ds = ExecData(self, iterCD.GetCurrentDataObject())
                outputDO.SetDataSet(iterCD, ds)
                del ds
            iterCD.GoToNextItem()
        del iterCD
    else:
      ds = ExecData(self, inputDO)
      if ds:
        outputDO.ShallowCopy(ds)
      del ds
    return True
