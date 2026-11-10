# Test the side data of unstructured grids through the reader, data information,
# Pass Arrays, Python Calculator, spreadsheet view and writer.

import os.path

from paraview import smtesting
from paraview.simple import *
from vtkmodules.vtkCommonCore import vtkDoubleArray, vtkIntArray, vtkPoints
from vtkmodules.vtkCommonDataModel import VTK_HEXAHEDRON, vtkDataObject, vtkUnstructuredGrid
from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader, vtkXMLUnstructuredGridWriter

smtesting.ProcessCommandLineArguments()


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


# Two hexahedra sharing one face: 6 + 6 - 1 = 11 sides.
grid = vtkUnstructuredGrid()
points = vtkPoints()
for x in range(3):
    for y in range(2):
        for z in range(2):
            points.InsertNextPoint(x, y, z)
grid.SetPoints(points)


def pid(x, y, z):
    return x * 4 + y * 2 + z


for x in range(2):
    grid.InsertNextCell(VTK_HEXAHEDRON, 8, [
        pid(x, 0, 0), pid(x + 1, 0, 0), pid(x + 1, 1, 0), pid(x, 1, 0),
        pid(x, 0, 1), pid(x + 1, 0, 1), pid(x + 1, 1, 1), pid(x, 1, 1)])

numberOfSides = grid.GetNumberOfSides()
check(numberOfSides == 11, "expected 11 sides, got %d" % numberOfSides)

flux = vtkDoubleArray()
flux.SetName("flux")
tag = vtkIntArray()
tag.SetName("tag")
for i in range(numberOfSides):
    flux.InsertNextValue(i)
    tag.InsertNextValue(2 * i)
grid.GetSideData().AddArray(flux)
grid.GetSideData().AddArray(tag)

fileName = os.path.join(smtesting.TempDir, "UnstructuredGridSideData.vtu")
writer = vtkXMLUnstructuredGridWriter()
writer.SetFileName(fileName)
writer.SetInputData(grid)
check(writer.Write() == 1, "failed to write " + fileName)

# The reader lists the side arrays and reads the selected ones.
reader = XMLUnstructuredGridReader(FileName=[fileName])
reader.UpdatePipelineInformation()
check(sorted(reader.GetProperty("SideArrayInfo")[::2]) == ["flux", "tag"],
      "unexpected side arrays %s" % reader.GetProperty("SideArrayInfo"))
reader.SideArrayStatus = ["flux"]
reader.UpdatePipeline()
check(reader.SideData.keys() == ["flux"], "unexpected side data %s" % reader.SideData.keys())
check(reader.GetDataInformation().GetNumberOfSides() == numberOfSides, "wrong number of sides")
check(reader.SideData["flux"].GetRange() == (0.0, numberOfSides - 1.0), "wrong flux range")

reader.SideArrayStatus = ["flux", "tag"]
reader.UpdatePipeline()
check(sorted(reader.SideData.keys()) == ["flux", "tag"], "side arrays not all read")

# Pass Arrays filters the side arrays.
passArrays = PassArrays(Input=reader, PointDataArrays=[], CellDataArrays=[],
                        SideDataArrays=["tag"])
passArrays.UpdatePipeline()
check(passArrays.SideData.keys() == ["tag"], "Pass Arrays did not filter the side data")

# The Python Calculator computes on side arrays.
calculator = PythonCalculator(Input=reader, ArrayAssociation="Side Data",
                              Expression="flux * 2", ArrayName="flux2")
calculator.UpdatePipeline()
check("flux2" in calculator.SideData.keys(), "Python Calculator did not add a side array")
check(calculator.SideData["flux2"].GetRange() == (0.0, 2.0 * (numberOfSides - 1)),
      "wrong flux2 range")
check("flux" in calculator.SideData.keys(), "Python Calculator did not copy the side arrays")

# Filters that do not process side data do not offer it.
arrayCalculator = Calculator(Input=reader)
check("Side Data" not in arrayCalculator.GetProperty("AttributeType").Available,
      "Calculator offers side data")

# The spreadsheet view shows one row per side.
view = CreateView("SpreadSheetView")
Show(reader, view)
view.FieldAssociation = vtkDataObject.FIELD_ASSOCIATION_SIDES
Render(view)
check(view.GetClientSideObject().GetNumberOfRows() == numberOfSides,
      "spreadsheet shows %d rows" % view.GetClientSideObject().GetNumberOfRows())

# The writer writes the selected side arrays.
outFileName = os.path.join(smtesting.TempDir, "UnstructuredGridSideDataOut.vtu")
SaveData(outFileName, proxy=calculator, ChooseArraysToWrite=1, PointDataArrays=[],
         CellDataArrays=[], SideDataArrays=["flux2"])
check2 = vtkXMLUnstructuredGridReader()
check2.SetFileName(outFileName)
check2.Update()
sideData = check2.GetOutput().GetSideData()
names = [sideData.GetArrayName(i) for i in range(sideData.GetNumberOfArrays())]
check(names == ["flux2"], "wrote side arrays %s" % names)
