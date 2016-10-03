from paraview.simple import *

wavelet = Wavelet()
UpdatePipeline()

p = PlotOverLine(Input=wavelet, Source='High Resolution Line Source')
r = Show(p)
r.SeriesLabelPrefix = "plot1_"

p2 = PlotOverLine(Input=wavelet, Source='High Resolution Line Source')
p2.Source.Point1 = [0.0, -10.0, 0.0]
p2.Source.Point2 = [0.0, 10.0, 0.0]
r2 =Show(p2)
r2.SeriesLabelPrefix = "plot2_"

Render()

# compare with baseline image
import os
import sys
try:
  baselineIndex = sys.argv.index('-B')+1
  baselinePath = sys.argv[baselineIndex]
except:
  print ("Could not get baseline directory. Test failed.")
  exit(1)
baseline_file = os.path.join(baselinePath, "TestLineChartLabelPrefix.png")
import vtk.test.Testing
vtk.test.Testing.VTK_TEMP_DIR = vtk.util.misc.vtkGetTempDir()
vtk.test.Testing.compareImage(GetActiveView().GetRenderWindow(), baseline_file,
                              threshold=25)
vtk.test.Testing.interact()
