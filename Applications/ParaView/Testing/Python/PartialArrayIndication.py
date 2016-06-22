# Test to check that partial array indicator is correct under various
# conditions.

from paraview.simple import *
from paraview.vtk import *
from paraview.vtk.vtkFiltersSources import vtkSphereSource

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
