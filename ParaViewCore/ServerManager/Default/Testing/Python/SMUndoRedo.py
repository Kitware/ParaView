# Test Undo/Redo.
# Tests registering/unregistering/property modification.

from paraview import smtesting

import os.path
import sys
import time

from paraview import servermanager
servermanager.Connect()


def RenderAndWait(ren):
  ren.StillRender()
  #time.sleep(.5)


smtesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(smtesting.SMStatesDir, "UndoRedo.pvsm")
print "State file: %s" % pvsm_file
smtesting.LoadServerManagerState(pvsm_file)

pxm = servermanager.ProxyManager()
renModule = pxm.GetProxy("rendermodules", "RenderModule0")
renModule.UpdateVTKObjects()

self_cid = servermanager.ActiveConnection.ID 

undoStack = servermanager.vtkSMUndoStack()
undoStackBuilder = servermanager.vtkSMUndoStackBuilder();
undoStackBuilder.SetUndoStack(undoStack);
undoStackBuilder.SetConnectionID(self_cid);

  
proxy = pxm.NewProxy("sources","SphereSource")
proxy2 = pxm.NewProxy("sources","CubeSource")

filter = pxm.NewProxy("filters", "ElevationFilter")
display = renModule.CreateDisplayProxy()
# CreateDisplayProxy() returns a proxy with an extra reference
# hence we need to unregister it.
display.UnRegister(None)
 
undoStackBuilder.Begin("CreateFilter")
pxm.RegisterProxy("mygroup", "sphere", proxy)
pxm.RegisterProxy("mygroup", "cube", proxy2)
pxm.RegisterProxy("filters", "elevationFilter", filter)
undoStackBuilder.End()
undoStackBuilder.PushToStack();

undoStackBuilder.Begin("FilterInput")
filter.SetInput(proxy)
filter.UpdateVTKObjects()
undoStackBuilder.End()
undoStackBuilder.PushToStack();
  
undoStackBuilder.Begin("CreateDisplay")
pxm.RegisterProxy("displays", "sphereDisplay", display)
undoStackBuilder.End()
undoStackBuilder.PushToStack();

undoStackBuilder.Begin("SetupDisplay")
display.SetInput(filter)
display.SetRepresentation(2)
display.UpdateVTKObjects()
undoStackBuilder.End()
undoStackBuilder.PushToStack();

undoStackBuilder.Begin("AddDisplay")
renModule.AddToDisplays(display)
renModule.UpdateVTKObjects()
undoStackBuilder.End()
undoStackBuilder.PushToStack();

undoStackBuilder.Begin("RemoveDisplay")
renModule.RemoveFromDisplays(display)
renModule.UpdateVTKObjects()
pxm.SMProxyManager.UnRegisterProxy("displays", "sphereDisplay")
undoStackBuilder.End()
undoStackBuilder.PushToStack();

undoStackBuilder.Begin("CleanupSources")
pxm.SMProxyManager.UnRegisterProxy("filters", "elevationFilter")
pxm.SMProxyManager.UnRegisterProxy("mygroup", "sphere")
pxm.SMProxyManager.UnRegisterProxy("mygroup", "cube")
#  Redoing this set should delete the proxies, if not, we have some
#  extra references to the proxies-->BUG.
#  Undoing this set will create new proxies, if not, i.e. existing
#  proxies are used then---> BUG.
undoStackBuilder.End()
undoStackBuilder.PushToStack();
#
# Release all references to the objects we created, only the Proxy manager
# now keeps a reference to these proxies. This ensures that unregister
# deletes the proxies...this will check the proxy creation code in the
# undo/redo elements.
del proxy
del display
del proxy2
del filter


RenderAndWait(renModule)
def UpdateVTKObjects(pxm):
  pxm.UpdateRegisteredProxies("mygroup", 1)
  pxm.UpdateRegisteredProxies("filters", 1)
  pxm.UpdateRegisteredProxies("displays", 1)
  pxm.UpdateRegisteredProxies(1)
  return

print "** UNDO **"
while undoStack.GetNumberOfUndoSets() > 0:
  print "**** Undo: %s" % undoStack.GetUndoSetLabel(0)
  undoStack.Undo()
  UpdateVTKObjects(pxm)
  RenderAndWait(renModule)
  
print "** REDO **"
while undoStack.GetNumberOfRedoSets() > 0:
  print "**** Redo: %s" % undoStack.GetRedoSetLabel(0)
  undoStack.Redo()
  UpdateVTKObjects(pxm)
  RenderAndWait(renModule)

print "** UNDO REDO **"
# Undo cleanup to test baseline.
print "**** Undo: %s" % undoStack.GetUndoSetLabel(0)
undoStack.Undo()
UpdateVTKObjects(pxm)
print "**** Redo: %s" % undoStack.GetRedoSetLabel(0)
undoStack.Redo()
UpdateVTKObjects(pxm)
print "**** Undo: %s" % undoStack.GetUndoSetLabel(0)
undoStack.Undo()
UpdateVTKObjects(pxm)
print "**** Undo: %s" % undoStack.GetUndoSetLabel(0)
undoStack.Undo()
UpdateVTKObjects(pxm)

RenderAndWait(renModule)

if not smtesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)
  pass


