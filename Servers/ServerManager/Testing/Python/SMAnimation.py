# Tests animation.

from libvtkPVServerCommonPython import *
from libvtkPVServerManagerPython import *

import SMPythonTesting

import os.path
import sys

SMPythonTesting.ProcessCommandLineArguments()

pvsm_file = os.path.join(SMPythonTesting.SMStatesDir, "Animation.pvsm")
print pvsm_file

SMPythonTesting.LoadServerManagerState(pvsm_file)
pxm = vtkSMObject.GetProxyManager()
rmProxy = pxm.GetProxy("rendermodules","RenderModule0")
rmProxy.StillRender()

amScene = pxm.GetProxy("animation_scene", "vtkPVAnimationScene_AnimationScene0")
amScene.SetAnimationTime(0)
amScene.Play()

amScene.SetAnimationTime(69)
if not SMPythonTesting.DoRegressionTesting():
  sys.exit(1)


