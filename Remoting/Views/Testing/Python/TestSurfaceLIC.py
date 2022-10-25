from paraview.simple import *
import os.path
from vtk.util.misc import vtkGetDataRoot

datafile = os.path.join(vtkGetDataRoot(), "Testing/Data/office.binary.vtk")
OpenDataFile(datafile)
r = Show(Representation = "Surface LIC")

# check for existence of certain representation properties from LIC
print ("LICIntensity", r.LICIntensity)
print ("SelectInputVectors", r.SelectInputVectors)
Render()
