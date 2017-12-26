#### import the simple module from the paraview
from paraview.simple import *
from paraview import smtesting
from paraview.vtk.util.misc import vtkGetTempDir
import os, os.path

# Create a sphere and save it as a Houdini file.
sphere = Sphere()
Show()

testDir = vtkGetTempDir()

geoFileName = os.path.join(testDir, "HoudiniWriterData.geo")
writer = CreateWriter(geoFileName, sphere)
writer.UpdatePipeline()

os.remove(geoFileName)
