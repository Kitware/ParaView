import paraview
paraview.compatibility.major = 6
paraview.compatibility.minor = 0

#### import the simple module from the paraview
from paraview.simple import *

#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

from paraview import smtesting
smtesting.ProcessCommandLineArguments()

# Create a new 'Render View'
renderView1 = CreateView('RenderView')
renderView1.Set(
    ViewSize=[934, 685],
    CameraPosition=[66.0, 0.0, 0.],
)

fastUniformGrid1 = FastUniformGrid(registrationName='FastUniformGrid1')
fastUniformGrid1Display = Show(fastUniformGrid1, renderView1, 'UniformGridRepresentation')

# get color transfer function/color map for 'Swirl'
swirlLUT = GetColorTransferFunction('Swirl')
swirlLUT.Set(
    RGBPoints=GenerateRGBPoints(
        range_min=-10,
        range_max=10,
    ),
    ScalarRangeInitialized=1.0,
    VectorComponent=1,
    VectorMode='Component',
)

# trace defaults for the display properties.
fastUniformGrid1Display.Set(
    Representation='Surface',
    ColorArrayName=['POINTS', 'Swirl'],
    LookupTable=swirlLUT,
)

# init the 'Piecewise Function' selected for 'ScaleTransferFunction'
fastUniformGrid1Display.ScaleTransferFunction.Points = [-10.0, 0.0, 0.5, 0.0, 10.0, 1.0, 0.5, 0.0]

# init the 'Piecewise Function' selected for 'OpacityTransferFunction'
fastUniformGrid1Display.OpacityTransferFunction.Points = [-10.0, 0.0, 0.5, 0.0, 10.0, 1.0, 0.5, 0.0]

# setup the color legend parameters for each legend in this view

# get color legend/bar for swirlLUT in view renderView1
swirlLUTColorBar = GetScalarBar(swirlLUT, renderView1)
swirlLUTColorBar.Set(
    Title='Swirl',
    ComponentTitle='Y',
)

# set color bar visibility
swirlLUTColorBar.Visibility = 1

# show color legend
fastUniformGrid1Display.SetScalarBarVisibility(renderView1, True)

# ----------------------------------------------------------------
# setup color maps and opacity maps used in the visualization
# note: the Get..() functions create a new object, if needed
# ----------------------------------------------------------------

# get opacity transfer function/opacity map for 'Swirl'
swirlPWF = GetOpacityTransferFunction('Swirl')
swirlPWF.Set(
    Points=[-10.0, 0.0, 0.5, 0.0, 10.0, 1.0, 0.5, 0.0],
    ScalarRangeInitialized=1,
)

fastUniformGrid1.WholeExtent[5] = 20# = [-10, 10, -10, 10, -10, 20]
fastUniformGrid1.UpdatePipeline()

fastUniformGrid1Display.RescaleTransferFunctionToDataRange(True, True)

assert swirlPWF.GetProperty("Points").GetData() == [-10.0, 0.0, 0.5, 0.0, 20.0, 1.0, 0.5, 0.0]
