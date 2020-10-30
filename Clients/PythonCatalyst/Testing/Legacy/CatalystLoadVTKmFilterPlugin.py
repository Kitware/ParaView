from paraview.simple import *

# Load the distributed plugin.
LoadDistributedPlugin("VTKmFilters" , remote=False, ns=globals())

assert VTKmContour

def DoCoProcessing(datadescription):
    print("in DoCoProcessing")

def RequestDataDescription(datadescription):
    datadescription.GetInputDescriptionByName('input').GenerateMeshOn()
