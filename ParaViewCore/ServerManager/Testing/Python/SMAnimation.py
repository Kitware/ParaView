# Tests animation.

import SMPythonTesting
from paraview import servermanager

import os.path
import sys

servermanager.Connect()

SMPythonTesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(SMPythonTesting.SMStatesDir, "Animation.pvsm")
print "State file: %s" % pvsm_file

SMPythonTesting.LoadServerManagerState(pvsm_file)
pxm = servermanager.ProxyManager() 
rmProxy = pxm.GetProxy("rendermodules","RenderModule0")
rmProxy.StillRender()

amScene = pxm.GetProxy("animation_scene", "vtkPVAnimationScene_AnimationScene0")
amScene.SetAnimationTime(0)
amScene.InvokeCommand("Play");

amScene.SetAnimationTime(69)
amScene.UpdateVTKObjects();
if not SMPythonTesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)

