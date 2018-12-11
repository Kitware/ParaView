# Test the vtkCellIntegrator

from paraview import smtesting
import os
import os.path
import sys
import paraview
paraview.compatibility.major = 3
paraview.compatibility.minor = 4
from paraview import servermanager
from paraview import util

smtesting.ProcessCommandLineArguments()

servermanager.Connect()

file1 = os.path.join(smtesting.DataDir, "Testing/Data/quadraticTetra01.vtu")
reader1 = servermanager.sources.XMLUnstructuredGridReader(FileName=file1)
reader1Output = servermanager.Fetch(reader1)

# General 3D cell
integVal = util.IntegrateCell(reader1Output, 0)
if integVal < 0.128 or integVal > 0.1285:
    print("ERROR: incorrect result for cell 0 of 1st dataset")
    sys.exit(1)

# General 2D cell
if util.IntegrateCell(reader1Output, 1) != 0.625:
    print("ERROR: incorrect result for cell 1 of 1st dataset")
    sys.exit(1)

file2 = os.path.join(smtesting.DataDir, "Testing/Data/elements.vtu")
reader2 = servermanager.sources.XMLUnstructuredGridReader(FileName=file2)
reader2Output = servermanager.Fetch(reader2)

# Line
if util.IntegrateCell(reader2Output, 2) != 1.0:
    print("ERROR: incorrect result for cell 2 of 2nd dataset")
    sys.exit(1)

# Triangle
if util.IntegrateCell(reader2Output, 4) != 0.5:
    print("ERROR: incorrect result for cell 4 of 2nd dataset")
    sys.exit(1)

# Quad
if util.IntegrateCell(reader2Output, 6) != 1.0:
    print("ERROR: incorrect result for cell 6 of 2nd dataset")
    sys.exit(1)

# Pixel
if util.IntegrateCell(reader2Output, 7) != 1.0:
    print("ERROR: incorrect result for cell 7 of 2nd dataset")
    sys.exit(1)

# Tetrahedron
integVal = util.IntegrateCell(reader2Output, 8)
if integVal < 0.166 or integVal > 0.167:
    print("ERROR: incorrect result for cell 8 of 2nd dataset")
    sys.exit(1)

# Voxel
if util.IntegrateCell(reader2Output, 13) != 1.0:
    print("ERROR: incorrect result for cell 13 of 2nd dataset")
    sys.exit(1)

# Polygon
if util.IntegrateCell(reader2Output, 0) != 1.0:
    print("ERROR: incorrect result for cell 0 of 2nd dataset")
    sys.exit(1)

file3 = os.path.join(smtesting.DataDir, "Testing/Data/blow.vtk")
reader3 = servermanager.sources.LegacyVTKFileReader(FileNames=file3)
reader3Output = servermanager.Fetch(reader3)

filter1 = servermanager.filters.DataSetSurfaceFilter(Input = reader3)

filter2 = servermanager.filters.Stripper(Input=filter1)
filter2Output = servermanager.Fetch(filter2)

# Triangle Strip
integVal = util.IntegrateCell(filter2Output, 200)
if integVal < 0.569 or integVal > 0.570:
    print("ERROR: incorrect result for cell 200 of 3rd dataset")
    sys.exit(1)
