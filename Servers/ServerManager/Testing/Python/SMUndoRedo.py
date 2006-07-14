# Test Undo/Redo.
# Tests registering/unregistering/property modification.

import SMPythonTesting

import os.path
import sys
import time

if os.name == "posix":
  from libvtkPVServerCommonPython import *
  from libvtkPVServerManagerPython import *
else:
  from vtkPVServerCommonPython import *
  from vtkPVServerManagerPython import *

def RenderAndWait(ren):
  ren.StillRender()
  #time.sleep(.5)


SMPythonTesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(SMPythonTesting.SMStatesDir, "UndoRedo.pvsm")
print "State file: %s" % pvsm_file
SMPythonTesting.LoadServerManagerState(pvsm_file)

pxm = vtkSMObject.GetProxyManager()
renModule = pxm.GetProxy("rendermodules", "RenderModule0")
renModule.UpdateVTKObjects()

undoStack = vtkSMUndoStack()

self_cid = vtkProcessModuleConnectionManager.GetSelfConnectionID()
  
proxy = pxm.NewProxy("sources","SphereSource")
proxy.UnRegister(None)
proxy2 = pxm.NewProxy("sources","CubeSource")
proxy2.UnRegister(None)

filter = pxm.NewProxy("filters", "ElevationFilter")
filter.UnRegister(None)
display = renModule.CreateDisplayProxy()
display.UnRegister(None)
 
undoStack.BeginOrContinueUndoSet(self_cid, "CreateFilter")
pxm.RegisterProxy("mygroup", "sphere", proxy)
pxm.RegisterProxy("mygroup", "cube", proxy2)
pxm.RegisterProxy("filters", "elevationFilter", filter)
undoStack.EndUndoSet()

undoStack.BeginOrContinueUndoSet(self_cid, "FilterInput")
filter.GetProperty("Input").AddProxy(proxy)
filter.UpdateVTKObjects()
undoStack.EndUndoSet()
  
undoStack.BeginOrContinueUndoSet(self_cid, "CreateDisplay")
pxm.RegisterProxy("displays", "sphereDisplay", display)
undoStack.EndUndoSet()

undoStack.BeginOrContinueUndoSet(self_cid, "SetupDisplay")
display.GetProperty("Input").AddProxy(filter)
display.GetProperty("Representation").SetElement(0, 2)
display.UpdateVTKObjects()
undoStack.EndUndoSet()

undoStack.BeginOrContinueUndoSet(self_cid, "AddDisplay")
renModule.GetProperty("Displays").AddProxy(display)
renModule.UpdateVTKObjects()
undoStack.EndUndoSet()

undoStack.BeginOrContinueUndoSet(self_cid, "RemoveDisplay")
renModule.GetProperty("Displays").RemoveProxy(display)
renModule.UpdateVTKObjects()
pxm.UnRegisterProxy("displays", "sphereDisplay")
undoStack.EndUndoSet()

undoStack.BeginOrContinueUndoSet(self_cid, "CleanupSources")
pxm.UnRegisterProxy("filters", "elevationFilter")
pxm.UnRegisterProxy("mygroup", "sphere")
pxm.UnRegisterProxy("mygroup", "cube")
#  Redoing this set should delete the proxies, if not, we have some
#  extra references to the proxies-->BUG.
#  Undoing this set will create new proxies, if not, i.e. existing
#  proxies are used then---> BUG.
undoStack.EndUndoSet()
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

if not SMPythonTesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)
  pass


