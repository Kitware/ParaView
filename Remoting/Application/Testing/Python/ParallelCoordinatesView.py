from paraview.simple import *

from paraview import smtesting
smtesting.ProcessCommandLineArguments()

filename = smtesting.DataDir + '/Testing/Data/vehicle_data.csv'

vehicle_data_csv = OpenDataFile(filename)

ParallelCoordinatesChartView1 = CreateParallelCoordinatesChartView()
ParallelCoordinatesChartView1.ViewSize = [800, 800]

DataRepresentation2 = Show()
DataRepresentation2.CompositeDataSetIndex = 0
DataRepresentation2.FieldAssociation = 'Row Data'
DataRepresentation2.SeriesVisibility = ['Cylinders', '1', 'Displacement', '1', 'HP', '1', 'Weight', '1', 'Year', '1', 'Acceleration', '1', 'MPG', '1']
Render(ParallelCoordinatesChartView1)

# Change order.
#DataRepresentation2.SeriesVisibility = ['Displacement', '1', 'HP', '1', 'Weight', '1', 'Year', '1', 'Acceleration', '1', 'MPG', '1', 'Cylinders', '1' ]

#if not smtesting.DoRegressionTesting(ParallelCoordinatesChartView1.SMProxy):
#  print "Failed!"
#  # This will lead to VTK object leaks.
#  import sys
#  sys.exit(1)
