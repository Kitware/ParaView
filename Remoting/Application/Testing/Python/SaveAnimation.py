from __future__ import print_function
from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

tempdir = smtesting.GetUniqueTempDirectory("SaveAnimation-")
print("Generating output files in `%s`" % tempdir)

def RegressionTest(imageName, baselineName):
    from paraview.vtk.vtkTestingRendering import vtkTesting
    testing = vtkTesting()
    testing.AddArgument("-T")
    testing.AddArgument(tempdir)
    testing.AddArgument("-V")
    testing.AddArgument(smtesting.DataDir + "/Remoting/Application/Testing/Data/Baseline/" + baselineName)
    return testing.RegressionTest(tempdir + "/" + imageName, 10) == vtkTesting.PASSED


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

# Save animation images
SaveAnimation(tempdir + "/SaveAnimation.png", ImageResolution=[600, 600], ImageQuality=40)

# Lets save stereo animation images (two eyes at the same time)
SaveAnimation(tempdir + "/SaveAnimationStereo.png",
        ImageResolution=[600, 600], ImageQuality=40,
        StereoMode="Both Eyes")

# Lets save stere video
SaveAnimation(tempdir + "/SaveAnimationStereo.ogv",
        ImageResolution=[600, 600], ImageQuality=40,
        StereoMode="Both Eyes")

pm = servermanager.vtkProcessModule.GetProcessModule()
if pm.GetPartitionId() == 0:
    if not RegressionTest("SaveAnimation.0002.png", "SaveAnimation.png"):
        raise RuntimeError("Test failed (non-stereo)")
    if not RegressionTest("SaveAnimationStereo.0002_left.png", "SaveAnimation.png"):
        raise RuntimeError("Test failed (stereo: left-eye)")
    if not RegressionTest("SaveAnimationStereo.0002_right.png", "SaveAnimation_right.png"):
        raise RuntimeError("Test failed (stereo: right-eye)")

    import os.path
    if not os.path.exists(os.path.join(tempdir, "SaveAnimationStereo_right.ogv")):
        raise RuntimeError("Missing video file (stereo: right-eye)")
    if not os.path.exists(os.path.join(tempdir, "SaveAnimationStereo_left.ogv")):
        raise RuntimeError("Missing video file (stereo: left-eye)")
