from paraview.vtk import dataset_adapter
import numpy
from paraview import servermanager
from paraview import numpy_support
from paraview import vtk

def global_min(narray):
    "Returns the minimum value of the given array among all process."
    local_min = min(narray)
    pm = servermanager.vtkProcessModule.GetProcessModule()
    if pm.GetNumberOfLocalPartitions == 1:
        return local_min
    else:
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        return comm.Allreduce(local_min, None, MPI.MIN)

def global_max(narray):
    "Returns the maximum value of the given array among all process."
    local_max = max(narray)
    pm = servermanager.vtkProcessModule.GetProcessModule()
    if pm.GetNumberOfLocalPartitions == 1:
        return local_max
    else:
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        return comm.Allreduce(local_max, None, MPI.MAX)

def dot(a1, a2):
    m = a1*a2
    va = dataset_adapter.VTKArray(numpy.add.reduce(m, 1))
    if a1.DataSet() == a2.DataSet():
        va.DataSet = a1.DataSet
    return va

def mag(a):
    return numpy.sqrt(dot(a, a))

def norm(a):
    return a/mag(a)

def min(narray):
    return numpy.min(numpy.array(narray))

def max(narray):
    return numpy.max(numpy.array(narray))

def divergence(narray, dataset=None):
    if not dataset:
        dataset = narray.DataSet()
    ncomp = narray.shape[1]
    if ncomp != 3:
        raise RuntimeError, 'Divergence only works with 3 component vectors'
    g = gradient(narray, dataset)
    g = g.reshape(g.shape[0], 3, 3)
    return dataset_adapter.VTKArray(numpy.add.reduce(g.diagonal(axis1=1, axis2=2), 1),\
        dataset=dataset)

def _cell_derivatives(narray, dataset, attribute_type, filter):
    # basic error checking
    if not dataset:
        raise RuntimeError, 'Need a dataset to compute gradients'
    if len(narray.shape) == 1:
        narray = narray.reshape((narray.shape[0], 1))
    if narray.shape[0] != dataset.GetNumberOfPoints():
        raise RuntimeError, 'The number of points does not match the number of tuples in the array'

    ncomp = narray.shape[1]
    if attribute_type == 'scalars' and ncomp != 1:
        raise RuntimeError, 'This function expects scalars.'
    if attribute_type == 'vectors' and ncomp != 3:
        raise RuntimeError, 'This function expects vectors.'

    # create a dataset with only our array but the same geometry/topology
    ds = dataset.NewInstance()
    ds.CopyStructure(dataset.VTKObject)
    # numpy_to_vtk converts only contiguous arrays
    if not narray.flags.contiguous:
        narray = narray.copy()
    varray = numpy_support.numpy_to_vtk(narray)

    if attribute_type == 'scalars':
        varray.SetName('scalars')
        ds.GetPointData().SetScalars(varray)
    else:
        varray.SetName('vectors')
        ds.GetPointData().SetVectors(varray)

    # setup and execute the pipeline
    # filter (vtkCellDerivatives) must  have all of its properties
    # set
    filter.SetInput(ds)
    ds.UnRegister(None)
    c2p = vtk.vtkCellDataToPointData()
    c2p.SetInputConnection(filter.GetOutputPort())
    c2p.Update()

    return c2p.GetOutput().GetPointData()

def curl(narray, dataset=None):

    if not dataset:
        dataset = narray.DataSet()

    cd = vtk.vtkCellDerivatives()
    cd.SetVectorModeToComputeVorticity()
    
    dsa = _cell_derivatives(narray, dataset, 'vectors', cd)

    retVal = dsa.GetVectors()
    retVal.SetName("vorticity")

    return dataset_adapter.vtkDataArrayToVTKArray(retVal, dataset)

def vorticity(narray, dataset=None):
    return curl(narray, dataset)

def strain(narray, dataset=None):

    if not dataset:
        dataset = narray.DataSet()

    cd = vtk.vtkCellDerivatives()
    cd.SetTensorModeToComputeStrain()
    
    dsa = _cell_derivatives(narray, dataset, 'vectors', cd)

    retVal = dsa.GetTensors()
    retVal.SetName("strain")

    return dataset_adapter.vtkDataArrayToVTKArray(retVal, dataset)

def gradient(narray, dataset=None):
    "Computes the gradient of a point-centered scalar array over a given dataset."
    if not dataset:
        dataset = narray.DataSet()
    if not dataset:
        raise RuntimeError, 'Need a dataset to compute gradients'
    ncomp = narray.shape[1]
    if ncomp != 1 and ncomp != 3:
        raise RuntimeError, 'Gradient only works with scalars (1 component) and vectors (3 component)'

    cd = vtk.vtkCellDerivatives()
    if ncomp == 1:
        attribute_type = 'scalars'
    else:
        attribute_type = 'vectors'

    dsa = _cell_derivatives(narray, dataset, attribute_type, cd)

    if ncomp == 1:
        retVal = dsa.GetVectors()
    else:
        retVal = dsa.GetTensors()

    try:
        if narray.GetName():
            retVal.SetName("gradient of " + narray.GetName())
        else:
            retVal.SetName("gradient")
    except AttributeError:
        retVal.SetName("gradient")

    return dataset_adapter.vtkDataArrayToVTKArray(retVal, dataset)

