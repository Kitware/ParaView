from paraview.simple import *
from paraview import smtesting
import vtk
import vtk.vtkRenderingVolume
import os

paraview.simple._DisableFirstRenderCameraReset()

# need a baseline?
saveImage = False

# check for driver support first
rw = vtk.vtkRenderWindow()
rw.SetSize(1,1)
rw.Render()
ptm = vtk.vtkProjectedTetrahedraMapper()
ok = ptm.IsSupported(rw)
print
print ('ProjectedTetrahedraMapper %s supported '%(
      'is' if(ok) else 'is not'))
del ptm
del rw

if ok:
  smtesting.ProcessCommandLineArguments()
  smtesting.LoadServerManagerState(smtesting.StateXMLFileName)

  view = GetRenderView()
  view.RemoteRenderThreshold = 0;

  if saveImage:
    SetActiveView(view)
    Render()
    imageFile = os.path.splitext(os.path.basename(smtesting.StateXMLFileName))[0]
    WriteImage('%s/../../%s.png'%(smtesting.TempDir, imageFile))

  if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError ('Test failed.')

  print
  print ('Test passes')

else:
  print ('Skipped untested.')

print
