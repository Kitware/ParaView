from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

view = CreateView("SpreadSheetView")
sphere = Sphere(ThetaResolution=3, PhiResolution=3)
Show()
Render()

pvview = view.GetClientSideObject()

pvview.InitializeOrderedColumnList()
columns = list(pvview.GetOrderedColumnList())

# Swap some column name
n0_idx = columns.index('Normals_0')
pm_index = columns.index('Points_Magnitude')
columns[n0_idx], columns[pm_index] = columns[pm_index], columns[n0_idx]

pvview.SetOrderedColumnList(columns)
pvview.OrderColumnsByList(1)

assert pvview.GetColumnLabel(1) == "Points_Magnitude"
assert pvview.GetColumnLabel(2) == "Normals"            # Normals_1
assert pvview.GetColumnLabel(3) == "Normals"            # Normals_2
assert pvview.GetColumnLabel(4) == "Normals_Magnitude"
assert pvview.GetColumnLabel(5) == "Points"             # Points_1
assert pvview.GetColumnLabel(6) == "Points"             # Points_2
assert pvview.GetColumnLabel(7) == "Points"             # Points_3
assert pvview.GetColumnLabel(8) == "Normals"            # Normals_0
