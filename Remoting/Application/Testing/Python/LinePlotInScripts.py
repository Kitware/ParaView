# Test rendering on line plots in Python scripts.
# This also exercises changing of axis range based on data as script progresses
# -- an attempt to simulate use of such plots in Catalyst, when this script is
# run with pvbatch --sym.

import sys
if sys.version_info >= (3,):
    xrange = range

from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

w = Wavelet()
UpdatePipeline()
p = PlotOverLine()
p.Point1 = [-10.0, -10.0, -10.0]
p.Point2 = [10.0, 10.0, 10.0]
p.Resolution = 1000
p.SamplingPattern = 2 # UNIFORM_SAMPLING

d = Show()
d.SeriesVisibility = ['RTData']
Render()

for i in xrange(5):
    w.Maximum = 2**i
    SaveScreenshot(smtesting.TempDir + "/LinePlotInScripts_%d.png" % i, magnification=2)
print("Done")
