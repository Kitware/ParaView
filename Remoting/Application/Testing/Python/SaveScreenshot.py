from __future__ import print_function

# Test to save screenshot from various views
from paraview.simple import *

from paraview import smtesting
smtesting.ProcessCommandLineArguments()

tempdir = smtesting.GetUniqueTempDirectory("SaveScreenshot-")
print("Generating output files in `%s`" % tempdir)

def RegressionTest(imageName):
    if servermanager.vtkProcessModule.GetProcessModule().GetPartitionId() > 0:
        # only do image comparison on root node; this avoids test failures
        # due to satellites attempting to read generated images even before the root
        # has written them out.
        return True
    from paraview.vtk.vtkTestingRendering import vtkTesting
    testing = vtkTesting()
    testing.AddArgument("-T")
    testing.AddArgument(tempdir)
    testing.AddArgument("-V")
    testing.AddArgument(smtesting.DataDir + "/Remoting/Application/Testing/Data/Baseline/" + imageName)
    return testing.RegressionTest(tempdir + "/" + imageName, 10) == vtkTesting.PASSED

renderView1 = CreateView('RenderView')
renderView1.ViewSize = [200, 200]

AssignViewToLayout(renderView1)

# get layout
layout1 = GetLayout()

# split cell
layout1.SplitHorizontal(0, 0.5)

# set active view
SetActiveView(None)

# Create a new 'Render View'
renderView2 = CreateView('RenderView')
renderView2.ViewSize = [200, 200]
renderView2.AxesGrid = 'GridAxes3DActor'
renderView2.StereoType = 0
renderView2.Background = [0.32, 0.34, 0.43]

# place view in the layout
layout1.AssignView(2, renderView2)

# split cell
layout1.SplitVertical(2, 0.5)

# set active view
SetActiveView(None)

# Create a new 'Line Chart View'
lineChartView1 = CreateView('XYChartView')
lineChartView1.ViewSize = [200, 200]

# place view in the layout
layout1.AssignView(6, lineChartView1)

# set active view
SetActiveView(renderView1)

# split cell
layout1.SplitVertical(1, 0.5)

# set active view
SetActiveView(None)

# Create a new 'Bar Chart View'
barChartView1 = CreateView('XYBarChartView')
barChartView1.ViewSize = [200, 200]

# place view in the layout
layout1.AssignView(4, barChartView1)

# set active view
SetActiveView(renderView1)
Wavelet()
r = Show()
r.Representation = "Outline"
Render()
SaveScreenshot(tempdir + "/SaveScreenshotOutline.png", magnification=2)

SetActiveView(renderView2)
r = GetDisplayProperties()
r.Representation = "Surface"
Show()
Render()
ResetCamera()
SaveScreenshot(tempdir + "/SaveScreenshotSurface.png", magnification=2)

SetActiveView(lineChartView1)
p = PlotOverLine(Source = "Line")
p.Source.Point1 = [-10.0, -10.0, -10.0]
p.Source.Point2 = [10.0, 10.0, 10.0]
p.Source.Resolution = 10
Show()
Render()
SaveScreenshot(tempdir + "/SaveScreenshotLinePlot.png", magnification=2)

SetActiveView(barChartView1)
Show()
Render()
SaveScreenshot(tempdir + "/SaveScreenshotBarPlot.png", magnification=2)

val1 = RegressionTest("SaveScreenshotOutline.png")
val2 = RegressionTest("SaveScreenshotSurface.png")
val3 = RegressionTest("SaveScreenshotLinePlot.png")
val4 = RegressionTest("SaveScreenshotBarPlot.png")

if not (val1 and val2 and val3 and val4):
    raise RuntimeError("Test Failed")
