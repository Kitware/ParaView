from paraview.simple import *

# disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'Wavelet'
wavelet1 = Wavelet(registrationName='Wavelet1')

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')

# show data in view
wavelet1Display = Show(wavelet1, renderView1, 'UniformGridRepresentation')

# trace defaults for the display properties.
wavelet1Display.Representation = 'Outline'

# reset view to fit data
renderView1.ResetCamera(False)

# update the view to ensure updated data information
renderView1.Update()

# create a new 'Plot Over Line'
plotOverLine1 = PlotOverLine(registrationName='PlotOverLine1', Input=wavelet1)

# show data in view
plotOverLine1Display = Show(plotOverLine1, renderView1, 'GeometryRepresentation')

# trace defaults for the display properties.
plotOverLine1Display.Representation = 'Surface'

# Create a new 'Line Chart View'
lineChartView1 = CreateView('XYChartView')

# show data in view
plotOverLine1Display_1 = Show(plotOverLine1, lineChartView1, 'XYChartRepresentation')

# get layout
layout1 = GetLayoutByName("Layout #1")

# add view to a layout so it's visible in UI
AssignViewToLayout(view=lineChartView1, layout=layout1, hint=0)

# Properties modified on plotOverLine1Display_1
plotOverLine1Display_1.SeriesOpacity = ['arc_length', '1', 'RTData', '1', 'vtkValidPointMask', '1', 'Points_X', '1', 'Points_Y', '1', 'Points_Z', '1', 'Points_Magnitude', '1']
plotOverLine1Display_1.SeriesPlotCorner = ['Points_Magnitude', '0', 'Points_X', '0', 'Points_Y', '0', 'Points_Z', '0', 'RTData', '0', 'arc_length', '0', 'vtkValidPointMask', '0']
plotOverLine1Display_1.SeriesLineStyle = ['Points_Magnitude', '1', 'Points_X', '1', 'Points_Y', '1', 'Points_Z', '1', 'RTData', '1', 'arc_length', '1', 'vtkValidPointMask', '1']
plotOverLine1Display_1.SeriesLineThickness = ['Points_Magnitude', '2', 'Points_X', '2', 'Points_Y', '2', 'Points_Z', '2', 'RTData', '2', 'arc_length', '2', 'vtkValidPointMask', '2']
plotOverLine1Display_1.SeriesMarkerStyle = ['Points_Magnitude', '0', 'Points_X', '0', 'Points_Y', '0', 'Points_Z', '0', 'RTData', '0', 'arc_length', '0', 'vtkValidPointMask', '0']
plotOverLine1Display_1.SeriesMarkerSize = ['Points_Magnitude', '4', 'Points_X', '4', 'Points_Y', '4', 'Points_Z', '4', 'RTData', '4', 'arc_length', '4', 'vtkValidPointMask', '4']

# Properties modified on plotOverLine1Display_1
plotOverLine1Display_1.SeriesVisibility = ['arc_length', 'RTData']

# Properties modified on plotOverLine1Display_1
plotOverLine1Display_1.SeriesVisibility = ['arc_length', 'Points_Z', 'RTData']

# Properties modified on plotOverLine1Display_1
plotOverLine1Display_1.SeriesOpacity = ['arc_length', '0.3', 'RTData', '0.3', 'vtkValidPointMask', '0.3', 'Points_X', '0.3', 'Points_Y', '0.3', 'Points_Z', '0.3', 'Points_Magnitude', '0.3']

# Properties modified on plotOverLine1Display_1
plotOverLine1Display_1.SeriesOpacity = ['arc_length', '1', 'RTData', '0.3', 'vtkValidPointMask', '0.3', 'Points_X', '0.3', 'Points_Y', '0.3', 'Points_Z', '0.3', 'Points_Magnitude', '0.3']

#================================================================
# Save the baseline and do the regression test
#================================================================

from os.path import join
from paraview.vtk.vtkTestingRendering import vtkTesting
import sys

testing = vtkTesting()
for arg in sys.argv:
    testing.AddArgument(arg)
for x in range(1, len(sys.argv)):
    if sys.argv[x] == "-B" and len(sys.argv) > (x+1):
        testing.AddArgument("-V")
        testing.AddArgument(join(sys.argv[x+1], "TestXYChartOpacity.png"))

testimage = join(testing.GetTempDirectory(), "TestXYChartOpacity.png")
SaveScreenshot(testimage, view=lineChartView1)

result = testing.RegressionTest(testimage, 10)
if result == testing.DO_INTERACTOR:
    sys.exit(0)
elif result == testing.NOT_RUN:
    sys.exit(125)
elif result == testing.FAILED:
    raise RuntimeError("test failed!")
