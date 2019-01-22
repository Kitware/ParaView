# Test the vtkIntegrateAttributes filter.

from paraview import smtesting
import os
import os.path
import sys

import paraview
paraview.compatibility.major = 3
paraview.compatibility.minor = 4

from paraview import *

smtesting.ProcessCommandLineArguments()

servermanager.Connect()

file1 = os.path.join(smtesting.DataDir, "Testing/Data/quadraticTetra01.vtu")
reader1 = servermanager.sources.XMLUnstructuredGridReader(FileName=file1)

filter1 = servermanager.filters.IntegrateAttributes(Input=reader1)
filter1Output = servermanager.Fetch(filter1)
val = filter1Output.GetPointData().GetArray("scalars").GetValue(0)
if val < 0.0162 or val > 0.01621:
    print("ERROR: Wrong scalars value for dataset 1")
    sys.exit(1)

val = filter1Output.GetCellData().GetArray("Volume").GetValue(0)
if val < 0.128 or val > 0.1284:
    print("ERROR: Wrong Volume value for dataset 1")
    sys.exit(1)

file2 = os.path.join(smtesting.DataDir, "Testing/Data/elements.vtu")
reader2 = servermanager.sources.XMLUnstructuredGridReader(FileName=file2)

filter2 = servermanager.filters.IntegrateAttributes(Input=reader2)
filter2Output = servermanager.Fetch(filter2)
val = filter2Output.GetPointData().GetArray("pointScalars").GetValue(0)
#if val < 207.499 or val > 207.501:
#    print "ERROR: Wrong pointScalars value for dataset 2"
#    sys.exit(1)

#val = filter2Output.GetCellData().GetArray("Volume").GetValue(0)
#if val < 3.33 or val > 3.34:
#    print "ERROR: Wrong Volume value for dataset 2"
#    sys.exit(1)

file3 = os.path.join(smtesting.DataDir, "Testing/Data/blow.vtk")
reader3 = servermanager.sources.LegacyVTKFileReader(FileNames=file3)

filter3 = servermanager.filters.DataSetSurfaceFilter(Input=reader3)

filter4 = servermanager.filters.Stripper(Input=filter3)

filter5 = servermanager.filters.IntegrateAttributes(Input=filter4)
filter5Output = servermanager.Fetch(filter5)

val = filter5Output.GetPointData().GetArray("displacement1").GetValue(0)
if val < 463.64 or val > 463.642:
    print("ERROR: Wrong displacement1 value for dataset 3")
    sys.exit(1)

val = filter5Output.GetPointData().GetArray("thickness3").GetValue(0)
if val < 874.61 or val > 874.618:
    print("ERROR: Wrong thickness3 value for dataset 3")
    sys.exit(1)

val = filter5Output.GetCellData().GetArray("Area").GetValue(0)
if val < 1145.405 or val > 1145.415:
    print("ERROR: Wrong Area value for dataset 3")
    sys.exit(1)
