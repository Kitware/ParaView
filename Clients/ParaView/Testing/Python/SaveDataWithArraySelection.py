from paraview.simple import *
from paraview.vtk.util.misc import vtkGetTempDir
from os.path import join
import shutil

Sphere()
UpdatePipeline()

e = Elevation()
UpdatePipeline()

dirname = join(vtkGetTempDir(), "savedatawitharrayselection")
shutil.rmtree(dirname, ignore_errors=True)

filename = join(dirname, "data.pvd")

SaveData(filename, ChooseArraysToWrite=1, PointDataArrays=["Normals"])

r = OpenDataFile(filename)
assert r.PointArrays.GetAvailable() == ["Normals"]
Delete(r)

shutil.rmtree(dirname, ignore_errors=True)

SetActiveSource(e)
SaveData(filename, ChooseArraysToWrite=0, PointDataArrays=["Normals"])

r = OpenDataFile(filename)
assert r.PointArrays.GetAvailable() == ["Normals", "Elevation"]

shutil.rmtree(dirname, ignore_errors=True)
