# This test tests BUG #17849.  The issue was that when
# the script called `animationScene1.UpdateAnimationUsingDataTimeSteps()`, the
# timesteps information it used was obsolete in pvpython since nothing called
# `UpdatePipelineInformation` on the reader after mode-shapes property change.
# The fix ensures that `UpdateAnimationUsingDataTimeSteps` calls
# `UpdatePipelineInformation` on sources that could have potentially changed.

#### import the simple module from the paraview
from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

# create a new 'ExodusIIReader'
canex2 = OpenDataFile(smtesting.DataDir + "/Testing/Data/can.ex2")

# get animation scene
animationScene1 = GetAnimationScene()

# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

# Properties modified on canex2
canex2.ElementBlocks = ['Unnamed block ID: 1 Type: HEX', 'Unnamed block ID: 2 Type: HEX']
canex2.HasModeShapes = 1

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
renderView1.ViewSize = [300, 300]

# show data in view
canex2Display = Show(canex2, renderView1)

# reset view to fit data
renderView1.CameraPosition = [0.21706008911132812, -47.74057374685633, -5.110947132110596]
renderView1.CameraFocalPoint = [0.21706008911132812, 4.0, -5.110947132110596]
renderView1.CameraViewUp = [0.0, 0.0, 1.0]
renderView1.CameraParallelScale = 13.391445890217907
renderView1.ResetCamera()

# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

# set scalar coloring
ColorBy(canex2Display, ('FIELD', 'vtkBlockColors'))

# Properties modified on canex2
canex2.ModeShape = 3

Render()

animationScene1.GoToFirst()
animationScene1.GoToNext()

if not smtesting.DoRegressionTesting(renderView1.SMProxy):
  # This will lead to VTK object leaks.
  sys.exit(1)
