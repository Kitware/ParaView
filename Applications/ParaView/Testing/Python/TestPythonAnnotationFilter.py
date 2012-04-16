from paraview.simple import *
from paraview import servermanager

import time

# Make sure the test driver know that process has properly started
print "Process started"
errors = 0

#-------------------- Start testing --------------------------

print "Start PythonAnnotationFilter testing"

options = servermanager.vtkProcessModule.GetProcessModule().GetOptions()
dataToLoad = options.GetParaViewDataName()


# Load data file
reader = OpenDataFile(dataToLoad)
reader.GlobalVariables = ['KE', 'XMOM', 'YMOM', 'ZMOM', 'NSTEPS', 'TMSTEP']
reader.UpdatePipeline()

# Time management
#animationscene = GetAnimationScene()
tk = servermanager.ProxyManager().GetProxy('timekeeper','TimeKeeper')
#animationscene.PlayMode = 'Snap To TimeSteps'
#animationscene.AnimationTime = tk.TimestepValues[5]
tk.Time = tk.TimestepValues[5]

# Merge blocks
merge = MergeBlocks()

# Annotation filter
annotation = PythonAnnotation()
annotation.Expression = '"%f %f %f" % (XMOM[t_index], YMOM[t_index], ZMOM[t_index])'
annotation.UpdatePipeline()
Show()

# Update time and trigger pipeline execution
tk.Time = tk.TimestepValues[5]
Render()

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "0.012132 0.001378 -1158.252808"

if ( value != expected):
  errors += 1
  print "Error Invalid active connection. Expected ", expected, " and got ", value

# Update time and trigger pipeline execution
tk.Time = tk.TimestepValues[7]
Render()

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "0.013970 0.001319 -1141.020020"

if ( value != expected):
  errors += 1
  print "Error Invalid active connection. Expected ", expected, " and got ", value

# Check time infos
annotation.Expression = '"%i %f %s" % (t_index, t_value, str(t_range))'
annotation.UpdatePipeline()

# Update time and trigger pipeline execution
tk.Time = tk.TimestepValues[7]
Render()

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "7 0.000700 [0, 0.00429999]"

if ( value != expected):
  errors += 1
  print "Error Invalid active connection. Expected ", expected, " and got ", value

# Update time and trigger pipeline execution
tk.Time = tk.TimestepValues[27]
Render()

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "27 0.002700 [0, 0.00429999]"

if ( value != expected):
  errors += 1
  print "Error Invalid active connection. Expected ", expected, " and got ", value

# Update time and trigger pipeline execution
tk.Time = tk.TimestepValues[len(tk.TimestepValues)-1]
Render()

annotation.SMProxy.UpdatePropertyInformation()
value = annotation.SMProxy.GetProperty('AnnotationValue').GetElement(0)
expected = "43 0.004300 [0, 0.00429999]"

if ( value != expected):
  errors += 1
  print "Error Invalid active connection. Expected ", expected, " and got ", value

# Disconnect and quit application...
Disconnect()

if errors > 0:
  raise RuntimeError, "An error occured during the execution"
