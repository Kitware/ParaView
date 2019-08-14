#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'Sphere'
sphere1 = Sphere()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [2150, 1268]

# show data in view
sphere1Display = Show(sphere1, renderView1)
# trace defaults for the display properties.
sphere1Display.Representation = 'Surface'
sphere1Display.ColorArrayName = [None, '']
sphere1Display.OSPRayScaleArray = 'Normals'
sphere1Display.OSPRayScaleFunction = 'PiecewiseFunction'
sphere1Display.SelectOrientationVectors = 'None'
sphere1Display.ScaleFactor = 0.1
sphere1Display.SelectScaleArray = 'None'
sphere1Display.GlyphType = 'Arrow'
sphere1Display.GlyphTableIndexArray = 'None'
sphere1Display.DataAxesGrid = 'GridAxesRepresentation'
sphere1Display.PolarAxes = 'PolarAxesRepresentation'
sphere1Display.GaussianRadius = 0.05
sphere1Display.SetScaleArray = [None, '']
sphere1Display.ScaleTransferFunction = 'PiecewiseFunction'
sphere1Display.OpacityArray = [None, '']
sphere1Display.OpacityTransferFunction = 'PiecewiseFunction'

# reset view to fit data
renderView1.ResetCamera()

# update the view to ensure updated data information
renderView1.Update()

# set scalar coloring
ColorBy(sphere1Display, ('POINTS', 'Normals', 'Magnitude'))

# hide color bar/color legend
sphere1Display.SetScalarBarVisibility(renderView1, False)

# rescale color and/or opacity maps used to include current data range
sphere1Display.RescaleTransferFunctionToDataRange(True, False)

# show color bar/color legend
sphere1Display.SetScalarBarVisibility(renderView1, False)

# get color transfer function/color map for 'Normals'
normalsLUT = GetColorTransferFunction('Normals')

# Properties modified on renderView1.AxesGrid
renderView1.AxesGrid.Visibility = 1

#### saving camera placements for all active views

# current camera placement for renderView1
renderView1.CameraPosition = [1.735036754835695, 1.7227028122521102, 2.201932229033424]
renderView1.CameraViewUp = [-0.2360446232404711, 0.8468625927050057, -0.4765571161111957]
renderView1.CameraParallelScale = 0.8516115354228021

from paraview import smtesting
import os

smtesting.ProcessCommandLineArguments()

RenderAllViews()
# alternatively, if you want to write images, you can use SaveScreenshot(...).
imageFile = os.path.splitext(os.path.basename(smtesting.StateXMLFileName))[0]
SaveScreenshot('%s/%s.png' % (smtesting.TempDir, imageFile))

if not smtesting.DoRegressionTesting(renderView1.SMProxy):
    raise smtesting.TestError('Test failed')
