# script-version: 2.0
# Catalyst state generated using paraview version 5.9.0-RC1-70-g9f7915dbb6

#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# ----------------------------------------------------------------
# setup views used in the visualization
# ----------------------------------------------------------------

# Create a new 'Render View'
renderView1 = CreateView('RenderView')
renderView1.ViewSize = [844, 539]
renderView1.AxesGrid = 'GridAxes3DActor'
renderView1.StereoType = 'Crystal Eyes'
renderView1.CameraPosition = [0.0, 0.0, 66.92130429902464]
renderView1.CameraFocalDisk = 1.0
renderView1.CameraParallelScale = 17.320508075688775

SetActiveView(None)

# ----------------------------------------------------------------
# setup view layouts
# ----------------------------------------------------------------

# create new layout object 'Layout #1'
layout1 = CreateLayout(name='Layout #1')
layout1.AssignView(0, renderView1)
layout1.SetSize(844, 539)

# ----------------------------------------------------------------
# restore active view
SetActiveView(renderView1)
# ----------------------------------------------------------------

# ----------------------------------------------------------------
# setup the data processing pipelines
# ----------------------------------------------------------------

# create a new 'Wavelet'
wavelet1 = Wavelet(registrationName='Wavelet1')

# create a new 'Slice'
slice1 = Slice(registrationName='Slice1', Input=wavelet1)
slice1.SliceType = 'Plane'
slice1.HyperTreeGridSlicer = 'Plane'
slice1.SliceOffsetValues = [0.0]

# init the 'Plane' selected for 'SliceType'
slice1.SliceType.Normal = [0.0, 0.0, 1.0]

# ----------------------------------------------------------------
# setup the visualization in view 'renderView1'
# ----------------------------------------------------------------

# show data from wavelet1
wavelet1Display = Show(wavelet1, renderView1, 'UniformGridRepresentation')

# trace defaults for the display properties.
wavelet1Display.Representation = 'Outline'
wavelet1Display.ColorArrayName = ['POINTS', '']
wavelet1Display.SelectTCoordArray = 'None'
wavelet1Display.SelectNormalArray = 'None'
wavelet1Display.SelectTangentArray = 'None'
wavelet1Display.OSPRayScaleArray = 'RTData'
wavelet1Display.OSPRayScaleFunction = 'PiecewiseFunction'
wavelet1Display.SelectOrientationVectors = 'None'
wavelet1Display.ScaleFactor = 2.0
wavelet1Display.SelectScaleArray = 'RTData'
wavelet1Display.GlyphType = 'Arrow'
wavelet1Display.GlyphTableIndexArray = 'RTData'
wavelet1Display.GaussianRadius = 0.1
wavelet1Display.SetScaleArray = ['POINTS', 'RTData']
wavelet1Display.ScaleTransferFunction = 'PiecewiseFunction'
wavelet1Display.OpacityArray = ['POINTS', 'RTData']
wavelet1Display.OpacityTransferFunction = 'PiecewiseFunction'
wavelet1Display.DataAxesGrid = 'GridAxesRepresentation'
wavelet1Display.PolarAxes = 'PolarAxesRepresentation'
wavelet1Display.ScalarOpacityUnitDistance = 1.7320508075688774
wavelet1Display.OpacityArrayName = ['POINTS', 'RTData']
wavelet1Display.IsosurfaceValues = [157.0909652709961]
wavelet1Display.SliceFunction = 'Plane'
wavelet1Display.Slice = 10

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
wavelet1Display.ScaleTransferFunction.Points = [37.35310363769531, 0.0, 0.5, 0.0, 276.8288269042969, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
wavelet1Display.OpacityTransferFunction.Points = [37.35310363769531, 0.0, 0.5, 0.0, 276.8288269042969, 1.0, 0.5, 0.0]

# show data from slice1
slice1Display = Show(slice1, renderView1, 'GeometryRepresentation')

# get color transfer function/color map for 'RTData'
rTDataLUT = GetColorTransferFunction('RTData')
rTDataLUT.RGBPoints = [77.22376251220703, 0.231373, 0.298039, 0.752941, 177.02629470825195, 0.865003, 0.865003, 0.865003, 276.8288269042969, 0.705882, 0.0156863, 0.14902]
rTDataLUT.ScalarRangeInitialized = 1.0

# trace defaults for the display properties.
slice1Display.Representation = 'Surface'
slice1Display.ColorArrayName = ['POINTS', 'RTData']
slice1Display.LookupTable = rTDataLUT
slice1Display.SelectTCoordArray = 'None'
slice1Display.SelectNormalArray = 'None'
slice1Display.SelectTangentArray = 'None'
slice1Display.OSPRayScaleArray = 'RTData'
slice1Display.OSPRayScaleFunction = 'PiecewiseFunction'
slice1Display.SelectOrientationVectors = 'None'
slice1Display.ScaleFactor = 2.0
slice1Display.SelectScaleArray = 'RTData'
slice1Display.GlyphType = 'Arrow'
slice1Display.GlyphTableIndexArray = 'RTData'
slice1Display.GaussianRadius = 0.1
slice1Display.SetScaleArray = ['POINTS', 'RTData']
slice1Display.ScaleTransferFunction = 'PiecewiseFunction'
slice1Display.OpacityArray = ['POINTS', 'RTData']
slice1Display.OpacityTransferFunction = 'PiecewiseFunction'
slice1Display.DataAxesGrid = 'GridAxesRepresentation'
slice1Display.PolarAxes = 'PolarAxesRepresentation'

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
slice1Display.ScaleTransferFunction.Points = [77.22376251220703, 0.0, 0.5, 0.0, 276.8288269042969, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
slice1Display.OpacityTransferFunction.Points = [77.22376251220703, 0.0, 0.5, 0.0, 276.8288269042969, 1.0, 0.5, 0.0]

# ----------------------------------------------------------------
# setup color maps and opacity mapes used in the visualization
# note: the Get..() functions create a new object, if needed
# ----------------------------------------------------------------

# get opacity transfer function/opacity map for 'RTData'
rTDataPWF = GetOpacityTransferFunction('RTData')
rTDataPWF.Points = [77.22376251220703, 0.0, 0.5, 0.0, 276.8288269042969, 1.0, 0.5, 0.0]
rTDataPWF.ScalarRangeInitialized = 1

# ----------------------------------------------------------------
# setup extractors
# ----------------------------------------------------------------

# create extractor
pNG1 = CreateExtractor('PNG', renderView1, registrationName='PNG1')
# trace defaults for the extractor.
# init the 'PNG' selected for 'Writer'
pNG1.Writer.FileName = 'pipeline1_%.6ts%cm.png'
pNG1.Writer.ImageResolution = [400, 400]
pNG1.Writer.Format = 'PNG'

# ----------------------------------------------------------------
# restore active source
SetActiveSource(pNG1)
# ----------------------------------------------------------------

# ------------------------------------------------------------------------------
# Catalyst options
from paraview import catalyst
options = catalyst.Options()

# init the 'TimeStep' selected for 'GlobalTrigger'
options.GlobalTrigger.UseStartTimeStep = 1
options.GlobalTrigger.StartTimeStep = 1
options.GlobalTrigger.Frequency = 3

# ------------------------------------------------------------------------------
if __name__ == '__main__':
    from paraview.simple import SaveExtractsUsingCatalystOptions
    # Code for non in-situ environments; if executing in post-processing
    # i.e. non-Catalyst mode, lets generate extracts using Catalyst options
    SaveExtractsUsingCatalystOptions(options)
