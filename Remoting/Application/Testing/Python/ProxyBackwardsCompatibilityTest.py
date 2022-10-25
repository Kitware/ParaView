from paraview.simple import *
import paraview

sourceDs = Wavelet()

print("Setting compatibility version to 5.0...")
paraview.compatibility.major = 5
paraview.compatibility.minor = 0

print("Testing Plot Over Line Legacy")

# Create plot over line proxy
p3 = PlotOverLine(Input=sourceDs)

# Check that its XML name is the one of the legacy
assert p3.GetXMLName() == "ProbeLineLegacy"

# Do a display pipeline
lineChartView1 = GetActiveViewOrCreate('XYChartView')
plot1Display = Show(p3, lineChartView1, 'XYChartRepresentation')
lineChartView1.Update()
