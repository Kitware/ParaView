# Tests animation.

import SMPythonTesting
import paraview

import os.path
import sys

paraview.ActiveConnection = paraview.connect()

SMPythonTesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(SMPythonTesting.SMStatesDir, "Animation.pvsm")
print "State file: %s" % pvsm_file

SMPythonTesting.LoadServerManagerState(pvsm_file)
pxm = paraview.pyProxyManager() 
rmProxy = pxm.GetProxy("rendermodules","RenderModule0")
rmProxy.StillRender()

amScene = pxm.GetProxy("animation_scene", "vtkPVAnimationScene_AnimationScene0")
amScene.SetAnimationTime(0)
amScene.Play()

amScene.SetAnimationTime(69)
if not SMPythonTesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)

