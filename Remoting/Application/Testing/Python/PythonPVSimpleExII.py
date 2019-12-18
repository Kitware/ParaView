from paraview.simple import *
import sys
from paraview import smtesting

smtesting.ProcessCommandLineArguments()
reader = ExodusIIReader(FileName=smtesting.DataDir+'/Testing/Data/can.ex2')

if len(reader.TimestepValues) != 44:
    raise smtesting.TestError('Wrong amount of time steps.')

if reader.TimestepValues[0] != 0.0 or reader.TimestepValues[-1] != 0.004299988504499197:
    raise smtesting.TestError('Wrong time step value.')

fields = reader.PointVariables

if 'DISPL' not in fields:
    raise smtesting.TestError('DISPL not available.')

if 'VEL' not in fields:
    raise smtesting.TestError('VEL not available.')

if 'ACCL' not in fields:
    raise smtesting.TestError('ACCL not available.')

fields = reader.PointVariables.Available

if 'DISPL' not in fields:
    raise smtesting.TestError('DISPL not available.')

if 'VEL' not in fields:
    raise smtesting.TestError('VEL not available.')

if 'ACCL' not in fields:
    raise smtesting.TestError('ACCL not available.')

reader.PointVariables = ["DISPL"]
fields = reader.PointVariables

if 'DISPL' not in fields:
    raise smtesting.TestError('DISPL not available.')

if 'VEL' in fields:
    raise smtesting.TestError('VEL should not available.')

if 'ACCL' in fields:
    raise smtesting.TestError('ACCL should not available.')

reader.UpdatePipeline()

# Now test that the default animation is setup correctly for the data.
Show()

RenderView1 = Render()
RenderView1.CameraPosition = [0.21706008911132812, 55.74057374685633, -5.110947132110596]
RenderView1.CameraViewUp = [0.0, 0.0, 1.0]
RenderView1.CameraFocalPoint = [0.21706008911132812, 4.0, -5.110947132110596]
ResetCamera()

GetAnimationScene().GoToLast()
GetAnimationScene().Play()
GetAnimationScene().GoToFirst()
GetAnimationScene().GoToLast()
GetAnimationScene().GoToPrevious()
GetAnimationScene().GoToPrevious()
GetAnimationScene().GoToPrevious()
GetAnimationScene().GoToPrevious()
GetAnimationScene().GoToNext()

if not smtesting.DoRegressionTesting(GetActiveView().SMProxy):
  # This will lead to VTK object leaks.
  sys.exit(1)
