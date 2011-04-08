# Test Undo/Redo.
# Tests registering/unregistering/property modification.

import SMPythonTesting

import os.path
import sys
import time
from paraview import servermanager
servermanager.Connect()


def RenderAndWait(ren):
  ren.StillRender()
  time.sleep(.1)


SMPythonTesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(SMPythonTesting.SMStatesDir, "CompoundProxyUndoRedo.pvsm")
print "State file: %s" % pvsm_file
SMPythonTesting.LoadServerManagerState(pvsm_file)

pxm = servermanager.ProxyManager()
renModule = pxm.GetProxy("rendermodules", "RenderModule0")
renModule.UpdateVTKObjects()

self_cid = servermanager.ActiveConnection.ID

undoStack = servermanager.vtkSMUndoStack()
undoStackBuilder = servermanager.vtkSMUndoStackBuilder();
undoStackBuilder.SetUndoStack(undoStack);
undoStackBuilder.SetConnectionID(self_cid);

# Create a compound proxy for the elevation filter.
shrink = pxm.GetProxy("filters", "Shrink0")
reflect = pxm.GetProxy("filters", "Reflect0")
connect = pxm.GetProxy("filters", "Connect0")

compound_proxy = servermanager.Proxy(proxy=servermanager.vtkSMCompoundProxy())
compound_proxy.AddProxy("first", shrink)
compound_proxy.AddProxy("second", reflect)
compound_proxy.AddProxy("third", connect)
compound_proxy.ExposeProperty("first", "Input", "Input")

cp_definition = compound_proxy.SMProxy.SaveDefinition(None)

pxm.RegisterCompoundProxyDefinition("MyMacro", cp_definition)

undoStackBuilder.Begin("CPRegister")
pxm.RegisterProxy("mygroup", "Groupping", compound_proxy)
undoStackBuilder.EndAndPushToStack()

del compound_proxy

compound_proxy = servermanager.Proxy(proxy=pxm.NewCompoundProxy("MyMacro"))
compound_proxy.SetConnectionID(self_cid)

undoStackBuilder.Begin("CPRegister2")
pxm.RegisterProxy("mygroup", "Instantiation", compound_proxy)
undoStackBuilder.EndAndPushToStack()
del compound_proxy

RenderAndWait(renModule)

while undoStack.GetNumberOfUndoSets() > 0:
  print "**** Undo: %s" % undoStack.GetUndoSetLabel(0)
  undoStack.Undo()
  pxm.UpdateRegisteredProxies(0)
  RenderAndWait(renModule)

while undoStack.GetNumberOfRedoSets() > 0:
  print "**** Redo: %s" % undoStack.GetRedoSetLabel(0)
  undoStack.Redo()
  pxm.UpdateRegisteredProxies(0)
  RenderAndWait(renModule)

proxy2 = servermanager.sources.CubeSource()

shrink.Input = proxy2
shrink.UpdateVTKObjects()
RenderAndWait(renModule)

# This change should affect Groupping compound proxy.
# Verify that.
compound_proxy = pxm.GetProxy("mygroup", "Groupping")
if proxy2 != compound_proxy.Input:
  print "ERROR: Groupping Compund proxy has not groupped proxies correctly."
  sys.exit(1);

compound_proxy2 = pxm.GetProxy("mygroup", "Instantiation")
if compound_proxy2.GetProperty("Input").GetNumberOfProxies() != 0:
  print "ERROR: Instantiated proxy should really have any inputs set as yet."
  sys.exit(1)

if not SMPythonTesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)
  pass


