from __future__ import print_function
from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

def RegressionTest(imageName, baselineName):
    from paraview.vtk.vtkTestingRendering import vtkTesting
    testing = vtkTesting()
    testing.AddArgument("-T")
    testing.AddArgument(smtesting.TempDir)
    testing.AddArgument("-V")
    testing.AddArgument(smtesting.DataDir + "/ParaViewCore/ServerManager/Default/Testing/Data/Baseline/" + baselineName)
    return testing.RegressionTest(smtesting.TempDir + "/" + imageName, 10) == vtkTesting.PASSED


# Create a new 'Render View'
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [300, 300]
# ----------------------------------------------------------------
# setup the data processing pipelines
# ----------------------------------------------------------------

# create a new 'ExodusIIReader'
reader = OpenDataFile(smtesting.DataDir + '/Testing/Data/dualSphereAnimation4.pvd')

# get animation scene
animationScene1 = GetAnimationScene()

# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

# ----------------------------------------------------------------
# setup the visualization in view 'renderView1'
# ----------------------------------------------------------------

# show data from reader
canex2Display = Show(reader, renderView1)

SaveAnimation(smtesting.TempDir + "/SaveAnimation.png", ImageResolution=[600, 600], ImageQuality=40)

pm = servermanager.vtkProcessModule.GetProcessModule()
if pm.GetPartitionId() == 0:
    if not RegressionTest("SaveAnimation.0002.png", "SaveAnimation.png"):
        raise RuntimeError("Test failed")
