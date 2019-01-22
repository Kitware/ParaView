# Test to test #17793.
# Ensures that field data arrays are correctly marked as partial when dealing
# with composite datasets.

from paraview.modules.vtkPVClientServerCoreCore import vtkPVDataInformation
from paraview.vtk.vtkCommonDataModel import vtkMultiBlockDataSet, vtkPolyData
from paraview.vtk.vtkCommonCore import vtkFloatArray

mb = vtkMultiBlockDataSet()
mb.SetBlock(0, vtkPolyData())
mb.SetBlock(1, vtkPolyData())

arr = vtkFloatArray()
arr.SetNumberOfTuples(1)
arr.SetName("Field1");
mb.GetFieldData().AddArray(arr)

di = vtkPVDataInformation()
di.CopyFromObject(mb)
assert di.GetFieldDataInformation().GetArrayInformation("Field1").GetIsPartial() == 0

mb2 = vtkMultiBlockDataSet()
mb2.SetBlock(0, mb)
mb2.SetBlock(1, vtkPolyData())

arr = vtkFloatArray()
arr.SetNumberOfTuples(1)
arr.SetName("Field2");
mb2.GetFieldData().AddArray(arr)

di = vtkPVDataInformation()
di.CopyFromObject(mb2)
assert di.GetFieldDataInformation().GetArrayInformation("Field1").GetIsPartial() == 1 and\
    di.GetFieldDataInformation().GetArrayInformation("Field2").GetIsPartial() == 0
