#/usr/bin/env python
from paraview.simple import *
from paraview.vtk.util.misc import vtkGetTempDir
from os.path import join

# This test tests that exporting of CSV from spreadsheet correctly exports column
# labels and respects column visibility, including not exporting internal
# columns.

def get_header(csvfilename):
    import csv
    reader = csv.reader(open(csvfilename, "r"), delimiter=",")
    for row in reader:
        return row
    return []

Sphere()
GroupDatasets()
UpdatePipeline()
Elevation()

filename = join(vtkGetTempDir(), "data.csv")
v = CreateView("SpreadSheetView")
r = Show()
ExportView(filename)

header = get_header(filename)
print(header)
assert ("Normals_0" in header and \
        "Block Name" in header and \
        "Elevation" in header and \
        "Point ID" in header and \
        "__vtkIsSelected__" not in header)

v.HiddenColumnLabels = ["Normals"]
Render()

ExportView(filename)
header = get_header(filename)
assert ("Normals_0" not in header and \
        "Block Name" in header and \
        "Elevation" in header and \
        "Point ID" in header and \
        "__vtkIsSelected__" not in header)
