from paraview import servermanager
from paraview import simple as smp
from paraview import smtesting

# Make sure the test driver know that process has properly started
print ("Process started")

def getHost(url):
   return url.split(':')[1][2:]
def getPort(url):
   return int(url.split(':')[2])


def runTest():
    options = servermanager.vtkProcessModule.GetProcessModule().GetOptions()
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

    smtesting.ProcessCommandLineArguments()
    if not smtesting.DoRegressionTesting(r.SMProxy):
        raise smtesting.TestError ("Test failed!!!")
    print ("Test Passed")
runTest()
