# Test the CSV writer and reader.

from paraview import smtesting
import os
import os.path
import sys

import paraview
paraview.compatibility.major = 3
paraview.compatibility.minor = 4
from paraview import servermanager

smtesting.ProcessCommandLineArguments()

servermanager.Connect()

sourceProxy = servermanager.sources.RTAnalyticSource()

filterProxy = servermanager.filters.ExtractHistogram(Input=sourceProxy)
filterProxy.UpdatePipeline()

temp_filename = os.path.join(smtesting.TempDir, "histogram.csv")

writerProxy = servermanager.writers.CSVWriter(Input=filterProxy, FileName=temp_filename)
writerProxy.UpdatePipeline()

readerProxy = servermanager.sources.CSVReader(FileName = temp_filename)
readerProxy.UpdatePipeline()

dataInfo = readerProxy.GetDataInformation()
numRows = dataInfo.GetNumberOfRows()
if numRows != 10:
  print("ERROR: Wrong number of rows reported %d" % numRows)
  sys.exit(1);

rowInfo = dataInfo.GetRowDataInformation()
numColumns = rowInfo.GetNumberOfArrays()
if numColumns != 2:
  print("ERROR: Wrong number of columns.")
  sys.exit(1);

rowArrayInfo = rowInfo.GetArrayInformation(0)
# actual value is 49.3269
if rowArrayInfo.GetComponentRange(0)[0] < 49.32 or\
     rowArrayInfo.GetComponentRange(0)[0] > 49.33:
     print("ERROR: Wrong bin data value. %f" % rowArrayInfo.GetComponentRange(0)[0])

# actual value is 264.855
if rowArrayInfo.GetComponentRange(0)[1] < 264.85 or\
     rowArrayInfo.GetComponentRange(0)[1] > 264.86:
     print("ERROR: Wrong bin data value. %f" % rowArrayInfo.GetComponentRange(0)[1])


rowArrayInfo = rowInfo.GetArrayInformation(1)
cellArrayName = rowArrayInfo.GetName()
if cellArrayName != "bin_values":
  print("ERROR: Wrong cell array name.")
  sys.exit(1);

cellNumComp = rowArrayInfo.GetNumberOfComponents()
if cellNumComp != 1:
  print("ERROR: Wrong number of array components.")
  sys.exit(1);

cellArrayRange = rowArrayInfo.GetComponentRange(0)
if cellArrayRange[0] != 72:
  print("ERROR: Wrong minimum array range.")
  sys.exit(1);

if cellArrayRange[1] != 1938:
  print("ERROR: Wrong maximum array range.")
  sys.exit(1);

try:
  os.remove(temp_filename)
except:
  pass
