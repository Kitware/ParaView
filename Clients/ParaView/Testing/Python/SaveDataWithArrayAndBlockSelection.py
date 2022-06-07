from paraview.simple import *
from paraview.vtk.util.misc import vtkGetTempDir
from os.path import join
import shutil

# Create MultiBlock Dataset

s1 = Sphere()
UpdatePipeline()

e1 = Elevation()
UpdatePipeline()

s2 = Sphere()
UpdatePipeline()

e2 = Elevation()
UpdatePipeline()

g = GroupDatasets(e1, e2)
UpdatePipeline()

# Setup output destination

dirname = join(vtkGetTempDir(), "savedatawitharrayselection")
shutil.rmtree(dirname, ignore_errors=True)

filename = join(dirname, "data.vtm")

# Test MultiBlock Dataset writer

SaveData(filename, ChooseArraysToWrite=1, PointDataArrays=["Normals"], ChooseBlocksToWrite=1, Selectors=['/Root/Elevation1'])

r = OpenDataFile(filename)
assert r.PointData.GetNumberOfArrays() == 1
assert r.PointData.GetArray(0).GetName() == "Normals"

dii = r.GetDataInformation().DataInformation
assert dii.GetNumberOfDataSets() == 1
assert dii.GetBlockName(1) == "Elevation1"

Delete(r)
shutil.rmtree(dirname, ignore_errors=True)

SetActiveSource(g)

SaveData(filename, ChooseArraysToWrite=1, PointDataArrays=["Normals"], ChooseBlocksToWrite=0, Selectors=['/Root/Elevation1'])

r = OpenDataFile(filename)
assert r.PointData.GetNumberOfArrays() == 1
assert r.PointData.GetArray(0).GetName() == "Normals"

dii = r.GetDataInformation().DataInformation
assert dii.GetNumberOfDataSets() == 2
assert dii.GetBlockName(1) == "Elevation1"
assert dii.GetBlockName(2) == "Elevation2"

Delete(r)
shutil.rmtree(dirname, ignore_errors=True)

SetActiveSource(g)

SaveData(filename, ChooseArraysToWrite=0, PointDataArrays=["Normals"], ChooseBlocksToWrite=1, Selectors=['/Root/Elevation1'])

r = OpenDataFile(filename)
assert r.PointData.GetNumberOfArrays() == 2
assert r.PointData.GetArray(0).GetName() == "Elevation"
assert r.PointData.GetArray(1).GetName() == "Normals"

dii = r.GetDataInformation().DataInformation
assert dii.GetNumberOfDataSets() == 1
assert dii.GetBlockName(1) == "Elevation1"

Delete(r)
shutil.rmtree(dirname, ignore_errors=True)

SetActiveSource(g)

SaveData(filename, ChooseArraysToWrite=0, PointDataArrays=["Normals"], ChooseBlocksToWrite=0, Selectors=['/Root/Elevation1'])

r = OpenDataFile(filename)
assert r.PointData.GetNumberOfArrays() == 2
assert r.PointData.GetArray(0).GetName() == "Elevation"
assert r.PointData.GetArray(1).GetName() == "Normals"

dii = r.GetDataInformation().DataInformation
assert dii.GetNumberOfDataSets() == 2
assert dii.GetBlockName(1) == "Elevation1"
assert dii.GetBlockName(2) == "Elevation2"

Delete(r)
shutil.rmtree(dirname, ignore_errors=True)
