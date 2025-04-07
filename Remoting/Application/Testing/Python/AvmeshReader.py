# Test the vtkCellIntegrator

from paraview import smtesting
import os
import os.path
import sys
import paraview
paraview.compatibility.major = 5
paraview.compatibility.minor = 13
from paraview import servermanager
from paraview import util

smtesting.ProcessCommandLineArguments()

servermanager.Connect()

dataFile = os.path.join(smtesting.DataDir, "Testing/Data/vwing_hexle.avm")
reader = servermanager.sources.AvmeshReader(FileName=dataFile)
reader.UpdatePipeline()

readerOutput = servermanager.Fetch(reader)
if readerOutput.GetNumberOfPoints() != 19728:
    print("ERROR: incorrect number of points from reader. Number of cells is ",
          readerOutput.GetNumberOfPoints(), " but should be 19728")
    sys.exit(1)

if readerOutput.GetNumberOfCells() != 45233:
    print("ERROR: incorrect number of cells from reader. Number of cells is ",
          readerOutput.GetNumberOfCells(), " but should be 45233")
    sys.exit(1)

reader.SurfaceOnly = 1
reader.UpdatePipeline()

readerOutput = servermanager.Fetch(reader)
if readerOutput.GetNumberOfPoints() != 2739:
    print("ERROR: incorrect number of points from reader. Number of cells is ",
          readerOutput.GetNumberOfPoints(), " but should be 2739")
    sys.exit(1)

if readerOutput.GetNumberOfCells() != 4087:
    print("ERROR: incorrect number of cells from reader. Number of cells is ",
          readerOutput.GetNumberOfCells(), " but should be 4087")
    sys.exit(1)
