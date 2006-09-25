# This is a module for Server Manager testing using Python. 
# This provides several utility functions useful for testing
import os
import re
import sys
import paraview


__ProcessedCommandLineArguments__ = False
DataDir = ""
TempDir = ""
BaselineImage = ""
Threshold = 10.0
SMStatesDir = ""
StateXMLFileName = ""
UseSavedStateForRegressionTests = False

def Error(message):
  print "ERROR: %s" % message
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
  parser = paraview.vtkPVXMLParser()

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
  loader = paraview.vtkSMStateLoader()
  loader.SetConnectionID(paraview.ActiveConnection.ID)
  root = parser.GetRootElement()
  if loader.LoadState(root,0):
    pxm = paraview.vtkSMObject.GetProxyManager()
    pxm.UpdateRegisteredProxies("sources", 0)
    pxm.UpdateRegisteredProxies("filters", 0)
    pxm.UpdateRegisteredProxies(0)
    return True
  return Error("Failed to load state file %s" % filename)

def DoRegressionTesting():
  """Perform regression testing."""
  global TempDir
  global BaselineImage
  global Threshold
  ProcessCommandLineArguments()
  
  testing = paraview.vtkSMTesting()
  testing.AddArgument("-T")
  testing.AddArgument(TempDir)
  testing.AddArgument("-V")
  testing.AddArgument(BaselineImage)

  pxm = paraview.vtkSMObject.GetProxyManager()
  rmProxy = pxm.GetProxy("rendermodules","RenderModule0")
  #pyProxy(rmProxy).SetRenderWindowSize(300, 300);
  #rmProxy.GetProperty("RenderWindowSize").SetElement(0, 300)
  #rmProxy.GetProperty("RenderWindowSize").SetElement(1, 300)
  #rmProxy.UpdateVTKObjects()
  rmProxy.StillRender()
  testing.SetRenderModuleProxy(rmProxy)
  if testing.RegressionTest(Threshold) == 1:
    return True
  return Error("Regression Test Failed!")
  
  
if __name__ == "__main__":
  # This script loads the state, saves out a temp state and loads the saved state.
  # This saved state is used for testing -- this will ensure load/save SM state
  # is working fine.
  paraview.ActiveConnection = paraview.Connect()
  ProcessCommandLineArguments()
  ret = 1
  if StateXMLFileName:
    if LoadServerManagerState(StateXMLFileName):
      pxm = paraview.vtkSMObject.GetProxyManager()
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

