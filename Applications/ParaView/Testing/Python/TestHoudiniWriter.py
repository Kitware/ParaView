#### import the simple module from the paraview
from paraview.simple import *
from paraview import smtesting
from vtk import *
import os

# Create a sphere and save it as a Houdini file.
sphere = Sphere()
Show()

testDir = vtk.util.misc.vtkGetTempDir()

geoFileName = os.path.join(testDir, "HoudiniWriterData.geo")
writer = CreateWriter(geoFileName, sphere)
writer.UpdatePipeline()

os.remove(geoFileName)
