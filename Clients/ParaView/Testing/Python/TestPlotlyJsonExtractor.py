# Check the PlotlyJsonExtractor for a simple line plot against a known state

import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 11

#### import the simple module from the paraview
from paraview.simple import *
from paraview import smtesting

#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# ----------------------------------------------------------------
# setup views used in the visualization
# ----------------------------------------------------------------

# Create a new 'Line Chart View'
lineChartView1 = CreateView('XYChartView')
lineChartView1.ViewSize = [488, 664]
lineChartView1.ChartTitle = 'Chart Title'
lineChartView1.ChartTitleColor = [0.6666666666666666, 0.0, 0.0]
lineChartView1.LeftAxisTitle = 'Y Axis '
lineChartView1.LeftAxisTitleColor = [1.0, 0.0, 0.0]
lineChartView1.LeftAxisRangeMinimum = 40.0
lineChartView1.LeftAxisRangeMaximum = 280.0
lineChartView1.BottomAxisTitle = 'X axis'
lineChartView1.BottomAxisTitleFontFamily = 'Courier'
lineChartView1.BottomAxisTitleColor = [1.0, 0.0, 0.0]
lineChartView1.BottomAxisRangeMaximum = 35.0
lineChartView1.RightAxisRangeMaximum = 6.66
lineChartView1.TopAxisRangeMaximum = 6.66

# ----------------------------------------------------------------
# setup the data processing pipelines
# ----------------------------------------------------------------

# create a new 'Wavelet'
wavelet1 = Wavelet(registrationName='Wavelet1')

# create a new 'Plot Over Line'
plotOverLine1 = PlotOverLine(registrationName='PlotOverLine1', Input=wavelet1)
plotOverLine1.Point1 = [-10.0, -10.0, -10.0]
plotOverLine1.Point2 = [10.0, 10.0, 10.0]

# ----------------------------------------------------------------
# setup the visualization in view 'lineChartView1'
# ----------------------------------------------------------------

# show data from plotOverLine1
plotOverLine1Display = Show(plotOverLine1, lineChartView1, 'XYChartRepresentation')

# trace defaults for the display properties.
plotOverLine1Display.UseIndexForXAxis = 0
plotOverLine1Display.XArrayName = 'arc_length'
plotOverLine1Display.SeriesVisibility = ['RTData']
plotOverLine1Display.SeriesLabel = ['arc_length', 'arc_length', 'RTData', 'Wavelet data', 'vtkValidPointMask', 'vtkValidPointMask', 'Points_X', 'Points_X', 'Points_Y', 'Points_Y', 'Points_Z', 'Points_Z', 'Points_Magnitude', 'Points_Magnitude']
plotOverLine1Display.SeriesColor = ['arc_length', '0', '0', '0', 'RTData', '0.8899977111467154', '0.10000762951094835', '0.1100022888532845', 'vtkValidPointMask', '0.220004577706569', '0.4899977111467155', '0.7199969481956207', 'Points_X', '0.30000762951094834', '0.6899977111467155', '0.2899977111467155', 'Points_Y', '0.6', '0.3100022888532845', '0.6399938963912413', 'Points_Z', '1', '0.5000076295109483', '0', 'Points_Magnitude', '0.6500038147554742', '0.3400015259021897', '0.16000610360875867']
plotOverLine1Display.SeriesOpacity = ['arc_length', '1', 'RTData', '1', 'vtkValidPointMask', '1', 'Points_X', '1', 'Points_Y', '1', 'Points_Z', '1', 'Points_Magnitude', '1']
plotOverLine1Display.SeriesPlotCorner = ['Points_Magnitude', '0', 'Points_X', '0', 'Points_Y', '0', 'Points_Z', '0', 'RTData', '0', 'arc_length', '0', 'vtkValidPointMask', '0']
plotOverLine1Display.SeriesLabelPrefix = ''
plotOverLine1Display.SeriesLineStyle = ['Points_Magnitude', '1', 'Points_X', '1', 'Points_Y', '1', 'Points_Z', '1', 'RTData', '2', 'arc_length', '1', 'vtkValidPointMask', '1']
plotOverLine1Display.SeriesLineThickness = ['Points_Magnitude', '2', 'Points_X', '2', 'Points_Y', '2', 'Points_Z', '2', 'RTData', '2', 'arc_length', '2', 'vtkValidPointMask', '2']
plotOverLine1Display.SeriesMarkerStyle = ['Points_Magnitude', '0', 'Points_X', '0', 'Points_Y', '0', 'Points_Z', '0', 'RTData', '1', 'arc_length', '0', 'vtkValidPointMask', '0']
plotOverLine1Display.SeriesMarkerSize = ['Points_Magnitude', '4', 'Points_X', '4', 'Points_Y', '4', 'Points_Z', '4', 'RTData', '4', 'arc_length', '4', 'vtkValidPointMask', '4']

# ----------------------------------------------------------------
# setup extractors
# ----------------------------------------------------------------

# create extractor
plotlyJson1 = CreateExtractor('PlotlyJson', lineChartView1, registrationName='Plotly Json1')
# trace defaults for the extractor.
plotlyJson1.Trigger = 'Time Step'

# init the 'Plotly Json' selected for 'Writer'
plotlyJson1.Writer.FileName = 'LineChartView1.json'

##--------------------------------------------
## Save all "Extractors" from the pipeline browser
output_directory = "./extracts"
SaveExtracts(ExtractsOutputDirectory=output_directory)



# compare generated file with baseline
from pathlib import Path
import json

filename = Path(output_directory) / plotlyJson1.Writer.FileName
generated = None
with open(filename,"r") as file:
  generated = json.loads(file.read())

smtesting.ProcessCommandLineArguments()

filename = Path(smtesting.DataDir) / "Testing/Data" / "plotly_json_line_chart.json"
expected = None
with open(filename,"r") as file:
  expected = json.loads(file.read())

assert expected == generated, "Expected and generated json Files differ !"
