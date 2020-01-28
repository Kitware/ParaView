# This is a module for Server Manager testing using Python.
# This provides several utility functions useful for testing
import os
import re
import sys
from paraview.modules.vtkRemotingServerManager import *
from paraview.modules.vtkRemotingApplication import *
from paraview.modules.vtkRemotingMisc import *

# we get different behavior based on how we import servermanager
# so we want to import servermanager the same way in this module
# as we do in any module that is importing this
SMModuleName = 'paraview.servermanager'
if 'paraview.simple' in sys.modules:
  SMModuleName = 'paraview.simple'

sm = __import__(SMModuleName)
servermanager = sm.servermanager

class TestError(Exception):
  pass

__ProcessedCommandLineArguments__ = False
DataDir = ""
TempDir = ""
BaselineImage = ""
Threshold = 10.0
SMStatesDir = ""
StateXMLFileName = ""
UseSavedStateForRegressionTests = False

def Error(message):
  print ("ERROR: %s" % message)
  return False

def ProcessCommandLineArguments():
  """Processes the command line areguments."""
  global DataDir
  global TempDir
  global BaselineImage
  global Threshold
  global StateXMLFileName
  global UseSavedStateForRegressionTests
  global SMStatesDir
  global __ProcessedCommandLineArguments__
  if __ProcessedCommandLineArguments__:
    return
  __ProcessedCommandLineArguments__ = True
  length = len(sys.argv)
  index = 1
  while index < length:
    key = sys.argv[index-1]
    value = sys.argv[index]
    index += 2
    if key == "-D":
      DataDir = value
    elif key == "-V":
      BaselineImage = value
    elif key == "-T":
      TempDir = value
    elif key == "-S":
      SMStatesDir = value
    elif key == "--threshold":
      Threshold = float(value)
    elif key == "--state":
      StateXMLFileName = value
    elif key == "--use_saved_state":
      UseSavedStateForRegressionTests = True
      index -= 1
    else:
      index -=1
  return

def LoadServerManagerState(filename):
  """This function loads the servermanager state xml/pvsm.
  Returns the status of the load."""
  global DataDir
  ProcessCommandLineArguments()
  parser = servermanager.vtkPVXMLParser()

  try:
    fp = open(filename, "r")
    data = fp.read()
    fp.close()
  except:
    return Error("Failed to open state file %s" % filename)

  regExp = re.compile("\${DataDir}")
  data = regExp.sub(DataDir, data)
  if not parser.Parse(data):
    return Error("Failed to parse")
  loader = servermanager.vtkSMStateLoader()
  loader.SetSession(servermanager.ActiveConnection.Session)
  root = parser.GetRootElement()
  if loader.LoadState(root):
    pxm = servermanager.vtkSMProxyManager.GetProxyManager().GetActiveSessionProxyManager()
    pxm.UpdateRegisteredProxiesInOrder(0);
    pxm.UpdateRegisteredProxies(0)
    return True
  return Error("Failed to load state file %s" % filename)

def DoRegressionTesting(rmProxy=None):
  """Perform regression testing."""
  global TempDir
  global BaselineImage
  global Threshold
  ProcessCommandLineArguments()

  testing = vtkSMTesting()
  testing.AddArgument("-T")
  testing.AddArgument(TempDir)
  testing.AddArgument("-V")
  testing.AddArgument(BaselineImage)

  if not rmProxy:
    rmProxy = servermanager.GetRenderView()
    if rmProxy:
      rmProxy = rmProxy.SMProxy
  if not rmProxy:
    raise "Failed to locate view to perform regression testing."
  #pyProxy(rmProxy).SetRenderWindowSize(300, 300);
  #rmProxy.GetProperty("RenderWindowSize").SetElement(0, 300)
  #rmProxy.GetProperty("RenderWindowSize").SetElement(1, 300)
  #rmProxy.UpdateVTKObjects()
  rmProxy.StillRender()
  testing.SetRenderViewProxy(rmProxy)
  if testing.RegressionTest(Threshold) == 1:
    return True
  return Error("Regression Test Failed!")


def GetUniqueTempDirectory(prefix):
    """A convenience method to generate a temporary directory unique to the
    current ParaView process (or MPI group of processes)"""
    global TempDir
    pm = servermanager.vtkProcessModule.GetProcessModule()
    if pm.GetNumberOfLocalPartitions() > 1 and pm.GetSymmetricMPIMode():
        if pm.GetPartitionId() == 0:
            import os
            pid = os.getpid()
        else:
            pid = None
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        pid = comm.bcast(pid, root=0)
    else:
        import os
        pid = os.getpid()

    import os.path
    tempdir = os.path.join(TempDir, "%s%d" % (prefix,pid))
    if pm.GetPartitionId() == 0:
        try:
            os.makedirs(tempdir)
        except OSError:
            pass
    if pm.GetNumberOfLocalPartitions() > 1 and pm.GetSymmetricMPIMode():
        from mpi4py import MPI
        comm = MPI.COMM_WORLD
        comm.Barrier()
    return tempdir

if __name__ == "__main__":
  # This script loads the state, saves out a temp state and loads the saved state.
  # This saved state is used for testing -- this will ensure load/save SM state
  # is working fine.
  servermanager.Connect()
  ProcessCommandLineArguments()
  ret = 1
  if StateXMLFileName:
    if LoadServerManagerState(StateXMLFileName):
      pxm = servermanager.vtkSMProxyManager.GetProxyManager().GetActiveSessionProxyManager()
      if UseSavedStateForRegressionTests:
        saved_state = os.path.join(TempDir, "temp.pvsm")
        pxm.SaveState(saved_state)
        pxm.UnRegisterProxies();
        LoadServerManagerState(saved_state)
        try:
          os.remove(saved_state)
        except:
          pass
      if DoRegressionTesting():
        ret = 0
  else:
    Error("No ServerManager state file specified")
  if ret:
    # This leads to vtkDebugLeaks reporting leaks, hence we do this
    # only when the tests failed.
    sys.exit(ret)
