from paraview.simple import *
from paraview import smtesting
import vtk
import vtk.vtkRenderingLIC
import os

paraview.simple._DisableFirstRenderCameraReset()

# need a baseline?
saveImage = False

# check for driver support first
rw = vtk.vtkRenderWindow()
rw.Render()
em = rw.GetExtensionManager()
painter = vtk.vtkRenderingLIC.vtkSurfaceLICPainter()
ok = painter.IsSupported(rw)
print
print 'SurfaceLIC %s Supported by:\n  %s\n  %s\n  %s\n'%(
      'is' if(ok) else 'is not',
      em.GetDriverGLVersion(),
      em.GetDriverGLVendor(),
      em.GetDriverGLRenderer())
del painter
del rw

if ok:
  smtesting.ProcessCommandLineArguments()

  LoadDistributedPlugin('SurfaceLIC', True, globals())

  smtesting.LoadServerManagerState(smtesting.StateXMLFileName)

  view = GetRenderView()
  view.RemoteRenderThreshold = 0;

  if saveImage:
    SetActiveView(view)
    Render()
    imageFile = os.path.splitext(os.path.basename(smtesting.StateXMLFileName))[0]
    WriteImage('%s/../../%s.png'%(smtesting.TempDir, imageFile))

  if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError, 'Test failed.'

  print
  print 'Test passes'

else:
  print 'Skipped untested.'

print
