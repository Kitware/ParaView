from paraview.vtk.util.misc import vtkGetTempDir, vtkGetDataRoot
import os.path

#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'IOSS Reader'
canex2 = IOSSReader(registrationName='can.ex2', FileName=[os.path.join(vtkGetDataRoot(), 'Testing/Data/can.ex2')])

# get animation scene
animationScene1 = GetAnimationScene()

# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')

# show data in view
canex2Display = Show(canex2, renderView1, 'UnstructuredGridRepresentation')

# trace defaults for the display properties.
canex2Display.Representation = 'Surface'

# reset view to fit data
renderView1.ResetCamera(False, 0.9)

# update the view to ensure updated data information
renderView1.Update()

# set scalar coloring
ColorBy(canex2Display, ('FIELD', 'vtkBlockColors'))

# show color bar/color legend
canex2Display.SetScalarBarVisibility(renderView1, True)

# get color transfer function/color map for 'vtkBlockColors'
vtkBlockColorsLUT = GetColorTransferFunction('vtkBlockColors')

# get opacity transfer function/opacity map for 'vtkBlockColors'
vtkBlockColorsPWF = GetOpacityTransferFunction('vtkBlockColors')

# get 2D transfer function for 'vtkBlockColors'
vtkBlockColorsTF2D = GetTransferFunction2D('vtkBlockColors')

# turn off scalar coloring
ColorBy(canex2Display, None)

# Hide the scalar bar for this color map if no visible data is colored by it.
HideScalarBarIfNotNeeded(vtkBlockColorsLUT, renderView1)

# export view, combining all timesteps into a single Alembic file rather than
# writing one file per timestep.
filename = os.path.join(vtkGetTempDir(), "tmp.abc")
ExportView(filename, view=renderView1, WriteTimeSteps=1, FrameWindow=[0, 3],
           WriteTimeStepsAsSingleFile=1)

assert os.path.isfile(filename), "Expected exported Alembic file %s to exist" % filename
assert os.path.getsize(filename) > 0, "Expected exported Alembic file %s to be non-empty" % filename
