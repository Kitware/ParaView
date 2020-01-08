#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'Wavelet'
wavelet1 = Wavelet()

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [1013, 781]
AssignViewToLayout(renderView1)

# show data in view
wavelet1Display = Show(wavelet1, renderView1)
# trace defaults for the display properties.
wavelet1Display.Representation = 'Outline'
wavelet1Display.ColorArrayName = ['POINTS', '']
wavelet1Display.ScalarOpacityUnitDistance = 1.7320508075688779
wavelet1Display.Slice = 10

# reset view to fit data
renderView1.ResetCamera()

# change representation type
wavelet1Display.SetRepresentationType('Surface')

# set scalar coloring
ColorBy(wavelet1Display, ('POINTS', 'RTData'))

# rescale color and/or opacity maps used to include current data range
wavelet1Display.RescaleTransferFunctionToDataRange(True)

# show color bar/color legend
wavelet1Display.SetScalarBarVisibility(renderView1, True)

# get color transfer function/color map for 'RTData'
rTDataLUT = GetColorTransferFunction('RTData')

# get opacity transfer function/opacity map for 'RTData'
rTDataPWF = GetOpacityTransferFunction('RTData')

# get layout
viewLayout1 = GetLayout()

# split cell
viewLayout1.SplitHorizontal(0, 0.5)

# set active view
SetActiveView(None)

# Create a new 'Histogram View'
histogramView1 = CreateView('XYHistogramChartView')
histogramView1.ViewSize = [500, 780]

# place view in the layout
viewLayout1.AssignView(2, histogramView1)

# set active source
SetActiveSource(wavelet1)

# show data in view
histogram = Show(wavelet1, histogramView1)
# trace defaults for the display properties.
histogram.SelectInputArray = ['POINTS', 'RTData']
histogram.CustomBinRanges = [37.35310363769531, 276.8288269042969]
histogram.UseColorMapping = True
histogram.LookupTable = rTDataLUT

Render(histogramView1)

# compare histogramView with baseline image
import os
import sys
try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print ("Could not get baseline directory. Test failed.")
baseline_file = os.path.join(baselinePath, "TestColorHistogram.png")

from paraview.vtk.test import Testing
from paraview.vtk.util.misc import vtkGetTempDir
Testing.VTK_TEMP_DIR = vtkGetTempDir()
Testing.compareImage(histogramView1.GetRenderWindow(), baseline_file, threshold=40)
Testing.interact()
