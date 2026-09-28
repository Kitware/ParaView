# Tests the "SwapXYAxes" option of line and point chart views, which plots the
# X array (or the index), i.e. the independent variable, along the vertical axis
# and all the series along the horizontal axis.

from paraview.simple import *
from paraview import smtesting
from paraview.modules.vtkRemotingViews import vtkPVPlotTime
from paraview.vtk.vtkChartsCore import vtkAxis

import os.path

smtesting.ProcessCommandLineArguments()

# independent variable 'depth' in [0, 100] (51 rows), and two dependent
# variables in [10, 20] and [30, 40].
fname = os.path.join(smtesting.TempDir, "ChartSwapXYAxes.csv")
with open(fname, "w") as f:
    f.write("depth,temperature,salinity\n")
    for i in range(51):
        f.write("%g,%g,%g\n" % (2 * i, 20 - i / 5.0, 30 + i / 5.0))
reader = CSVReader(FileName=[fname])

# same, using an independent variable named 'Time'.
timeFname = os.path.join(smtesting.TempDir, "ChartSwapXYAxesTime.csv")
with open(timeFname, "w") as f:
    f.write("Time,temperature\n")
    for i in range(51):
        f.write("%g,%g\n" % (2 * i, 20 - i / 5.0))
timeReader = CSVReader(FileName=[timeFname])


def get_chart(view):
    Render(view)
    return view.GetClientSideObject().GetChart()


def check_ranges(view, left, bottom):
    chart = get_chart(view)
    for name, index, expected in (("left", vtkAxis.LEFT, left), ("bottom", vtkAxis.BOTTOM, bottom)):
        axis = chart.GetAxis(index)
        actual = (axis.GetUnscaledMinimum(), axis.GetUnscaledMaximum())
        if abs(actual[0] - expected[0]) > 1e-6 or abs(actual[1] - expected[1]) > 1e-6:
            raise RuntimeError("Unexpected %s axis range %s for %s (expected %s)" %
                               (name, actual, view.GetXMLName(), expected))


def get_time_axis_mode(view):
    chart = get_chart(view)
    for i in range(chart.GetNumberOfPlots()):
        plot = chart.GetPlot(i)
        if plot.IsA("vtkPVPlotTime"):
            return plot.GetTimeAxisMode()
    raise RuntimeError("Missing time marker.")


for viewType in ["XYChartView", "XYPointChartView"]:
    view = CreateView(viewType)
    view.ViewSize = [400, 300]
    display = Show(reader, view)
    display.UseIndexForXAxis = 0
    display.XArrayName = "depth"
    display.SeriesVisibility = ["temperature", "salinity"]

    check_ranges(view, left=(10, 40), bottom=(0, 100))

    view.SwapXYAxes = 1
    check_ranges(view, left=(0, 100), bottom=(10, 40))

    # the index is plotted along the vertical axis too.
    display.UseIndexForXAxis = 1
    check_ranges(view, left=(0, 50), bottom=(10, 40))

    view.SwapXYAxes = 0
    check_ranges(view, left=(10, 40), bottom=(0, 50))

    # the time marker is along the axis showing the X array named "Time".
    timeDisplay = Show(timeReader, view)
    timeDisplay.UseIndexForXAxis = 0
    timeDisplay.XArrayName = "Time"
    timeDisplay.SeriesVisibility = ["temperature"]
    Hide(reader, view)
    view.ViewTime = 10
    if get_time_axis_mode(view) != vtkPVPlotTime.X_AXIS:
        raise RuntimeError("Time marker expected along the horizontal axis.")
    view.SwapXYAxes = 1
    if get_time_axis_mode(view) != vtkPVPlotTime.Y_AXIS:
        raise RuntimeError("Time marker expected along the vertical axis.")

    Delete(view)
    del view

# only line and point charts support swapping axes.
barChartView = CreateView("XYBarChartView")
if "SwapXYAxes" in barChartView.ListProperties():
    raise RuntimeError("Bar chart views must not support 'SwapXYAxes'.")
