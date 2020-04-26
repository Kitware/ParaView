from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

tempdir = smtesting.GetUniqueTempDirectory("SpreadSheetViewBlockNames")

view = CreateView("SpreadSheetView")
OpenDataFile(smtesting.DataDir + '/Testing/Data/can.ex2')
Show()
Render()

pvview = view.GetClientSideObject()

assert pvview.GetColumnLabel(0) == "Block Name"
assert pvview.GetValue(0, 0).ToString() == "Unnamed block ID: 1"
assert pvview.GetValue(6723, 0).ToString() == "Unnamed block ID: 1"
assert pvview.GetValue(6724, 0).ToString() == "Unnamed block ID: 2"
assert pvview.GetValue(10087, 0).ToString() == "Unnamed block ID: 2"
