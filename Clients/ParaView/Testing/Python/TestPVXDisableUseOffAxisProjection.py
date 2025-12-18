from paraview import servermanager
from paraview import simple
from paraview import smtesting
import os

# Make sure the test driver know that process has properly started
print ("Process started")


def getHost(url):
   return url.split(':')[1][2:]


def getPort(url):
   return int(url.split(':')[2])


def runTest():
    options = servermanager.vtkRemotingCoreConfiguration.GetInstance()
    url = options.GetServerURL()

    simple.Connect(getHost(url), getPort(url))

    simple.Sphere()
    simple.Show()
    renderView = simple.Render()

    renderView.CameraPosition = [-10.0, 0.0, 10.0]
    renderView.CameraFocalPoint = [0.10, 0.0, 2.0]

    renderView.Update()

    smtesting.ProcessCommandLineArguments()

    if not smtesting.DoRegressionTesting(renderView.SMProxy):
      raise smtesting.TestError('Test failed')

    assert renderView.GetIsInCAVE()

    # Ensure we correctly get the values from "../XML/DisableUseOffAxisProjection.pvx"

    assert(renderView.GetUseOffAxisProjection() is False)

    simple.Disconnect()


if __name__ == "__main__":
    runTest()
