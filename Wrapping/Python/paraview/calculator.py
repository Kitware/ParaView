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
    # -- this will import vtkMultiProcessController and vtkMPI4PyCommunicator

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
            raise ValueError, \
                "Argument must be DataSetAttributes or CompositeDataSetAttributes."
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
        arraynames = arrays.keys()
        # gather to root and then broadcast
        # I couldn't get Allgather/Allreduce to work properly with strings.
        gathered_names = comm.gather(arraynames, root=0)
          # gathered_names is a list of lists.
        if rank == 0:
            result = set()
            for list in gathered_names:
                for val in list: result.add(val)
            gathered_names = [x for x in result]
        arraynames = comm.bcast(gathered_names, root=0)
        for name in arraynames:
            if not arrays.has_key(name):
                arrays[name] = dsa.NoneArray
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
