# Tests animation.

from paraview import smtesting
from paraview import servermanager

import os.path
import sys

servermanager.Connect()

smtesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(smtesting.SMStatesDir, "Animation.pvsm")
print "State file: %s" % pvsm_file

smtesting.LoadServerManagerState(pvsm_file)
pxm = servermanager.ProxyManager()
rmProxy = pxm.GetProxy("rendermodules","RenderModule0")
rmProxy.StillRender()

amScene = pxm.GetProxy("animation_scene", "vtkPVAnimationScene_AnimationScene0")
amScene.SetAnimationTime(0)
amScene.InvokeCommand("Play");

amScene.SetAnimationTime(69)
amScene.UpdateVTKObjects();
if not smtesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)
