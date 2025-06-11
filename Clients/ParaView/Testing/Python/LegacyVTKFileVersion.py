from paraview.simple import *
from paraview.vtk.util.misc import vtkGetTempDir
from os.path import join
import shutil
from os import makedirs

s = Sphere()
SetActiveSource(s)
UpdatePipeline()

dirname = join(vtkGetTempDir(), "legacyvtkfileversion")
shutil.rmtree(dirname, ignore_errors=True)
makedirs(dirname)

filename = join(dirname, "data.vtk")

SaveData(filename, FormatVersion=42)

r = OpenDataFile(filename)
SetActiveSource(r)
UpdatePipeline()
assert r.GetDataInformation().DataInformation.GetNumberOfCells() == 96
Delete(r)

shutil.rmtree(dirname, ignore_errors=True)
