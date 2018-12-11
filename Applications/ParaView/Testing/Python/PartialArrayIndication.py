# Test to check that partial array indicator is correct under various
# conditions.

from paraview.simple import *
from paraview.vtk import *
from paraview.vtk.vtkFiltersSources import vtkSphereSource
from paraview.vtk.vtkCommonDataModel import vtkPolyData
from paraview.vtk.vtkCommonCore import vtkIntArray
from paraview.modules.vtkPVClientServerCoreCore import vtkPVDataInformation

source = PVTrivialProducer()
algo = source.GetClientSideObject()

mb = vtkMultiBlockDataSet()
algo.SetOutput(mb)
source.MarkModified(None)
source.UpdatePipeline()

assert source.GetDataInformation().GetNumberOfPoints() == 0

sphere = vtkSphereSource()
sphere.Update()
mb.SetBlock(1, sphere.GetOutputDataObject(0))
source.MarkModified(None)
source.UpdatePipeline()

assert source.GetDataInformation().GetPointDataInformation().GetArrayInformation("Normals").GetIsPartial() == 0

pd2 = vtkPolyData()
mb.SetBlock(2, pd2)
source.MarkModified(None)
source.UpdatePipeline()

assert source.GetDataInformation().GetPointDataInformation().GetArrayInformation("Normals").GetIsPartial() == 0

pd3 = vtkPolyData()
pd3.CopyStructure(sphere.GetOutputDataObject(0))
mb.SetBlock(3, pd3)
source.MarkModified(None)
source.UpdatePipeline()

assert source.GetDataInformation().GetPointDataInformation().GetArrayInformation("Normals").GetIsPartial() == 1

#------------------------------------------------------------
# Test for BUG #17813
nonemptyData = sphere.GetOutputDataObject(0)
emptyData = vtkPolyData()

# add field arrays to both.
arr = vtkIntArray()
arr.SetName("field0");
arr.SetNumberOfTuples(1)
arr.SetValue(0, 100)

nonemptyData.GetFieldData().AddArray(arr)
emptyData.GetFieldData().AddArray(arr)

infoNE = vtkPVDataInformation()
infoNE.CopyFromObject(nonemptyData)
assert infoNE.GetNumberOfPoints() > 0
assert infoNE.GetFieldDataInformation().GetArrayInformation("field0") != None

infoE = vtkPVDataInformation()
infoE.CopyFromObject(emptyData)
assert infoE.GetNumberOfPoints() == 0
assert infoE.GetFieldDataInformation().GetArrayInformation("field0") != None

combined = vtkPVDataInformation()
combined.AddInformation(infoE)
combined.AddInformation(infoNE)
assert combined.GetFieldDataInformation().GetArrayInformation("field0").GetIsPartial() == 0
