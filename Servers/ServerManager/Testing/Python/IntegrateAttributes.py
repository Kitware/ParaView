# Test the vtkIntegrateAttributes filter.

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

filter1 = paraview.CreateProxy("filters", "IntegrateAttributes")
filter1.GetProperty("Input").AddProxy(reader1.SMProxy, 0)
filter1.UpdateVTKObjects()
filter1Output = paraview.Fetch(filter1)
val = filter1Output.GetPointData().GetArray("scalars").GetValue(0)
if val < 0.0162 or val > 0.01621:
    print "ERROR: Wrong scalars value for dataset 1"
    sys.exit(1)

val = filter1Output.GetCellData().GetArray("Volume").GetValue(0)
if val < 0.128 or val > 0.1284:
    print "ERROR: Wrong Volume value for dataset 1"
    sys.exit(1)

file2 = os.path.join(SMPythonTesting.DataDir, "Data/elements.vtu")
reader2 = paraview.CreateProxy("sources", "XMLUnstructuredGridReader")
reader2.GetProperty("FileName").SetElement(0, file2)
reader2.UpdateVTKObjects()

filter2 = paraview.CreateProxy("filters", "IntegrateAttributes")
filter2.GetProperty("Input").AddProxy(reader2.SMProxy, 0)
filter2.UpdateVTKObjects()
filter2Output = paraview.Fetch(filter2)
val = filter2Output.GetPointData().GetArray("pointScalars").GetValue(0)
if val < 207.499 or val > 207.501:
    print "ERROR: Wrong pointScalars value for dataset 2"
    sys.exit(1)

val = filter2Output.GetCellData().GetArray("Volume").GetValue(0)
if val < 3.33 or val > 3.34:
    print "ERROR: Wrong Volume value for dataset 2"
    sys.exit(1)

file3 = os.path.join(SMPythonTesting.DataDir, "Data/blow.vtk")
reader3 = paraview.CreateProxy("sources", "LegacyVTKFileReader")
reader3.GetProperty("FileNames").SetElement(0, file3)
reader3.UpdateVTKObjects()

filter3 = paraview.CreateProxy("filters", "DataSetSurfaceFilter")
filter3.GetProperty("Input").AddProxy(reader3.SMProxy, 0)
filter3.UpdateVTKObjects()

filter4 = paraview.CreateProxy("filters", "Stripper")
filter4.GetProperty("Input").AddProxy(filter3.SMProxy, 0)
filter4.UpdateVTKObjects()

filter5 = paraview.CreateProxy("filters", "IntegrateAttributes")
filter5.GetProperty("Input").AddProxy(filter4.SMProxy, 0)
filter5.UpdateVTKObjects()
filter5Output = paraview.Fetch(filter5)

val = filter5Output.GetPointData().GetArray("displacement1").GetValue(0)
if val < 463.64 or val > 463.642:
    print "ERROR: Wrong displacement1 value for dataset 3"
    sys.exit(1)

val = filter5Output.GetPointData().GetArray("thickness3").GetValue(0)
if val < 874.61 or val > 874.618:
    print "ERROR: Wrong thickness3 value for dataset 3"
    sys.exit(1)

val = filter5Output.GetCellData().GetArray("Area").GetValue(0)
if val < 1145.405 or val > 1145.415:
    print "ERROR: Wrong Area value for dataset 3"
    sys.exit(1)
