from paraview.vtk import dataset_adapter
import numpy
from paraview import servermanager
from paraview import numpy_support
from paraview import vtk

def global_min(narray):
    "Returns the minimum value of the given array among all process."
    local_min = numpy.min(narray)
    pm = servermanager.vtkProcessModule.GetProcessModule()
    if pm.GetNumberOfLocalPartitions == 1:
        return local_min
    else:
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        return comm.Allreduce(local_min, None, MPI.MIN)

def global_max(narray):
    "Returns the maximum value of the given array among all process."
    local_max = numpy.max(narray)
    pm = servermanager.vtkProcessModule.GetProcessModule()
    if pm.GetNumberOfLocalPartitions == 1:
        return local_max
    else:
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        return comm.Allreduce(local_max, None, MPI.MAX)

def gradient(narray, dataset=None):
    "Computes the gradient of a point-centered scalar array over a given dataset."
    if not dataset:
        dataset = narray.DataSet
    if not dataset:
        raise RuntimeError, 'Need a dataset to compute gradients'
    if len(narray.shape) == 1:
        narray = narray.reshape((narray.shape[0], 1))
    if narray.shape[0] != narray.GetNumberOfTuples():
        raise RuntimeError, 'The number of points does not match the number of tuples in the array'
    if narray.shape[1] > 1:
        raise RuntimeError, 'Gradient only works with 1 component arrays'

    ds = dataset.NewInstance()
    ds.CopyStructure(dataset.VTKObject)
    # numpy_to_vtk converts only contiguous arrays
    if not narray.flags.contiguous:
        narray = narray.copy()
    varray = numpy_support.numpy_to_vtk(narray)
    varray.SetName('scalars')
    ds.GetPointData().SetScalars(varray)
    cd = vtk.vtkCellDerivatives()
    cd.SetInput(ds)
    ds.UnRegister(None)
    c2p = vtk.vtkCellDataToPointData()
    c2p.SetInputConnection(cd.GetOutputPort())
    c2p.Update()

    retVal = c2p.GetOutput().GetPointData().GetVectors()
    try:
        if narray.GetName():
            retVal.SetName("gradient of " + narray.GetName())
        else:
            retVal.SetName("gradient")
    except AttributeError:
        retVal.SetName("gradient")

    return dataset_adapter.vtkDataArrayToVTKArray(retVal, dataset)

