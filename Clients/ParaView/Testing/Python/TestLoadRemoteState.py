from paraview import servermanager
import paraview.simple as smp
from paraview.vtk.util.misc import vtkGetDataRoot
from os.path import realpath, join, dirname
from paraview.modules.vtkRemotingCore import vtkPVSession

scriptdir = dirname(realpath(__file__))
statefile = join(scriptdir, "StateWithDatasets.pvsm")
data_dir = vtkGetDataRoot() + "/Testing/Data/"

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

    smp.LoadState(statefile, data_directory=data_dir, location=vtkPVSession.SERVERS)

    names = set([x[0] for x in smp.GetSources().keys()])
    assert names == set(["can.ex2", "timeseries", "dataset"])

    smp.Disconnect()


runTest()
