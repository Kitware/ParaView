from paraview import smtesting
from paraview.simple import *
import os.path

smtesting.ProcessCommandLineArguments()

datafile = os.path.join(smtesting.DataDir, "office.binary.vtk")
OpenDataFile(datafile)
r = Show()

# check for existence of certain representation properties from LIC
# First, before loading the plugin
try:
    print ("LICIntensity", r.LICIntensity)
except AttributeError:
    pass
else:
    raise RuntimeError("FAILED! LICIntensity should not have exisited!")

try:
    print ("SelectInputVectors", r.SelectInputVectors)
except AttributeError:
    pass
else:
    raise RuntimeError("FAILED! SelectInputVectors should not have exisited!")

# now load the plugin and try again
LoadDistributedPlugin("SurfaceLIC", remote=False, ns=globals())
datafile = os.path.join(smtesting.DataDir, "office.binary.vtk")
OpenDataFile(datafile)
r = Show(Representation = "Surface LIC")

# check for existence of certain representation properties from LIC
print ("LICIntensity", r.LICIntensity)
print ("SelectInputVectors", r.SelectInputVectors)
Render()
