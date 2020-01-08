from paraview import servermanager
import sys
import math

import os
import os.path
from paraview import smtesting

smtesting.ProcessCommandLineArguments()

# Connect to the "builtin"
#(in-process) ParaView server ...
#=================================
servermanager.Connect()

# Create an Exodus reader to load our data ...
#==============================================
exodus_file = os.path.join(smtesting.DataDir, "Testing/Data/disk_out_ref.ex2")
reader = servermanager.sources.ExodusIIReader(FileName=exodus_file)
reader.UpdatePipeline()
reader.UpdatePropertyInformation()
pxm = servermanager.ProxyManager()
pxm.RegisterProxy("sources",  "my reader", reader)

# Create our programmable filter and set its program ...
#========================================================
filter = servermanager.filters.ProgrammableFilter()
filter.GetProperty("Script").SetElement(0, """
input = self.GetInputDataObject(0, 0)

output = self.GetOutputDataObject(0)
output.DeepCopy(input)

""")

# Connect the reader output to
# the programmable filter input ...
#===================================
filter.Input = reader
pxm.RegisterProxy("sources", "my programmable filter", filter)

# Perform a sum operation
#=========================
sum = servermanager.filters.MinMax(Operation="SUM")

# Reduce the programmable filter output
# data using our "max" algorithm,
# returning just the maximum error value
# (instead of transferring the entire
# dataset to the client)
#=======================================
myoutput = servermanager.Fetch(filter, sum, sum)

cellData = myoutput.GetCellData()
if cellData.GetArray("ObjectId").GetValue(0) != 7472:
  print("ERROR: Wrong value returned from cell %s array." % cellData.GetArray(0).GetName())
  sys.exit(1)

if cellData.GetArray("GlobalElementId").GetValue(0) != 27919128:
  print("ERROR: Wrong value returned from cell %s array." % cellData.GetArray(1).GetName())
  sys.exit(1)

if cellData.GetArray("PedigreeElementId").GetValue(0) != 27919128:
  print("ERROR: Wrong value returned from cell %s array." % cellData.GetArray(2).GetName())
  sys.exit(1)

pointData = myoutput.GetPointData()
if pointData.GetArray("GlobalNodeId").GetValue(0) != 36120750:
  print("ERROR: Wrong value returned from point %s array." % pointData.GetArray(0).GetName())
  sys.exit(1)

if pointData.GetArray("PedigreeNodeId").GetValue(0) != 36120750:
  print("ERROR: Wrong value returned from point %s array." % pointData.GetArray(1).GetName())
  sys.exit(1)

#---------------Now repeat--------------

# Create an Exodus reader to load our data ...
#==============================================
exodus_file = os.path.join(smtesting.DataDir, "Testing/Data/disk_out_ref.ex2")
reader = servermanager.sources.ExodusIIReader(FileName=exodus_file)
reader.UpdatePipeline()
reader.UpdatePropertyInformation()
pxm = servermanager.ProxyManager()
pxm.RegisterProxy("sources",  "my reader", reader)

# Create our programmable filter and set its program ...
#========================================================
filter = servermanager.filters.ProgrammableFilter()
filter.GetProperty("Script").SetElement(0, """
input = self.GetInputDataObject(0, 0)

output = self.GetOutputDataObject(0)
output.DeepCopy(input)

""")

# Connect the reader output to
# the programmable filter input ...
#===================================
filter.Input = reader
pxm.RegisterProxy("sources", "my programmable filter", filter)

# Perform a sum operation
#=========================
sum = servermanager.filters.MinMax(Operation="SUM")

# Reduce the programmable filter output
# data using our "max" algorithm,
# returning just the maximum error value
# (instead of transferring the entire
# dataset to the client)
#=======================================
myoutput = servermanager.Fetch(filter, sum, sum)

cellData = myoutput.GetCellData()
if cellData.GetArray("ObjectId").GetValue(0) != 7472:
  print("ERROR: Wrong value returned from cell %s array." % cellData.GetArray(0).GetName())
  sys.exit(1)

if cellData.GetArray("GlobalElementId").GetValue(0) != 27919128:
  print("ERROR: Wrong value returned from cell %s array." % cellData.GetArray(1).GetName())
  sys.exit(1)

if cellData.GetArray("PedigreeElementId").GetValue(0) != 27919128:
  print("ERROR: Wrong value returned from cell %s array." % cellData.GetArray(2).GetName())
  sys.exit(1)

pointData = myoutput.GetPointData()
if pointData.GetArray("GlobalNodeId").GetValue(0) != 36120750:
  print("ERROR: Wrong value returned from point %s array." % pointData.GetArray(0).GetName())
  sys.exit(1)

if pointData.GetArray("PedigreeNodeId").GetValue(0) != 36120750:
  print("ERROR: Wrong value returned from point %s array." % pointData.GetArray(1).GetName())
  sys.exit(1)
