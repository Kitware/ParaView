from paraview.vtk import dataset_adapter
import numpy
from paraview import numpy_support
from paraview import vtk
from vtkPVClientServerCoreCorePython import vtkProcessModule

from vtkFiltersCorePython import vtkPolyDataNormals, vtkCellDataToPointData
from vtkFiltersVerdictPython import vtkCellQuality, vtkMatrixMathFilter
from vtkFiltersGeneralPython import vtkCellDerivatives

def _cell_derivatives (narray, dataset, attribute_type, filter):
    if not dataset :
       raise RuntimeError, 'Need a dataset to compute _cell_derivatives.'

    # Reshape n dimensional vector to n by 1 matrix
    if len(narray.shape) == 1 :
       narray = narray.reshape((narray.shape[0], 1))

    ncomp = narray.shape[1]
    if attribute_type == 'scalars' and ncomp != 1 :
       raise RuntimeError, 'This function expects scalars.'\
                           'Input shape ' + narray.shape
    if attribute_type == 'vectors' and ncomp != 3 :
       raise RuntimeError, 'This function expects vectors.'\
                           'Input shape ' + narray.shape

    # numpy_to_vtk converts only contiguous arrays
    if not narray.flags.contiguous : narray = narray.copy()
    varray = numpy_support.numpy_to_vtk(narray)

    if attribute_type == 'scalars': varray.SetName('scalars')
    else : varray.SetName('vectors')

    # create a dataset with only our array but the same geometry/topology
    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.CopyStructure(dataset.VTKObject)

    if dataset_adapter.ArrayAssociation.FIELD == narray.Association :
       raise RuntimeError, 'Unknown data association. Data should be associated with points or cells.'

    if dataset_adapter.ArrayAssociation.POINT == narray.Association :
       # Work on point data
       if narray.shape[0] != dataset.GetNumberOfPoints() :
          raise RuntimeError, 'The number of points does not match the number of tuples in the array'
       if attribute_type == 'scalars': ds.GetPointData().SetScalars(varray)
       else : ds.GetPointData().SetVectors(varray)
    elif dataset_adapter.ArrayAssociation.CELL == narray.Association :
       # Work on cell data
       if narray.shape[0] != dataset.GetNumberOfCells() :
          raise RuntimeError, 'The number of does not match the number of tuples in the array'

       # Since vtkCellDerivatives only works with point data, we need to convert
       # the cell data to point data first.

       ds2 = dataset.NewInstance()
       ds2.UnRegister(None)
       ds2.CopyStructure(dataset.VTKObject)

       if attribute_type == 'scalars' : ds2.GetCellData().SetScalars(varray)
       else : ds2.GetCellData().SetVectors(varray)

       c2p = vtkCellDataToPointData()
       c2p.SetInputData(ds2)
       c2p.Update()

       # Set the output to the ds dataset
       if attribute_type == 'scalars':
          ds.GetPointData().SetScalars(c2p.GetOutput().GetPointData().GetScalars())
       else:
          ds.GetPointData().SetVectors(c2p.GetOutput().GetPointData().GetVectors())

    filter.SetInputData(ds)

    if dataset_adapter.ArrayAssociation.POINT == narray.Association :
       # Since the data is associated with cell and the query is on points
       # we have to convert to point data before returning
       c2p = vtkCellDataToPointData()
       c2p.SetInputConnection(filter.GetOutputPort())
       c2p.Update()
       return c2p.GetOutput().GetPointData()
    elif dataset_adapter.ArrayAssociation.CELL == narray.Association :
       filter.Update()
       return filter.GetOutput().GetCellData()
    else :
       # We shall never reach here
       raise RuntimeError, 'Unknown data association. Data should be associated with points or cells.'

def _cell_quality (dataset, quality) :
    if not dataset : raise RuntimeError, 'Need a dataset to compute _cell_quality'

    # create a dataset with only our array but the same geometry/topology
    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.CopyStructure(dataset.VTKObject)

    filter = vtkCellQuality()
    filter.SetInputData(ds)

    if   "area"         == quality : filter.SetQualityMeasureToArea()
    elif "aspect"       == quality : filter.SetQualityMeasureToAspectRatio()
    elif "aspect_gamma" == quality : filter.SetQualityMeasureToAspectGamma()
    elif "condition"    == quality : filter.SetQualityMeasureToCondition()
    elif "diagonal"     == quality : filter.SetQualityMeasureToDiagonal()
    elif "jacobian"     == quality : filter.SetQualityMeasureToJacobian()
    elif "max_angle"    == quality : filter.SetQualityMeasureToMaxAngle()
    elif "shear"        == quality : filter.SetQualityMeasureToShear()
    elif "skew"         == quality : filter.SetQualityMeasureToSkew()
    elif "min_angle"    == quality : filter.SetQualityMeasureToMinAngle()
    elif "volume"       == quality : filter.SetQualityMeasureToVolume()
    else : raise RuntimeError, 'Unknown cell quality ['+quality+'].'

    filter.Update()

    varray = filter.GetOutput().GetCellData().GetArray("CellQuality")
    ans = dataset_adapter.vtkDataArrayToVTKArray(varray, dataset)

    # The association information has been lost over the vtk filter
    # we must reconstruct it otherwise lower pipeline will be broken.
    ans.Association = dataset_adapter.ArrayAssociation.CELL

    return ans

def _matrix_math_filter (narray, operation) :
    if operation not in ['Determinant', 'Inverse', 'Eigenvalue', 'Eigenvector'] :
       raise RuntimeError, 'Unknown quality measure ['+operation+']'+\
                           'Supported are [Determinant, Inverse, Eigenvalue, Eigenvector]'

    dataset = narray.DataSet()
    if not dataset : raise RuntimeError, 'narray is not associated with a dataset.'

    if dataset_adapter.ArrayAssociation.FIELD == narray.Association :
       raise RuntimeError, 'Unknown data association. Data should be associated with points or cells.'

    if narray.ndim != 3 :
       raise RuntimeError, operation+' only works for an array of matrices(3D array).'\
                           'Input shape ' + narray.shape
    elif narray.shape[1] != narray.shape[2] :
       raise RuntimeError, operation+' requires an array of 2D square matrices.'\
                           'Input shape ' + narray.shape

    # numpy_to_vtk converts only contiguous arrays
    if not narray.flags.contiguous : narray = narray.copy()

    # Reshape is necessary because numpy_support.numpy_to_vtk only works with 2D or
    # less arrays.
    nrows = narray.shape[0]
    ncols = narray.shape[1] * narray.shape[2]
    narray = narray.reshape(nrows, ncols)

    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.ShallowCopy(dataset.VTKObject)

    varray = numpy_support.numpy_to_vtk(narray)
    varray.SetName('tensors')

    filter = vtkMatrixMathFilter()

    if   operation == 'Determinant'  : filter.SetOperationToDeterminant()
    elif operation == 'Inverse'      : filter.SetOperationToInverse()
    elif operation == 'Eigenvalue'   : filter.SetOperationToEigenvalue()
    elif operation == 'Eigenvector'  : filter.SetOperationToEigenvector()

    if dataset_adapter.ArrayAssociation.POINT == narray.Association :
       ds.GetPointData().SetTensors(varray)
       # filter.SetQualityTypeToPointQuality()
    elif dataset_adapter.ArrayAssociation.CELL == narray.Association :
       ds.GetCellData().SetTensors(varray)
       # filter.SetQualityTypeToCellQuality()

    filter.SetInputData(ds)
    filter.Update()

    if dataset_adapter.ArrayAssociation.POINT == narray.Association :
       varray = filter.GetOutput().GetPointData().GetArray(operation)
    elif dataset_adapter.ArrayAssociation.CELL == narray.Association :
       varray = filter.GetOutput().GetCellData().GetArray(operation)

    ans = dataset_adapter.vtkDataArrayToVTKArray(varray, dataset)

    # The association information has been lost over the vtk filter
    # we must reconstruct it otherwise lower pipeline will be broken.
    ans.Association = narray.Association

    return ans

# Python interfaces
def abs (narray) :
    "Returns the absolute values of an array of scalars/vectors/tensors."
    return numpy.abs(narray)

def area (dataset) :
    "Returns the surface area of each cell in a mesh."
    return _cell_quality(dataset, "area")

def aspect (dataset) :
    "Returns the aspect ratio of each cell in a mesh."
    return _cell_quality(dataset, "aspect")

def aspect_gamma (dataset) :
    "Returns the aspect ratio gamma of each cell in a mesh."
    return _cell_quality(dataset, "aspect_gamma")

def condition (dataset) :
    "Returns the condition number of each cell in a mesh."
    return _cell_quality(dataset, "condition")

def cross (x, y) :
    "Return the cross product for two 3D vectors from two arrays of 3D vectors."
    if x.ndim != y.ndim or x.shape != y.shape:
       raise RuntimeError, 'Both operands must have same dimension and shape.'\
                           'Input shapes ' + x.shape + ' and ' + y.shape

    if x.ndim != 1 and x.ndim != 2 :
       raise RuntimeError, 'Cross only works for 3D vectors or an array of 3D vectors.'\
                           'Input shapes ' + x.shape + ' and ' + y.shape

    if x.ndim == 1 and x.shape[0] != 3 :
       raise RuntimeError, 'Cross only works for 3D vectors.'\
                           'Input shapes ' + x.shape + ' and ' + y.shape

    if x.ndim == 2 and x.shape[1] != 3 :
       raise RuntimeError, 'Cross only works for an array of 3D vectors.'\
                           'Input shapes ' + x.shape + ' and ' + y.shape

    return numpy.cross(x, y)

def curl (narray, dataset=None):
    "Returns the curl of an array of 3D vectors."
    if not dataset : dataset = narray.DataSet()
    if not dataset : raise RuntimeError, 'Need a dataset to compute curl.'

    if narray.ndim != 2 or narray.shape[1] != 3 :
       raise RuntimeError, 'Curl only works with an array of 3D vectors.'\
                           'Input shape ' + narray.shape

    cd = vtkCellDerivatives()
    cd.SetVectorModeToComputeVorticity()

    dsa = _cell_derivatives(narray, dataset, 'vectors', cd)

    retVal = dsa.GetVectors()
    retVal.SetName("vorticity")

    ans = dataset_adapter.vtkDataArrayToVTKArray(retVal, dataset)

    # The association information has been lost over the vtk filter
    # we must reconstruct it otherwise lower pipeline will be broken.
    ans.Association = narray.Association

    return ans

def divergence (narray, dataset=None):
    "Returns the divergence of an array of 3D vectors."
    if not dataset : dataset = narray.DataSet()
    if not dataset : raise RuntimeError, 'Need a dataset to compute divergence'

    if narray.ndim != 2 or narray.shape[1] != 3 :
       raise RuntimeError, 'Divergence only works with an array of 3D vectors.'\
                           'Input shape ' + narray.shape

    g = gradient(narray, dataset)
    g = g.reshape(g.shape[0], 3, 3)

    return dataset_adapter.VTKArray\
           (numpy.add.reduce(g.diagonal(axis1=1, axis2=2), 1), dataset=g.DataSet())

def det (narray) :
    "Returns the determinant of an array of 2D square matrices."
    return _matrix_math_filter(narray, "Determinant")

def determinant (narray) :
    "Returns the determinant of an array of 2D square matrices."
    return det(narray)

def diagonal (dataset) :
    "Returns the diagonal length of each cell in a dataset."
    return _cell_quality(dataset, "diagonal")

def dot (a1, a2):
    "Returns the dot product of two scalars/vectors of two array of scalars/vectors."
    if a1.shape[1] != a2.shape[1] :
     raise RuntimeError, 'Dot product only works with vectors of same dimension.'\
                         'Input shapes ' + a1.shape + ' and ' + a2.shape
    m = a1*a2
    va = dataset_adapter.VTKArray(numpy.add.reduce(m, 1))
    if a1.DataSet == a2.DataSet : va.DataSet = a1.DataSet
    return va

def eigenvalue (narray) :
    "Returns the eigenvalue of an array of 2D square matrices."
    return _matrix_math_filter(narray, "Eigenvalue")

def eigenvector (narray) :
    "Returns the eigenvector of an array of 2D square matrices."
    return _matrix_math_filter(narray, "Eigenvector")

def global_max(narray):
    "Returns the maximum value of an array of scalars/vectors/tensors among all process."
    if len(narray) > 0:
        M = max(narray).astype(numpy.float64)
    else:
        M = numpy.finfo(numpy.float64).min

    if vtkProcessModule.GetProcessModule().GetNumberOfLocalPartitions() > 1 :
       from mpi4py import MPI
       M_recv = numpy.array(M)
       MPI.COMM_WORLD.Allreduce([M, MPI.DOUBLE], [M_recv, MPI.DOUBLE], MPI.MAX)
       M = M_recv
    return M

def global_mean (narray) :
    "Returns the mean value of an array of scalars/vectors/tensors among all process."
    # For some reason, Allreduce on MPI.INT does not return the correct value.
    # I have to convert it into a double array to get the correct results.
    dim = narray.shape[1]
    if len(narray.shape) == 3 : dim = narray.shape[1] * narray.shape[2]

    N = numpy.empty(dim).astype(numpy.float64)
    N.fill(narray.shape[0])

    S = narray.sum(0).astype(numpy.float64)
    if len(S.shape) == 2 and S.shape[0] == 3 and S.shape[1] == 3:
       S = numpy.reshape(S, 9)
    elif len(S.shape) == 3 and S.shape[0] == 1 and S.shape[1] == 3 and S.shape[2] == 3:
       S = numpy.reshape(S, 9)

    if vtkProcessModule.GetProcessModule().GetNumberOfLocalPartitions() > 1 :
       from mpi4py import MPI
       comm = MPI.COMM_WORLD
       S_recv = numpy.array(S)
       N_recv = numpy.array(N)
       comm.Allreduce([S, MPI.DOUBLE], [S_recv, MPI.DOUBLE],MPI.SUM)
       comm.Allreduce([N, MPI.DOUBLE], [N_recv, MPI.DOUBLE],MPI.SUM)
       S = S_recv
       N = N_recv
    return S / N

def global_min(narray):
    "Returns the minimum value of an array of scalars/vectors/tensors among all process."
    if len(narray) > 0:
        m = min(narray).astype(numpy.float64)
    else:
        m = numpy.finfo(numpy.float64).max

    if vtkProcessModule.GetProcessModule().GetNumberOfLocalPartitions() > 1 :
       from mpi4py import MPI
       m_recv = numpy.array(m)
       MPI.COMM_WORLD.Allreduce([m, MPI.DOUBLE], [m_recv, MPI.DOUBLE], MPI.MIN)
       m = m_recv
    return m

def gradient(narray, dataset=None):
    "Returns the gradient of an array of scalars/vectors."
    if not dataset: dataset = narray.DataSet()
    if not dataset: raise RuntimeError, 'Need a dataset to compute gradient'

    ncomp = narray.shape[1]
    if ncomp != 1 and ncomp != 3:
       raise RuntimeError, 'Gradient only works with scalars (1 component) and vectors (3 component)'\
                           'Input shape ' + narray.shape

    cd = vtkCellDerivatives()
    if ncomp == 1 : attribute_type = 'scalars'
    else : attribute_type = 'vectors'

    dsa = _cell_derivatives(narray, dataset, attribute_type, cd)

    if ncomp == 1 : retVal = dsa.GetVectors()
    else : retVal = dsa.GetTensors()

    try:
        if narray.GetName() : retVal.SetName("gradient of " + narray.GetName())
        else : retVal.SetName("gradient")
    except AttributeError : retVal.SetName("gradient")

    ans = dataset_adapter.vtkDataArrayToVTKArray(retVal, dataset)

    # The association information has been lost over the vtk filter
    # we must reconstruct it otherwise lower pipeline will be broken.
    ans.Association = narray.Association

    return ans

def inv (narray) :
    "Returns the inverse an array of 2D square matrices."
    return _matrix_math_filter(narray, "Inverse")

def inverse (narray) :
    "Returns the inverse of an array of 2D square matrices."
    return inv(narray)

def jacobian (dataset) :
    "Returns the jacobian of an array of 2D square matrices."
    return _cell_quality(dataset, "jacobian")

def laplacian (narray, dataset=None) :
    "Returns the jacobian of an array of scalars."
    if not dataset : dataset = narray.DataSet()
    if not dataset : raise RuntimeError, 'Need a dataset to compute laplacian'
    ans = gradient(narray, dataset)
    return divergence(ans)

def ln (narray) :
    "Returns the natural logarithm of an array of scalars/vectors/tensors."
    return numpy.log(narray)

def log (narray) :
    "Returns the natural logarithm of an array of scalars/vectors/tensors."
    return ln(narray)

def log10 (narray) :
    "Returns the base 10 logarithm of an array of scalars/vectors/tensors."
    return numpy.log10(narray)

# Functions marked with this decorator will check all leaves of a composite
# dataset.
def composite_function(func):
    def new_function(array):
        if hasattr(array, 'composite_iterator'):
            return func(array.composite_iterator)
        else:
            return func(array)
    new_function.__name__ = func.__name__
    return new_function

# This decorator will strip single-entry sequences from returned values.
def strip_sequence_from_result(func):
    def new_function(array):
        res = func(array)
        try:
            while len(res) == 1:
                # matrices have weird slice behaviors, slice them as a ndarray.
                if isinstance(res, numpy.matrix):
                    res = res.view(numpy.ndarray)
                res = res[0]
        except TypeError:
            pass
        return res
    new_function.__name__ = func.__name__
    return new_function

# This decorator fallsback to the provided builtin function when the input is
# not an instance of numpy ndarray.
def use_builtin_function_if_not_numpy_array(builtin_func):
    def inner_decorator(func):
        def new_function(array):
            if isinstance(array, numpy.ndarray):
                return func(array)
            return builtin_func(array)
        new_function.__name__ = func.__name__
        return new_function
    return inner_decorator

@strip_sequence_from_result
@composite_function
@use_builtin_function_if_not_numpy_array(max)
def max (narray):
    "Returns the maximum value of an array of scalars/vectors/tensors."
    ans = numpy.max(numpy.array(narray), axis=0)
    if len(ans.shape) == 2 and ans.shape[0] == 3 and ans.shape[1] == 3: ans.reshape(9)
    return ans

def max_angle (dataset) :
    "Returns the maximum angle of each cell in a dataset."
    return _cell_quality(dataset, "max_angle")

def mag (a) :
    "Returns the magnigude of an array of scalars/vectors."
    return numpy.sqrt(dot(a, a))

def mean(iter):
    """Fallback mean implementation, since there is no builtin."""
    s = sum(iter)
    l = len(iter)
    return s / l

@strip_sequence_from_result
@composite_function
@use_builtin_function_if_not_numpy_array(mean)
def mean (narray) :
    "Returns the mean value of an array of scalars/vectors/tensors."
    ans = numpy.mean(numpy.array(narray), axis=0)
    if len(ans.shape) == 2 and ans.shape[0] == 3 and ans.shape[1] == 3: ans.reshape(9)
    return ans

@strip_sequence_from_result
@composite_function
@use_builtin_function_if_not_numpy_array(min)
def min (narray):
    "Returns the min value of an array of scalars/vectors/tensors."
    ans = numpy.min(numpy.array(narray), axis=0)
    if len(ans.shape) == 2 and ans.shape[0] == 3 and ans.shape[1] == 3: ans.reshape(9)
    return ans

def min_angle (dataset) :
    "Returns the minimum angle of each cell in a dataset."
    return _cell_quality(dataset, "min_angle")

def norm (a) :
    "Returns the normalized values of an array of scalars/vectors."
    return a/mag(a)

def shear (dataset) :
    "Returns the shear of each cell in a dataset."
    return _cell_quality(dataset, "shear")

def skew (dataset) :
    "Returns the skew of each cell in a dataset."
    return _cell_quality(dataset, "skew")

def strain (narray, dataset=None) :
    "Returns the strain of an array of 3D vectors."
    if not dataset : dataset = narray.DataSet()
    if not dataset : raise RuntimeError, 'Need a dataset to compute strain'

    if 2 != narray.ndim or 3 != narray.shape[1] :
       raise RuntimeError, 'strain only works with an array of 3D vectors'\
                           'Input shape ' + narray.shape

    cd = vtkCellDerivatives()
    cd.SetTensorModeToComputeStrain()

    dsa = _cell_derivatives(narray, dataset, 'vectors', cd)

    retVal = dsa.GetTensors()
    retVal.SetName("strain")

    ans = dataset_adapter.vtkDataArrayToVTKArray(retVal, dataset)

    # The association information has been lost over the vtk filter
    # we must reconstruct it otherwise lower pipeline will be broken.
    ans.Association = narray.Association

    return ans

def surface_normal (dataset) :
    "Returns the surface normal of each cell in a dataset."
    if not dataset : raise RuntimeError, 'Need a dataset to compute surface_normal'

    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.CopyStructure(dataset.VTKObject)

    filter = vtkPolyDataNormals()
    filter.SetInputData(ds)
    filter.ComputeCellNormalsOn()
    filter.ComputePointNormalsOff()

    filter.SetFeatureAngle(180)
    filter.SplittingOff()
    filter.ConsistencyOff()
    filter.AutoOrientNormalsOff()
    filter.FlipNormalsOff()
    filter.NonManifoldTraversalOff()
    filter.Update()

    varray = filter.GetOutput().GetCellData().GetNormals()
    ans = dataset_adapter.vtkDataArrayToVTKArray(varray, dataset)

    # The association information has been lost over the vtk filter
    # we must reconstruct it otherwise lower pipeline will be broken.
    ans.Association = dataset_adapter.ArrayAssociation.CELL

    return ans

def trace (narray) :
    "Returns the trace of an array of 2D square matrices."
    ax1 = 0
    ax2 = 1
    if narray.ndim > 2 :
       ax1 = 1
       ax2 = 2
    return numpy.trace(narray, axis1=ax1, axis2=ax2)

def volume (dataset) :
    "Returns the volume normal of each cell in a dataset."
    return _cell_quality(dataset, "volume")

def vorticity(narray, dataset=None):
    "Returns the vorticity/curl of an array of 3D vectors."
    return curl(narray, dataset)

def vertex_normal (dataset) :
    "Returns the vertex normal of each point in a dataset."
    if not dataset : raise RuntimeError, 'Need a dataset to compute vertex_normal'

    ds = dataset.NewInstance()
    ds.UnRegister(None)
    ds.CopyStructure(dataset.VTKObject)

    filter = vtkPolyDataNormals()
    filter.SetInputData(ds)
    filter.ComputeCellNormalsOff()
    filter.ComputePointNormalsOn()

    filter.SetFeatureAngle(180)
    filter.SplittingOff()
    filter.ConsistencyOff()
    filter.AutoOrientNormalsOff()
    filter.FlipNormalsOff()
    filter.NonManifoldTraversalOff()
    filter.Update()

    varray = filter.GetOutput().GetPointData().GetNormals()
    ans = dataset_adapter.vtkDataArrayToVTKArray(varray, dataset)

    # The association information has been lost over the vtk filter
    # we must reconstruct it otherwise lower pipeline will be broken.
    ans.Association = dataset_adapter.ArrayAssociation.POINT

    return ans

