try:
    import numpy
except ImportError:
    raise RuntimeError("This module depends on the numpy module. Please make\
sure that it is installed properly.")

from paraview import servermanager
from paraview import numpy_support

class VTKObjectWrapper(object):
    "Superclass for classes that wrap VTK objects with Python objects."
    def __init__(self, vtkobject):
        self.VTKObject = vtkobject

    def __getattr__(self, name):
        "Forwards unknown attribute requests to VTK object."
        if not self.VTKObject:
            raise AttributeError("class has no attribute %s" % name)
            return None
        return getattr(self.VTKObject, name)

def MakeObserver(numpy_array):
    "Internal function used to attach a numpy array to a vtk array"
    def Closure(caller, event):
        foo = numpy_array
    return Closure

def vtkDataArrayToVTKArray(array, dataset=None):
    "Given a vtkDataArray and a dataset owning it, returns a VTKArray."
    narray = numpy_support.vtk_to_numpy(array)
    # The numpy_support convention of returning an array of snape (n,)
    # causes problems. Change it to (n,1)
    if len(narray.shape) == 1:
        narray = narray.reshape(narray.shape[0], 1)
    return VTKArray(narray, array=array, dataset=dataset)
    
def numpyTovtkDataArray(array, name="numpy_array"):
    """Given a numpy array or a VTKArray and a name, returns a vtkDataArray.
    The resulting vtkDataArray will store a reference to the numpy array
    through a DeleteEvent observer: the numpy array is released only when
    the vtkDataArray is destroyed."""
    if not array.flags.contiguous:
        array = array.copy()
    vtkarray = numpy_support.numpy_to_vtk(array)
    vtkarray.SetName(name)
    # This makes the VTK array carry a reference to the numpy array.
    vtkarray.AddObserver('DeleteEvent', MakeObserver(array))
    return vtkarray
    
class VTKArray(numpy.ndarray):
    """This is a sub-class of numpy ndarray that stores a
    reference to a vtk array as well as the owning dataset.
    The numpy array and vtk array should point to the same
    memory location."""
    
    def __new__(cls, input_array, array=None, dataset=None):
        # Input array is an already formed ndarray instance
        # We first cast to be our class type
        obj = numpy.asarray(input_array).view(cls)
        # add the new attributes to the created instance
        obj.VTKObject = array
        obj.DataSet = dataset
        # Finally, we must return the newly created object:
        return obj

    def __array_finalize__(self,obj):
        # reset the attributes from passed original object
        self.VTKObject = getattr(obj, 'VTKObject', None)
        self.DataSet = getattr(obj, 'DataSet', None)
        # We do not need to return anything

    def __getattr__(self, name):
        "Forwards unknown attribute requests to VTK array."
        if not self.VTKObject:
            raise AttributeError("class has no attribute %s" % name)
            return None
        return getattr(self.VTKObject, name)


class DataSetAttributes(VTKObjectWrapper):
    """This is a python friendly wrapper of vtkDataSetAttributes. It
    returns VTKArrays. It also provides the dictionary interface."""
    
    def __init__(self, vtkobject, dataset):
        self.VTKObject = vtkobject
        self.DataSet = dataset

    def __getitem__(self, idx):
        """Implements the [] operator. Accepts an array name."""
        return self.GetArray(idx)

    def GetArray(self, idx):
        "Given an index or name, returns a VTKArray."
        vtkarray = self.VTKObject.GetArray(idx)
        if not vtkarray:
            return None
        return vtkDataArrayToVTKArray(vtkarray, self.DataSet)

    def keys(self):
        """Returns the names of the arrays as a list."""
        kys = []
        narrays = self.VTKObject.GetNumberOfArrays()
        for i in range(narrays):
            name = self.VTKObject.GetArray(i).GetName()
            if name:
                kys.append(name)
        return kys

    def values(self):
        """Returns the arrays as a list."""
        vals = []
        narrays = self.VTKObject.GetNumberOfArrays()
        for i in range(narrays):
            a = self.VTKObject.GetArray(i)
            if a.GetName():
                vals.append(a)
        return vals

    def append(self, narray, name):
        """Appends a new array to the dataset attributes."""
        if not narray.flags.contiguous:
            narray = narray.copy()
        arr = numpyTovtkDataArray(narray, name)
        self.VTKObject.AddArray(arr)

class CompositeDataIterator(object):
    """Wrapper for a vtkCompositeDataIterator class to satisfy
       the python iterator protocol.
       """
    
    def __init__(self, cds):
        self.Iterator = cds.NewIterator()
        if self.Iterator:
            self.Iterator.UnRegister(None)
            self.Iterator.GoToFirstItem()

    def __iter__(self):
        return self

    def next(self):
        if not self.Iterator:
            raise StopIteration

        if self.Iterator.IsDoneWithTraversal():
            raise StopIteration
        retVal = self.Iterator.GetCurrentDataObject()
        self.Iterator.GoToNextItem()
        return WrapDataObject(retVal)

    def __getattr__(self, name):
        """Returns attributes from the vtkCompositeDataIterator."""
        return getattr(self.Iterator, name)

class CompositeDataSet(VTKObjectWrapper):

    def __iter__(self):
        "Creates an iterator for the contained datasets."
        return CompositeDataIterator(self)
    
class DataSet(VTKObjectWrapper):
    """This is a python friendly wrapper of a vtkDataSet that defines
    a few useful properties."""
    
    def GetPointData(self):
        "Returns the point data as a DataSetAttributes instance."
        return DataSetAttributes(self.VTKObject.GetPointData(), self)

    def GetCellData(self):
        "Returns the cell data as a DataSetAttributes instance."
        return DataSetAttributes(self.VTKObject.GetCellData(), self)

    def GetFieldData(self):
        "Returns the field data as a DataSetAttributes instance."
        return DataSetAttributes(self.VTKObject.GetFieldData(), self)
        
    def GetPoints(self):
        """Returns the points as a VTKArray instance. Returns None if the
        dataset has implicit points."""
        if not self.VTKObject.GetPoints():
            return None
        return vtkDataArrayToVTKArray(
            self.VTKObject.GetPoints().GetData(), self)

    PointData = property(GetPointData, None, None, "This property returns \
        the point data of the dataset.")
    CellData = property(GetCellData, None, None, "This property returns \
        the cell data of a dataset.")
    FieldData = property(GetFieldData, None, None, "This property returns \
        the field data of a dataset.")
    Points = property(GetPoints, None, None, "This property returns the \
        point coordinates of dataset. It returns None if the points are \
        implicit (i.e. image data and rectiliear grid).")

def WrapDataObject(ds):
    if ds.IsA("vtkDataSet"):
        return DataSet(ds)
    elif ds.IsA("vtkCompositeDataSet"):
        return CompositeDataSet(ds)
        
