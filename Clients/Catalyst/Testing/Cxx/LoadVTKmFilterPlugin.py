from paraview.simple import *
LoadDistributedPlugin("VTKmFilters", ns=globals())
assert VTKmContour

def DoCoProcessing(datadescription):
    print("in DoCoProcessing")
    print("VTKmFilters plugin has been loaded successfully!")

def RequestDataDescription(datadescription):
    datadescription.GetInputDescriptionByName('input').GenerateMeshOn()
