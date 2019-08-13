r"""This module is used by vtkPythonCalculator. It encapsulates the logic
implemented by the vtkPythonCalculator to operate on datasets to compute
derived quantities.
"""

try:
  import numpy as np
except ImportError:
  raise RuntimeError ("'numpy' module is not found. numpy is needed for "\
    "this functionality to work. Please install numpy and try again.")

import paraview
import vtkmodules.numpy_interface.dataset_adapter as dsa
from vtkmodules.numpy_interface.algorithms import *
    # -- this will import vtkMultiProcessController and vtkMPI4PyCommunicator

from paraview.vtk import vtkDoubleArray, vtkSelectionNode, vtkSelection, vtkStreamingDemandDrivenPipeline
from paraview.modules import vtkPVClientServerCorePython

import sys
if sys.version_info >= (3,):
    xrange = range

def get_arrays(attribs, controller=None):
    """Returns a 'dict' referring to arrays in dsa.DataSetAttributes or
    dsa.CompositeDataSetAttributes instance.

    When running in parallel, this method will ensure that arraynames are
    reduced across all ranks and for any arrays missing on the local process, a
    NoneArray will be added to the returned dictionary. This ensures that
    expressions evaluate without issues due to missing arrays on certain ranks.
    """
    if not isinstance(attribs, dsa.DataSetAttributes) and \
        not isinstance(attribs, dsa.CompositeDataSetAttributes):
            raise ValueError (
                "Argument must be DataSetAttributes or CompositeDataSetAttributes.")
    arrays = dict()
    for key in attribs.keys():
        varname = paraview.make_name_valid(key)
        arrays[varname] = attribs[key]


    # If running in parallel, ensure that the arrays are synced up so that
    # missing arrays get NoneArray assigned to them avoiding any unnecessary
    # errors when evaluating expressions.
    if controller is None and vtkMultiProcessController is not None:
        controller = vtkMultiProcessController.GetGlobalController()
    if controller and controller.IsA("vtkMPIController") and controller.GetNumberOfProcesses() > 1:
        from mpi4py import MPI
        comm = vtkMPI4PyCommunicator.ConvertToPython(controller.GetCommunicator())
        rank = comm.Get_rank()

        # reduce the array names across processes to ensure arrays missing on
        # certain ranks are handled correctly.
        arraynames = list(arrays)  # get keys from the arrays as a list.
        # gather to root and then broadcast
        # I couldn't get Allgather/Allreduce to work properly with strings.
        gathered_names = comm.gather(arraynames, root=0)
          # gathered_names is a list of lists.
        if rank == 0:
            result = set()
            for alist in gathered_names:
                for val in alist: result.add(val)
            gathered_names = [x for x in result]
        arraynames = comm.bcast(gathered_names, root=0)
        for name in arraynames:
            if name not in arrays:
                arrays[name] = dsa.NoneArray
    return arrays

def pointIsNear(locations, distance, inputs):
    array = vtkDoubleArray()
    array.SetNumberOfComponents(3)
    array.SetNumberOfTuples(len(locations))
    for i in range(len(locations)):
        array.SetTuple(i, locations[i])
    node = vtkSelectionNode()
    node.SetFieldType(vtkSelectionNode.POINT)
    node.SetContentType(vtkSelectionNode.LOCATIONS)
    node.GetProperties().Set(vtkSelectionNode.EPSILON(), distance)
    node.SetSelectionList(array)

    from paraview.vtk.vtkFiltersExtraction import vtkLocationSelector
    selector = vtkLocationSelector()
    selector.SetInsidednessArrayName("vtkInsidedness")
    selector.Initialize(node)

    inputDO = inputs[0].VTKObject
    outputDO = inputDO.NewInstance()
    outputDO.CopyStructure(inputDO)

    output = dsa.WrapDataObject(outputDO)
    if outputDO.IsA('vtkCompositeDataSet'):
        it = inputDO.NewIterator()
        it.InitTraversal()
        while not it.IsDoneWithTraversal():
            outputDO.SetDataSet(it, inputDO.GetDataSet(it).NewInstance())
            it.GoToNextItem()
    selector.Execute(inputDO, outputDO)

    return output.PointData.GetArray('vtkInsidedness')

def cellContainsPoint(inputs, locations):
    array = vtkDoubleArray()
    array.SetNumberOfComponents(3)
    array.SetNumberOfTuples(len(locations))
    for i in range(len(locations)):
        array.SetTuple(i, locations[i])
    node = vtkSelectionNode()
    node.SetFieldType(vtkSelectionNode.CELL)
    node.SetContentType(vtkSelectionNode.LOCATIONS)
    node.SetSelectionList(array)

    from paraview.vtk.vtkFiltersExtraction import vtkLocationSelector
    selector = vtkLocationSelector()
    selector.SetInsidednessArrayName("vtkInsidedness")
    selector.Initialize(node)

    inputDO = inputs[0].VTKObject
    outputDO = inputDO.NewInstance()
    outputDO.CopyStructure(inputDO)

    output = dsa.WrapDataObject(outputDO)
    if outputDO.IsA('vtkCompositeDataSet'):
        it = inputDO.NewIterator()
        it.InitTraversal()
        while not it.IsDoneWithTraversal():
            outputDO.SetDataSet(it, inputDO.GetDataSet(it).NewInstance())
            it.GoToNextItem()
    selector.Execute(inputDO, outputDO)

    return output.CellData.GetArray('vtkInsidedness')

def compute(inputs, expression, ns=None):
    #  build the locals environment used to eval the expression.
    mylocals = dict()
    if ns:
        mylocals.update(ns)
    mylocals["inputs"] = inputs
    try:
        mylocals["points"] = inputs[0].Points
    except AttributeError: pass

    finalRet = None
    for subEx in expression.split(' and '):
        retVal = eval(subEx, globals(), mylocals)
        if finalRet is None:
            finalRet = retVal
        else:
            finalRet = dsa.VTKArray([a & b for a,b in zip(finalRet, retVal)])

    return finalRet

def get_data_time(self, do, ininfo):
    dinfo = do.GetInformation()
    if dinfo and dinfo.Has(do.DATA_TIME_STEP()):
        t = dinfo.Get(do.DATA_TIME_STEP())
    else: t = None

    key = vtkStreamingDemandDrivenPipeline.TIME_STEPS()
    t_index = None
    if ininfo.Has(key):
        tsteps = [ininfo.Get(key, x) for x in range(ininfo.Length(key))]
        try:
            t_index = tsteps.index(t)
        except ValueError:
            pass
    return (t, t_index)

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
        # wrap all input data objects using vtkmodules.numpy_interface.dataset_adapter
        wdo_input = dsa.WrapDataObject(self.GetInputDataObject(0, index))
        t, t_index = get_data_time(self, wdo_input.VTKObject, self.GetInputInformation(0, index))
        wdo_input.time_value = wdo_input.t_value = t
        wdo_input.time_index = wdo_input.t_index = t_index
        inputs.append(wdo_input)

    # Setup output.
    output = dsa.WrapDataObject(self.GetOutputDataObject(0))

    if self.GetCopyArrays():
        output.GetPointData().PassData(inputs[0].GetPointData())
        output.GetCellData().PassData(inputs[0].GetCellData())

    # get a dictionary for arrays in the dataset attributes. We pass that
    # as the variables in the eval namespace for compute.
    variables = get_arrays(inputs[0].GetAttributes(self.GetArrayAssociation()))
    variables.update({ "time_value": inputs[0].time_value,
                       "t_value": inputs[0].t_value,
                       "time_index": inputs[0].time_index,
                       "t_index": inputs[0].t_index })
    retVal = compute(inputs, expression, ns=variables)
    if retVal is not None:
        if hasattr(retVal, "Association"):
            output.GetAttributes(retVal.Association).append(\
              retVal, self.GetArrayName())
        else:
            # if somehow the association was removed we
            # fall back to the input array association
            output.GetAttributes(self.GetArrayAssociation()).append(\
              retVal, self.GetArrayName())
