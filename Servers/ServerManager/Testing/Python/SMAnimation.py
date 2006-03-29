# Tests animation.

import SMPythonTesting

import os.path
import sys

if os.name == "posix":
  from libvtkPVServerCommonPython import *
  from libvtkPVServerManagerPython import *
else:
  from vtkPVServerCommonPython import *
  from vtkPVServerManagerPython import *


SMPythonTesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(SMPythonTesting.SMStatesDir, "Animation.pvsm")
print "State file: %s" % pvsm_file

SMPythonTesting.LoadServerManagerState(pvsm_file)
pxm = vtkSMObject.GetProxyManager()
rmProxy = pxm.GetProxy("rendermodules","RenderModule0")
rmProxy.StillRender()

amScene = pxm.GetProxy("animation_scene", "vtkPVAnimationScene_AnimationScene0")
amScene.SetAnimationTime(0)
amScene.Play()

amScene.SetAnimationTime(69)
if not SMPythonTesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)

