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
  time.sleep(.5)


SMPythonTesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(SMPythonTesting.SMStatesDir, "UndoRedo.pvsm")
print "State file: %s" % pvsm_file
SMPythonTesting.LoadServerManagerState(pvsm_file)

pxm = vtkSMObject.GetProxyManager()
renModule = pxm.GetProxy("rendermodules", "RenderModule0")

undoStack = vtkSMUndoStack()
pxm.SetUndoStack(undoStack)

self_cid = vtkProcessModuleConnectionManager.GetSelfConnectionID()
  
proxy = pxm.NewProxy("sources","SphereSource")
proxy2 = pxm.NewProxy("sources","CubeSource")
filter = pxm.NewProxy("filters", "ElevationFilter")

display = renModule.CreateDisplayProxy()

undoStack.BeginUndoSet(self_cid, "Create")
pxm.RegisterProxy("mygroup", "sphere", proxy)
pxm.RegisterProxy("mygroup", "cube", proxy2)
pxm.RegisterProxy("displays", "sphereDisplay", display)
pxm.RegisterProxy("filters", "elevationFilter", filter)

proxy.UnRegister(None)
proxy2.UnRegister(None)
display.UnRegister(None)
filter.UnRegister(None)

filter.GetProperty("Input").AddProxy(proxy)
filter.UpdateVTKObjects()
display.GetProperty("Input").AddProxy(filter)
display.GetProperty("Representation").SetElement(0, 2)
display.UpdateVTKObjects()

renModule.UpdateVTKObjects()

renModule.GetProperty("Displays").AddProxy(display)
renModule.UpdateVTKObjects()

undoStack.EndUndoSet()
pxm.UpdateRegisteredProxies(0)
renModule.StillRender()
  
undoStack.BeginUndoSet(self_cid, "Pipeline")
filter.GetProperty("Input").SetProxy(0, proxy2)
filter.UpdateVTKObjects()
undoStack.EndUndoSet()
 
# We will uncomment this one vtkSMProxy can be created with fixed IDs.
#del proxy
#del display
#del proxy2
#del filter

RenderAndWait(renModule)

print "Undoing.....(2)"
undoStack.Undo()
undoStack.Undo()
RenderAndWait(renModule)

print "Redoing.....(2)"
undoStack.Redo()
undoStack.Redo()
RenderAndWait(renModule)

print "Undoing.....(1)"
undoStack.Undo()
RenderAndWait(renModule)

if not SMPythonTesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)


