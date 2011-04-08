from paraview.simple import *
import sys
import SMPythonTesting

resolution = 15
cone = Cone(Resolution=resolution)
if cone.Resolution != resolution:
    raise SMPythonTesting.Error('Test failed: Resolution has not been set properly.')

resolution = 12
cone.Resolution = resolution
if cone.Resolution != resolution:
    raise SMPythonTesting.Error('Test failed: Problem changing resolution.')

cone.Center = [3.1, 4.2, 5.5]
if cone.Center[0] != 3.1 or cone.Center[1] != 4.2 or cone.Center[2] != 5.5:
    raise SMPythonTesting.Error('Test failed: Problem setting center of cone.')

shrinkFilter = Shrink(cone)
if shrinkFilter.Input != cone:
    raise SMPythonTesting.Error('Test failed: Pipeline not properly set.')

shrinkFilter.UpdatePipeline()
if shrinkFilter.GetDataInformation().GetNumberOfCells() != resolution+1 or shrinkFilter.GetDataInformation().GetNumberOfPoints() != resolution*4:
    raise SMPythonTesting.Error('Test failed: Pipeline not operating properly.')

resolution = 33
rp = cone.GetProperty("Resolution")
rp.SetElement(0, resolution)
cone.UpdateProperty("Resolution")
shrinkFilter.UpdatePipeline()
if shrinkFilter.GetDataInformation().GetNumberOfCells() != resolution+1 or shrinkFilter.GetDataInformation().GetNumberOfPoints() != resolution*4:
    raise SMPythonTesting.Error('Test failed: Problem setting property directly.')

Show(shrinkFilter)
ren = Render() 

if not SMPythonTesting.DoRegressionTesting(ren.SMProxy):
    raise SMPythonTesting.Error('Image comparison failed.')

