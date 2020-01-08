#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'Wavelet'
wavelet1 = Wavelet()

# set active source
SetActiveSource(wavelet1)

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [228, 284]

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
wavelet1Display.ScalarOpacityUnitDistance = 1.7320508075688779
wavelet1Display.Slice = 10

# reset view to fit data
renderView1.ResetCamera()

# show data in view
wavelet1Display = Show(wavelet1, renderView1)

# reset view to fit data
renderView1.ResetCamera()

# create a new 'Calculator'
calculator1 = Calculator(Input=wavelet1)
calculator1.Function = ''

# Properties modified on calculator1
calculator1.Function = 'RTData*iHat + ln(RTData)*jHat + coordsZ*kHat'

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [1158, 833]

# show data in view
calculator1Display = Show(calculator1, renderView1)
# trace defaults for the display properties.
calculator1Display.Representation = 'Outline'
calculator1Display.ColorArrayName = ['POINTS', '']
calculator1Display.OSPRayScaleArray = 'RTData'
calculator1Display.OSPRayScaleFunction = 'PiecewiseFunction'
calculator1Display.SelectOrientationVectors = 'Result'
calculator1Display.ScaleFactor = 2.0
calculator1Display.SelectScaleArray = 'RTData'
calculator1Display.GlyphType = 'Arrow'
calculator1Display.ScalarOpacityUnitDistance = 1.7320508075688779
calculator1Display.Slice = 10

# hide data in view
Hide(wavelet1, renderView1)

# set scalar coloring
ColorBy(calculator1Display, ('POINTS', 'Result'))

# rescale color and/or opacity maps used to include current data range
calculator1Display.RescaleTransferFunctionToDataRange(True)

# hide color bar/color legend
calculator1Display.SetScalarBarVisibility(renderView1, False)

# get color transfer function/color map for 'Result'
ResultLUT = GetColorTransferFunction('Result')

# get opacity transfer function/opacity map for 'Result'
ResultPWF = GetOpacityTransferFunction('Result')

# set scalar coloring
ColorBy(calculator1Display, ('POINTS', 'Result', 'Z'))

# change representation type
calculator1Display.SetRepresentationType('Surface')

# set scalar coloring
ColorBy(calculator1Display, ('POINTS', 'Result', 'X'))

# rescale color and/or opacity maps used to include current data range
calculator1Display.RescaleTransferFunctionToDataRange(True)

# hide color bar/color legend
calculator1Display.SetScalarBarVisibility(renderView1, False)

# Update a scalar bar component title.
UpdateScalarBarsComponentTitle(ResultLUT, calculator1Display)

#### saving camera placements for all active views

# current camera placement for renderView1
renderView1.CameraPosition = [0.0, 0.0, 82.35963323102031]
renderView1.CameraParallelScale = 21.57466795392812

Render(renderView1)

import os
import sys
try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print("Could not get baseline directory. Test failed.")
baseline_file = os.path.join(baselinePath, "ColorBy.png")
from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(renderView1.GetRenderWindow(), baseline_file, threshold=40)
Testing.interact()
