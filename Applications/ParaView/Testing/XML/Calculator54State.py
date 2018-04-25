# state file generated using paraview version 5.4.1

# ----------------------------------------------------------------
# setup views used in the visualization
# ----------------------------------------------------------------

#### import the simple module from the paraview
from paraview.simple import *

# the compatibility information has to be manually set
paraview.compatibility.major = 5
paraview.compatibility.minor = 4
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# Create a new 'Render View'
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [1102, 1142]
renderView1.AxesGrid = 'GridAxes3DActor'
renderView1.StereoType = 0
renderView1.CameraPosition = [0.0, 0.0, 69.19033689105265]
renderView1.CameraParallelScale = 17.949201653753704
renderView1.Background = [0.32, 0.34, 0.43]

# ----------------------------------------------------------------
# setup the data processing pipelines
# ----------------------------------------------------------------

# create a new 'Wavelet'
wavelet1 = Wavelet()

# create a new 'Calculator'
calculator1 = Calculator(Input=wavelet1)
calculator1.AttributeMode = 'Cell Data'
calculator1.Function = '1'

# ----------------------------------------------------------------
# setup the visualization in view 'renderView1'
# ----------------------------------------------------------------

# show data from calculator1
calculator1Display = Show(calculator1, renderView1)
# trace defaults for the display properties.
calculator1Display.Representation = 'Outline'
calculator1Display.ColorArrayName = ['POINTS', '']
calculator1Display.EdgeColor = [0.0, 0.0, 0.0]
calculator1Display.OSPRayScaleArray = 'RTData'
calculator1Display.OSPRayScaleFunction = 'PiecewiseFunction'
calculator1Display.SelectOrientationVectors = 'None'
calculator1Display.ScaleFactor = 2.0
calculator1Display.SelectScaleArray = 'RTData'
calculator1Display.GlyphType = 'Arrow'
calculator1Display.GlyphTableIndexArray = 'RTData'
calculator1Display.DataAxesGrid = 'GridAxesRepresentation'
calculator1Display.PolarAxes = 'PolarAxesRepresentation'
calculator1Display.ScalarOpacityUnitDistance = 1.7320508075688779
calculator1Display.Slice = 10

# ----------------------------------------------------------------
# finally, restore active source
SetActiveSource(calculator1)
# ----------------------------------------------------------------
