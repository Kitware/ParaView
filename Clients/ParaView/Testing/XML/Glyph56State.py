# trace generated using paraview version 5.5.2

#### import the simple module from the paraview
from paraview.simple import *

# the compatibility information has to be manually set
paraview.compatibility.major = 5
paraview.compatibility.minor = 5

#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'Wavelet'
wavelet1 = Wavelet()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [1436, 898]

# show data in view
wavelet1Display = Show(wavelet1, renderView1)

# trace defaults for the display properties.
wavelet1Display.Representation = 'Outline'
wavelet1Display.ColorArrayName = ['POINTS', '']
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
wavelet1Display.SelectionCellLabelFontFile = ''
wavelet1Display.SelectionPointLabelFontFile = ''
wavelet1Display.PolarAxes = 'PolarAxesRepresentation'
wavelet1Display.ScalarOpacityUnitDistance = 1.7320508075688779
wavelet1Display.IsosurfaceValues = [157.0909652709961]
wavelet1Display.Slice = 10

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
wavelet1Display.ScaleTransferFunction.Points = [37.35310363769531, 0.0, 0.5, 0.0, 276.8288269042969, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
wavelet1Display.OpacityTransferFunction.Points = [37.35310363769531, 0.0, 0.5, 0.0, 276.8288269042969, 1.0, 0.5, 0.0]

# init the 'GridAxesRepresentation' selected for 'DataAxesGrid'
wavelet1Display.DataAxesGrid.XTitleFontFile = ''
wavelet1Display.DataAxesGrid.YTitleFontFile = ''
wavelet1Display.DataAxesGrid.ZTitleFontFile = ''
wavelet1Display.DataAxesGrid.XLabelFontFile = ''
wavelet1Display.DataAxesGrid.YLabelFontFile = ''
wavelet1Display.DataAxesGrid.ZLabelFontFile = ''

# init the 'PolarAxesRepresentation' selected for 'PolarAxes'
wavelet1Display.PolarAxes.PolarAxisTitleFontFile = ''
wavelet1Display.PolarAxes.PolarAxisLabelFontFile = ''
wavelet1Display.PolarAxes.LastRadialAxisTextFontFile = ''
wavelet1Display.PolarAxes.SecondaryRadialAxesTextFontFile = ''

# reset view to fit data
renderView1.ResetCamera()

# update the view to ensure updated data information
renderView1.Update()

# create a new 'Gradient'
gradient1 = Gradient(Input=wavelet1)
gradient1.SelectInputScalars = ['POINTS', 'RTData']

# show data in view
gradient1Display = Show(gradient1, renderView1)

# trace defaults for the display properties.
gradient1Display.Representation = 'Outline'
gradient1Display.ColorArrayName = ['POINTS', '']
gradient1Display.OSPRayScaleArray = 'RTDataGradient'
gradient1Display.OSPRayScaleFunction = 'PiecewiseFunction'
gradient1Display.SelectOrientationVectors = 'None'
gradient1Display.ScaleFactor = 2.0
gradient1Display.SelectScaleArray = 'RTDataGradient'
gradient1Display.GlyphType = 'Arrow'
gradient1Display.GlyphTableIndexArray = 'RTDataGradient'
gradient1Display.GaussianRadius = 0.1
gradient1Display.SetScaleArray = ['POINTS', 'RTDataGradient']
gradient1Display.ScaleTransferFunction = 'PiecewiseFunction'
gradient1Display.OpacityArray = ['POINTS', 'RTDataGradient']
gradient1Display.OpacityTransferFunction = 'PiecewiseFunction'
gradient1Display.DataAxesGrid = 'GridAxesRepresentation'
gradient1Display.SelectionCellLabelFontFile = ''
gradient1Display.SelectionPointLabelFontFile = ''
gradient1Display.PolarAxes = 'PolarAxesRepresentation'
gradient1Display.ScalarOpacityUnitDistance = 1.7320508075688779
gradient1Display.IsosurfaceValues = [1.0375213623046875]
gradient1Display.Slice = 10

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
gradient1Display.ScaleTransferFunction.Points = [-15.3538818359375, 0.0, 0.5, 0.0, 17.428924560546875, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
gradient1Display.OpacityTransferFunction.Points = [-15.3538818359375, 0.0, 0.5, 0.0, 17.428924560546875, 1.0, 0.5, 0.0]

# init the 'GridAxesRepresentation' selected for 'DataAxesGrid'
gradient1Display.DataAxesGrid.XTitleFontFile = ''
gradient1Display.DataAxesGrid.YTitleFontFile = ''
gradient1Display.DataAxesGrid.ZTitleFontFile = ''
gradient1Display.DataAxesGrid.XLabelFontFile = ''
gradient1Display.DataAxesGrid.YLabelFontFile = ''
gradient1Display.DataAxesGrid.ZLabelFontFile = ''

# init the 'PolarAxesRepresentation' selected for 'PolarAxes'
gradient1Display.PolarAxes.PolarAxisTitleFontFile = ''
gradient1Display.PolarAxes.PolarAxisLabelFontFile = ''
gradient1Display.PolarAxes.LastRadialAxisTextFontFile = ''
gradient1Display.PolarAxes.SecondaryRadialAxesTextFontFile = ''

# hide data in view
Hide(wavelet1, renderView1)

# update the view to ensure updated data information
renderView1.Update()

# create a new 'Glyph'
glyph1 = Glyph(Input=gradient1,
    GlyphType='Arrow')
glyph1.Scalars = ['POINTS', 'None']
glyph1.Vectors = ['POINTS', 'None']
glyph1.ScaleFactor = 2.0
glyph1.GlyphTransform = 'Transform2'

# show data in view
glyph1Display = Show(glyph1, renderView1)

# get color transfer function/color map for 'RTDataGradient'
rTDataGradientLUT = GetColorTransferFunction('RTDataGradient')

# trace defaults for the display properties.
glyph1Display.Representation = 'Surface'
glyph1Display.ColorArrayName = ['POINTS', 'RTDataGradient']
glyph1Display.LookupTable = rTDataGradientLUT
glyph1Display.OSPRayScaleArray = 'RTDataGradient'
glyph1Display.OSPRayScaleFunction = 'PiecewiseFunction'
glyph1Display.SelectOrientationVectors = 'None'
glyph1Display.ScaleFactor = 2.2
glyph1Display.SelectScaleArray = 'RTDataGradient'
glyph1Display.GlyphType = 'Arrow'
glyph1Display.GlyphTableIndexArray = 'RTDataGradient'
glyph1Display.GaussianRadius = 0.11
glyph1Display.SetScaleArray = ['POINTS', 'RTDataGradient']
glyph1Display.ScaleTransferFunction = 'PiecewiseFunction'
glyph1Display.OpacityArray = ['POINTS', 'RTDataGradient']
glyph1Display.OpacityTransferFunction = 'PiecewiseFunction'
glyph1Display.DataAxesGrid = 'GridAxesRepresentation'
glyph1Display.SelectionCellLabelFontFile = ''
glyph1Display.SelectionPointLabelFontFile = ''
glyph1Display.PolarAxes = 'PolarAxesRepresentation'

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
glyph1Display.ScaleTransferFunction.Points = [-15.280296325683594, 0.0, 0.5, 0.0, 17.390480041503906, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
glyph1Display.OpacityTransferFunction.Points = [-15.280296325683594, 0.0, 0.5, 0.0, 17.390480041503906, 1.0, 0.5, 0.0]

# init the 'GridAxesRepresentation' selected for 'DataAxesGrid'
glyph1Display.DataAxesGrid.XTitleFontFile = ''
glyph1Display.DataAxesGrid.YTitleFontFile = ''
glyph1Display.DataAxesGrid.ZTitleFontFile = ''
glyph1Display.DataAxesGrid.XLabelFontFile = ''
glyph1Display.DataAxesGrid.YLabelFontFile = ''
glyph1Display.DataAxesGrid.ZLabelFontFile = ''

# init the 'PolarAxesRepresentation' selected for 'PolarAxes'
glyph1Display.PolarAxes.PolarAxisTitleFontFile = ''
glyph1Display.PolarAxes.PolarAxisLabelFontFile = ''
glyph1Display.PolarAxes.LastRadialAxisTextFontFile = ''
glyph1Display.PolarAxes.SecondaryRadialAxesTextFontFile = ''

# show color bar/color legend
glyph1Display.SetScalarBarVisibility(renderView1, True)

# update the view to ensure updated data information
renderView1.Update()

# Properties modified on glyph1
glyph1.Vectors = ['POINTS', 'RTDataGradient']

# update the view to ensure updated data information
renderView1.Update()

# hide data in view
Hide(gradient1, renderView1)

# hide color bar/color legend
glyph1Display.SetScalarBarVisibility(renderView1, False)

# Properties modified on glyph1
glyph1.GlyphType = 'Cylinder'

# update the view to ensure updated data information
renderView1.Update()

#### saving camera placements for all active views

# current camera placement for renderView1
renderView1.CameraPosition = [0.0, 0.0, 66.92130429902464]
renderView1.CameraParallelScale = 17.320508075688775

#### uncomment the following to render all views
# RenderAllViews()
# alternatively, if you want to write images, you can use SaveScreenshot(...).
