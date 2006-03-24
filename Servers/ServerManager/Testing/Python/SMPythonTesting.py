# This is a module for Server Manager testing using Python. 
# This provides several utility functions useful for testing
from libvtkPVServerCommonPython import *
from libvtkPVServerManagerPython import *

import sys
import re

__ProcessedCommandLineArguments__ = False
DataDir = ""
TempDir = ""
BaselineImage = ""
Threshold = 10.0
StateXMLFileName = ""

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
    elif key == "--threshold":
      Threshold = float(value)
    elif key == "--state":
      StateXMLFileName = value
    else:
      index -=1
  return

def LoadServerManagerState(filename):
  """This function loads the servermanager state xml/pvsm.
  Returns the status of the load."""
  global DataDir
  ProcessCommandLineArguments()
  parser = vtkPVXMLParser()

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
  loader = vtkSMStateLoader()
  root = parser.GetRootElement()
  if loader.LoadState(root,0):
    return True
  return Error("Failed to load state file %s" % filename)

def DoRegressionTesting():
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

  pxm = vtkSMObject.GetProxyManager()
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
  ProcessCommandLineArguments()
  ret = 1
  if StateXMLFileName:
    if LoadServerManagerState(StateXMLFileName):
      pxm = vtkSMObject.GetProxyManager()
      pxm.UpdateRegisteredProxies(0)
      if DoRegressionTesting():
        ret = 0
  else:
    Error("No ServerManager state file specified")
  sys.exit(ret)

