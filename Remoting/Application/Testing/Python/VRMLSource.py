# Test the vtkVRMLSource and fetching multi-group data.
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

file1 = os.path.join(smtesting.DataDir, "Testing/Data/bot2.wrl")
reader = servermanager.sources.vrmlreader(FileName = file1)
readerOutput = servermanager.Fetch(reader)
if readerOutput.GetClassName() != "vtkMultiBlockDataSet":
    print("ERROR: Wrong dataset type returned: %s" % readerOutput.GetClassName())
    sys.exit(1)

if readerOutput.GetNumberOfPoints() != 337:
    print("ERROR: Wrong number of points returned.")
    sys.exit(1)

if readerOutput.GetNumberOfBlocks() != 1:
    print("ERROR: Wrong number of blocks returned.")
    sys.exit(1)

ds0 = readerOutput.GetBlock(0)

if ds0.GetClassName() != "vtkPolyData":
    print("ERROR: Wrong dataset type returned for dataset (0, 0).")
    sys.exit(1)

if ds0.GetNumberOfPoints() != 337:
    print("ERROR: Wrong number of points returned for dataset (0, 0).")
    sys.exit(1)

if ds0.GetNumberOfCells() != 486:
    print("ERROR: Wrong number of cells returned for dataset (0, 0).")
    sys.exit(1)

colorArray = ds0.GetPointData().GetArray("VRMLColor")

if colorArray.GetRange(0)[0] != 8.0:
    print("ERROR: Wrong minimum value for component 0 of VRMLColor.")
    sys.exit(1)

if colorArray.GetRange(0)[1] != 255.0:
    print("ERROR: Wrong maximum value for component 0 of VRMLColor.")
    sys.exit(1)

if colorArray.GetRange(1)[0] != 8.0:
    print("ERROR: Wrong minimum value for component 1 of VRMLColor.")
    sys.exit(1)

if colorArray.GetRange(1)[1] != 255.0:
    print("ERROR: Wrong maximum value for component 1 of VRMLColor.")
    sys.exit(1)

if colorArray.GetRange(2)[0] != 26.0:
    print("ERROR: Wrong minimum value for component 2 of VRMLColor.")
    sys.exit(1)

if colorArray.GetRange(2)[1] != 255.0:
    print("ERROR: Wrong maximum value for component 2 of VRMLColor.")
    sys.exit(1)
