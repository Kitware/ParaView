from paraview import servermanager
from paraview import simple as smp
from paraview import smtesting

import os
import sys

smp.LoadPalette("BlueGrayBackground")

# Make sure the test driver know that process has properly started
print ("Process started")

def getHost(url):
   return url.split(':')[1][2:]
def getPort(url):
   return int(url.split(':')[2])


def runTest():
    options = servermanager.vtkRemotingCoreConfiguration.GetInstance()
    url = options.GetServerURL()
    smp.Connect(getHost(url), getPort(url))

    r = smp.CreateRenderView()
    r.RemoteRenderThreshold = 20
    s = smp.Sphere()
    s.PhiResolution = 80
    s.ThetaResolution = 80

    d = smp.Show()
    d.Representation = "Wireframe"
    smp.Render()
    r.RemoteRenderThreshold = 0
    smp.Render()
    s.PhiResolution = 8
    s.ThetaResolution = 8
    smp.Render()

    # macOS differs a little bit from other platforms.
    if sys.platform == 'darwin':
        os.environ["VTK_TESTING_IMAGE_COMPARE_METHOD"] = "LOOSE_VALID"
    smtesting.ProcessCommandLineArguments()
    if not smtesting.DoRegressionTesting(r.SMProxy):
        raise smtesting.TestError ("Test failed!!!")
    print ("Test Passed")
runTest()
