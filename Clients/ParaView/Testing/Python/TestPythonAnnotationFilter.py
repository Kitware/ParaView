from paraview.simple import *
from paraview import servermanager

import time

# Make sure the test driver know that process has properly started
print ("Process started")
errors = 0

#-------------------- Comparison helper ----------------------

def equal(a, b):
   if a == b:
      return True
   aList = a.replace(","," ").replace("["," ").replace("]", " ").split(" ")
   bList = b.replace(","," ").replace("["," ").replace("]", " ").split(" ")
   size = len(aList)
   if size != len(bList):
      return False
   for i in xrange(size):
      if len(aList[i]) > 0:
         af = float(aList[i])
         bf = float(bList[i])
         if ((af-bf)*(af-bf)) > 0.000001:
           return False
   return True

#-------------------- Start testing --------------------------

print ("Start PythonAnnotationFilter testing")

# now try with the can.ex2 exodus file for multiblock testing
from paraview.vtk.util.misc import vtkGetDataRoot
import os.path
dataToLoad = os.path.join(vtkGetDataRoot(), 'Testing/Data/can.ex2')

# Load data file
reader = OpenDataFile(dataToLoad)
reader.GlobalVariables = ['KE', 'XMOM', 'YMOM', 'ZMOM', 'NSTEPS', 'TMSTEP']
reader.UpdatePipeline()

# Time management
controller = servermanager.ParaViewPipelineController()
timekeeper = controller.FindTimeKeeper(servermanager.ActiveConnection.Session)
timesteps = timekeeper.TimestepValues
time = timesteps[5]

# Merge blocks
merge = MergeBlocks()

# Add Python calculator
# Using this expression tests that the time variables get defined by the
# calculator, as expected.
calculator = PythonCalculator(Expression="t_index*t_value*time_index*time_value")

# Annotation filter
annotation = PythonAnnotation()
annotation.Expression = '"%f %f %f" % (XMOM[t_index], YMOM[t_index], ZMOM[t_index])'

# Update time and trigger pipeline execution
time = timesteps[5]
annotation.UpdatePipeline(time)

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "0.012132 0.001378 -1158.252808"

if not equal(value, expected):
  errors += 1
  print ("Error: Expected ", expected, " and got ", value)

# Update time and trigger pipeline execution
time = timesteps[7]
annotation.UpdatePipeline(time)

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "0.013970 0.001319 -1141.020020"

if not equal(value, expected):
  errors += 1
  print ("Error: Expected ", expected, " and got ", value)

# Check time infos
annotation.Expression = '"%i %f %s" % (t_index, t_value, str(t_range))'

# Update time and trigger pipeline execution
time = timesteps[7]
annotation.UpdatePipeline(time)

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "7 0.000700 (0.0, 0.004299988504499197)"

if not equal(value, expected):
  errors += 1
  print ("Error: Expected ", expected, " and got ", value)

# Update time and trigger pipeline execution
time = timesteps[27]
annotation.UpdatePipeline(time)

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "27 0.002700 (0.0, 0.004299988504499197)"

if not equal(value, expected):
  errors += 1
  print ("Error: Expected ", expected, " and got ", value)

# Update time and trigger pipeline execution
time = timesteps[len(timesteps)-1]
annotation.UpdatePipeline(time)

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "43 0.004300 (0.0, 0.004299988504499197)"

if not equal(value, expected):
  errors += 1
  print ("Error: Expected ", expected, " and got ", value)

# Disconnect and quit application...
Disconnect()

if errors > 0:
  raise RuntimeError ("An error occurred during the execution")
