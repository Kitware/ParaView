# Test the CSV writer and reader.

import SMPythonTesting
import os
import os.path
import sys
import paraview

SMPythonTesting.ProcessCommandLineArguments()

paraview.ActiveConnection = paraview.Connect()

pxm = paraview.pyProxyManager()

sourceProxy = pxm.NewProxy("sources", "RTAnalyticSource")

filterProxy = pxm.NewProxy("filters", "ExtractHistogram")
filterProxy.SetInput(sourceProxy)
filterProxy.UpdateVTKObjects()
filterProxy.UpdatePipeline()

temp_filename = os.path.join(SMPythonTesting.TempDir, "histogram.csv")

writerProxy = pxm.NewProxy("writers", "CSVWriter")
writerProxy.SetInput(filterProxy)
writerProxy.SetFileName(temp_filename)
writerProxy.UpdateVTKObjects()
writerProxy.UpdatePipeline()

readerProxy = pxm.NewProxy("sources", "CSVReader")
readerProxy.SetFileName(temp_filename)
readerProxy.UpdateVTKObjects()

dataInfo = readerProxy.GetDataInformation()
numPts = dataInfo.GetNumberOfPoints()
if numPts != 11:
  print "ERROR: Wrong number of points reported."
  sys.exit(1);

numCells = dataInfo.GetNumberOfCells()
if numCells != 10:
  print "ERROR: Wrong number of cells reported."
  sys.exit(1);

bounds = dataInfo.GetBounds()
correctBounds = 0
# actual value is 37.3531
if bounds[0] > 37.35:
  if bounds[0] < 37.36:
    correctBounds = 1;

if correctBounds != 1:
  print "ERROR: Wrong minimum bounds in X."
  sys.exit(1);

correctBounds = 0
# actual value is 276.829
if bounds[1] > 276.8:
  if bounds[1] < 276.9:
    correctBounds = 1;

if correctBounds != 1:
  print "ERROR: Wrong maximum bounds in X."
  sys.exit(1);

extent = dataInfo.GetExtent()
if extent[0] != 0:
  print "ERROR: Wrong minimum extent in X."
  sys.exit(1);

if extent[1] != 10:
  print "ERROR: Wrong maximum extent in X."
  sys.exit(1);

cellInfo = dataInfo.GetCellDataInformation()
numCellArrays = cellInfo.GetNumberOfArrays()
if numCellArrays != 1:
  print "ERROR: Wrong number of cell arrays."
  sys.exit(1);

cellArrayInfo = cellInfo.GetArrayInformation(0)
cellArrayName = cellArrayInfo.GetName()
if cellArrayName != "bin_values":
  print "ERROR: Wrong cell array name."
  sys.exit(1);

cellNumComp = cellArrayInfo.GetNumberOfComponents()
if cellNumComp != 1:
  print "ERROR: Wrong number of array components."
  sys.exit(1);

cellArrayRange = cellArrayInfo.GetComponentRange(0)
if cellArrayRange[0] != 72:
  print "ERROR: Wrong minimum array range."
  sys.exit(1);

if cellArrayRange[1] != 1938:
  print "ERROR: Wrong maximum array range."
  sys.exit(1);

os.remove(temp_filename)
