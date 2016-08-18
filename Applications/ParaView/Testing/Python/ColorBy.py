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

# create a new 'Random Vectors'
randomVectors1 = RandomVectors(Input=wavelet1)

# show data in view
randomVectors1Display = Show(randomVectors1, renderView1)
# trace defaults for the display properties.
randomVectors1Display.Representation = 'Outline'
randomVectors1Display.ColorArrayName = ['POINTS', '']
randomVectors1Display.OSPRayScaleArray = 'RTData'
randomVectors1Display.OSPRayScaleFunction = 'PiecewiseFunction'
randomVectors1Display.SelectOrientationVectors = 'BrownianVectors'
randomVectors1Display.ScaleFactor = 2.0
randomVectors1Display.SelectScaleArray = 'RTData'
randomVectors1Display.GlyphType = 'Arrow'
randomVectors1Display.ScalarOpacityUnitDistance = 1.7320508075688779
randomVectors1Display.Slice = 10

# hide data in view
Hide(wavelet1, renderView1)

# set scalar coloring
ColorBy(randomVectors1Display, ('POINTS', 'BrownianVectors'))

# rescale color and/or opacity maps used to include current data range
randomVectors1Display.RescaleTransferFunctionToDataRange(True)

# show color bar/color legend
randomVectors1Display.SetScalarBarVisibility(renderView1, True)

# get color transfer function/color map for 'BrownianVectors'
brownianVectorsLUT = GetColorTransferFunction('BrownianVectors')

# get opacity transfer function/opacity map for 'BrownianVectors'
brownianVectorsPWF = GetOpacityTransferFunction('BrownianVectors')

# set scalar coloring
ColorBy(randomVectors1Display, ('POINTS', 'BrownianVectors', '2'))

# change representation type
randomVectors1Display.SetRepresentationType('Surface')

# set scalar coloring
ColorBy(randomVectors1Display, ('POINTS', 'BrownianVectors', '1'))

# set scalar coloring
ColorBy(randomVectors1Display, ('POINTS', 'RTData'))

# rescale color and/or opacity maps used to include current data range
randomVectors1Display.RescaleTransferFunctionToDataRange(True)

# show color bar/color legend
randomVectors1Display.SetScalarBarVisibility(renderView1, True)

# get color transfer function/color map for 'RTData'
rTDataLUT = GetColorTransferFunction('RTData')

# get opacity transfer function/opacity map for 'RTData'
rTDataPWF = GetOpacityTransferFunction('RTData')

# set scalar coloring
ColorBy(randomVectors1Display, ('POINTS', 'BrownianVectors'))

# rescale color and/or opacity maps used to include current data range
randomVectors1Display.RescaleTransferFunctionToDataRange(True)

# show color bar/color legend
randomVectors1Display.SetScalarBarVisibility(renderView1, True)

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
  print "Could not get baseline directory. Test failed."
baseline_file = os.path.join(baselinePath, "ColorBy.png")
import vtk.test.Testing
vtk.test.Testing.VTK_TEMP_DIR = vtk.util.misc.vtkGetTempDir()
vtk.test.Testing.compareImage(renderView1.GetRenderWindow(), baseline_file, threshold=40)
vtk.test.Testing.interact()
