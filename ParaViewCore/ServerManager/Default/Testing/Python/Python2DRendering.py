import os
import os.path
import sys

from paraview import servermanager
from paraview import simple

from paraview import smtesting

smtesting.ProcessCommandLineArguments()

# Make sure the test driver know that process has properly started
print "Process started"
errors = 0

#-------------------- Start testing --------------------------

print "Start TestPython2DRendering testing"

# Create a 3D renderer
view = simple.GetRenderView()
view.Background = [0, 0, 0]
view.CenterAxesVisibility = 0

legend = simple.Text(Text='Text annotation of 3D view')
displayProperties = simple.GetDisplayProperties(legend)
displayProperties.Bold = 0
displayProperties.FontFamily = 'Arial'
displayProperties.FontSize = 9
displayProperties.Color = [1.0,1.0,1.0]
simple.Show(legend)


# Create preset lookup tables
lutCR = simple.CreateLookupTable( ColorSpace='RGB', \
                                  NumberOfTableValues=256, \
                                  RGBPoints=[0,0.6,0.88,1,100,1,0,0])
lutWGR = simple.CreateLookupTable( ColorSpace='RGB', \
                                   NumberOfTableValues=256, \
                                   RGBPoints=[0,0.9,0.9,0.9,\
                                              25,0.55,0.75,0.5,50,0.1,0.58,0.2,\
                                              75,0.54,0.31,0.17,100,0.78,0.2,0.15])
lutYGB = simple.CreateLookupTable( ColorSpace='RGB', \
                                   NumberOfTableValues=256, \
                                   RGBPoints=[0,1,1,0.8,25,0.63,0.86,0.71,50,0.26,0.71,0.77,75,0.17,0.5,0.72,100,0.15,0.2,0.58])

scalarbar = simple.CreateScalarBar( Title='', LabelFontSize=8, Enabled=0, TitleFontSize=10, NumberOfLabels=2, LookupTable=lutCR, Position=[0,0.1], Position2=[0.2, 0.9], Visibility=0, Selectable=1, AutomaticLabelFormat=0, LabelFormat='%-#6.3g')

view.Representations.append(scalarbar)

# Update camera manipulator
manipulatorsProperty = view.GetProperty('Camera3DManipulators');
newList = [];
for index in xrange(manipulatorsProperty.GetNumberOfProxies()):
   manipulator = servermanager._getPyProxy(manipulatorsProperty.GetProxy(index))
   if manipulator.ManipulatorName != 'Rotation':
      if manipulator.ManipulatorName == 'Pan2':
         manipulator.Shift = 0
      newList.append(manipulator)

manipulatorsProperty.RemoveAllProxies()
for manipulator in newList:
   manipulatorsProperty.AddProxy(manipulator.SMProxy)


view.CameraParallelProjection = 1
view.CameraParallelScale = 2000000
simple.Render()
scalarbar.Visibility = True
legend.Text = "123"
simple.Render()

if not smtesting.DoRegressionTesting(view.SMProxy):
  raise smtesting.TestError, 'Test failed.'

simple.Disconnect()
