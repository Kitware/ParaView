# Test the vtkCellIntegrator

import SMPythonTesting
import os
import os.path
import sys
import paraview

SMPythonTesting.ProcessCommandLineArguments()

paraview.ActiveConnection = paraview.Connect()

file1 = os.path.join(SMPythonTesting.DataDir, "Data/quadraticTetra01.vtu")
reader1 = paraview.CreateProxy("sources", "XMLUnstructuredGridReader")
reader1.GetProperty("FileName").SetElement(0, file1)
reader1.UpdateVTKObjects()
reader1Output = paraview.Fetch(reader1)

# General 3D cell
integVal = paraview.IntegrateCell(reader1Output, 0)
if integVal < 0.128 or integVal > 0.1285:
    print "ERROR: incorrect result for cell 0 of 1st dataset"
    sys.exit(1)

# General 2D cell
if paraview.IntegrateCell(reader1Output, 1) != 0.625:
    print "ERROR: incorrect result for cell 1 of 1st dataset"
    sys.exit(1)

file2 = os.path.join(SMPythonTesting.DataDir, "Data/elements.vtu")
reader2 = paraview.CreateProxy("sources", "XMLUnstructuredGridReader")
reader2.GetProperty("FileName").SetElement(0, file2)
reader2.UpdateVTKObjects()
reader2Output = paraview.Fetch(reader2)

# Line
if paraview.IntegrateCell(reader2Output, 2) != 1.0:
    print "ERROR: incorrect result for cell 2 of 2nd dataset"
    sys.exit(1)

# Triangle
if paraview.IntegrateCell(reader2Output, 4) != 0.5:
    print "ERROR: incorrect result for cell 4 of 2nd dataset"
    sys.exit(1)

# Quad
if paraview.IntegrateCell(reader2Output, 6) != 1.0:
    print "ERROR: incorrect result for cell 6 of 2nd dataset"
    sys.exit(1)

# Pixel
if paraview.IntegrateCell(reader2Output, 7) != 1.0:
    print "ERROR: incorrect result for cell 7 of 2nd dataset"
    sys.exit(1)

# Tetrahedron
integVal = paraview.IntegrateCell(reader2Output, 8)
if integVal < 0.166 or integVal > 0.167:
    print "ERROR: incorrect result for cell 8 of 2nd dataset"
    sys.exit(1)

# Voxel
if paraview.IntegrateCell(reader2Output, 13) != 1.0:
    print "ERROR: incorrect result for cell 13 of 2nd dataset"
    sys.exit(1)

# Polygon
if paraview.IntegrateCell(reader2Output, 0) != 1.0:
    print "ERROR: incorrect result for cell 0 of 2nd dataset"
    sys.exit(1)

file3 = os.path.join(SMPythonTesting.DataDir, "Data/blow.vtk")
reader3 = paraview.CreateProxy("sources", "legacyreader")
reader3.GetProperty("FileName").SetElement(0, file3)
reader3.UpdateVTKObjects()
reader3Output = paraview.Fetch(reader3)

filter1 = paraview.CreateProxy("filters", "DataSetSurfaceFilter")
filter1.GetProperty("Input").AddProxy(reader3.SMProxy, 0)
filter1.UpdateVTKObjects()

filter2 = paraview.CreateProxy("filters", "Stripper")
filter2.GetProperty("Input").AddProxy(filter1.SMProxy, 0)
filter2.UpdateVTKObjects()
filter2Output = paraview.Fetch(filter2)

# Triangle Strip
integVal = paraview.IntegrateCell(filter2Output, 200)
if integVal < 0.569 or integVal > 0.570:
    print "ERROR: incorrect result for cell 200 of 3rd dataset"
    sys.exit(1)
