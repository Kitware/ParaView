from paraview.vtk.util.misc import vtkGetTempDir, vtkGetDataRoot
import os.path

from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

canex2 = IOSSReader(registrationName='can.ex2', FileName=[os.path.join(vtkGetDataRoot(), 'Testing/Data/can.ex2')])

animationScene1 = GetAnimationScene()
animationScene1.UpdateAnimationUsingDataTimeSteps()
renderView1 = GetActiveViewOrCreate('RenderView')
canex2Display = Show(canex2, renderView1, 'UnstructuredGridRepresentation')
canex2Display.Representation = 'Surface'
renderView1.ResetCamera(False, 0.9)
renderView1.Update()

# set scalar coloring
ColorBy(canex2Display, ('FIELD', 'vtkBlockColors'))

# show color bar/color legend
canex2Display.SetScalarBarVisibility(renderView1, True)

# get color transfer function/color map for 'vtkBlockColors'
vtkBlockColorsLUT = GetColorTransferFunction('vtkBlockColors')

# turn off scalar coloring
ColorBy(canex2Display, None)

# Hide the scalar bar for this color map if no visible data is colored by it.
HideScalarBarIfNotNeeded(vtkBlockColorsLUT, renderView1)

# export view, combining all timesteps into a single USD file rather than
# writing one file per timestep.
filename = os.path.join(vtkGetTempDir(), "tmp.usda")
ExportView(filename, view=renderView1, WriteTimeSteps=1, FrameWindow=[0, 3],
           WriteTimeStepsToSingleFile=1)

assert os.path.isfile(filename), "Expected exported USD file %s to exist" % filename
assert os.path.getsize(filename) > 0, "Expected exported USD file %s to be non-empty" % filename

with open(filename, "r") as f:
    contents = f.read()
assert "timeSamples" in contents, \
    "Expected exported USD file %s to contain time-sampled attributes" % filename
