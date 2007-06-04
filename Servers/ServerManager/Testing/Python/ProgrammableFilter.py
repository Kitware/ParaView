import paraview
import sys
import math

import os
import os.path
import SMPythonTesting

SMPythonTesting.ProcessCommandLineArguments()

# Connect to the "builtin"
#(in-process) ParaView server ...
#=================================
paraview.ActiveConnection = paraview.Connect()

# Create an Exodus reader to load our data ...
#==============================================
exodus_file = os.path.join(SMPythonTesting.DataDir, "Data/disk_out_ref.ex2")
reader = paraview.CreateProxy("sources", "ExodusReader")
reader.GetProperty("FileName").SetElement(0, exodus_file)
reader.UpdateVTKObjects()
reader.UpdatePipeline()
reader.UpdatePropertyInformation()
paraview.RegisterProxy(reader, "my reader")

# Create our programmable filter and set its program ...
#========================================================
filter = paraview.CreateProxy("filters", "Programmable Filter")
filter.GetProperty("Script").SetElement(0, """
input = self.GetUnstructuredGridInput()

output = self.GetUnstructuredGridOutput()
output.DeepCopy(input)

""")

# Connect the reader output to
# the programmable filter input ...
#===================================
filter.GetProperty("Input").AddProxy(reader.SMProxy, 0)
filter.UpdateVTKObjects()
paraview.RegisterProxy(filter, "my programmable filter")

# Perform a sum operation
#=========================
sum = paraview.CreateProxy("filters", "MinMax")
sum.SetOperation("SUM")
sum.UpdateVTKObjects()

# Reduce the programmable filter output
# data using our "max" algorithm,
# returning just the maximum error value
# (instead of transferring the entire
# dataset to the client)
#=======================================
myoutput = paraview.Fetch(filter, sum)

cellData = myoutput.GetCellData()
if cellData.GetArray(0).GetValue(0) != 7472:
  print "ERROR: Wrong value returned from cell %s array." % cellData.GetArray(0).GetName()
  sys.exit(1)

if cellData.GetArray(1).GetValue(0) != 27919128:
  print "ERROR: Wrong value returned from cell %s array." % cellData.GetArray(1).GetName()
  sys.exit(1)

if cellData.GetArray(2).GetValue(0) != 27919128:
  print "ERROR: Wrong value returned from cell %s array." % cellData.GetArray(2).GetName()
  sys.exit(1)

pointData = myoutput.GetPointData()
if pointData.GetArray(0).GetValue(0) != 36120750:
  print "ERROR: Wrong value returned from point %s array." % pointData.GetArray(0).GetName()
  sys.exit(1)

if pointData.GetArray(1).GetValue(0) != 36120750:
  print "ERROR: Wrong value returned from point %s array." % pointData.GetArray(1).GetName()
  sys.exit(1)

#---------------Now repeat--------------

# Create an Exodus reader to load our data ...
#==============================================
reader = paraview.CreateProxy("sources", "ExodusReader")
reader.GetProperty("FileName").SetElement(0, exodus_file)
reader.UpdateVTKObjects()
reader.UpdatePipeline()
reader.UpdatePropertyInformation()
paraview.RegisterProxy(reader, "my reader")

# Create our programmable filter and set its program ...
#========================================================
filter = paraview.CreateProxy("filters", "Programmable Filter")
filter.GetProperty("Script").SetElement(0, """

input = self.GetUnstructuredGridInput()

output = self.GetUnstructuredGridOutput()
output.DeepCopy(input)

""")

# Connect the reader output to
# the programmable filter input ...
#===================================
filter.GetProperty("Input").AddProxy(reader.SMProxy, 0)
filter.UpdateVTKObjects()
paraview.RegisterProxy(filter, "my programmable filter")

# Perform a sum operation
#=========================
sum = paraview.CreateProxy("filters", "MinMax")
sum.SetOperation("SUM")
sum.UpdateVTKObjects()

# Reduce the programmable filter output
# data using our "max" algorithm,
# returning just the maximum error value
# (instead of transferring the entire
# dataset to the client)
#=======================================
myoutput = paraview.Fetch(filter, sum)
cellData = myoutput.GetCellData()

cellData = myoutput.GetCellData()
if cellData.GetArray(0).GetValue(0) != 7472:
  print "ERROR: Wrong value returned from cell %s array." % cellData.GetArray(0).GetName()
  sys.exit(1)

if cellData.GetArray(1).GetValue(0) != 27919128:
  print "ERROR: Wrong value returned from cell %s array." % cellData.GetArray(1).GetName()
  sys.exit(1)

if cellData.GetArray(2).GetValue(0) != 27919128:
  print "ERROR: Wrong value returned from cell %s array." % cellData.GetArray(2).GetName()
  sys.exit(1)

pointData = myoutput.GetPointData()
if pointData.GetArray(0).GetValue(0) != 36120750:
  print "ERROR: Wrong value returned from point %s array." % pointData.GetArray(0).GetName()
  sys.exit(1)

if pointData.GetArray(1).GetValue(0) != 36120750:
  print "ERROR: Wrong value returned from point %s array." % pointData.GetArray(1).GetName()
  sys.exit(1)
