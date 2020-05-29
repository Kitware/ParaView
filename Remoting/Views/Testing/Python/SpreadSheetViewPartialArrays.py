from paraview.simple import *
from paraview import smtesting
smtesting.ProcessCommandLineArguments()

sphere = Sphere(ThetaResolution=3, PhiResolution=3)
cal1 = Calculator(Input=sphere, ResultArrayName="Var1", Function="1")
cal2 = Calculator(Input=sphere, ResultArrayName="Var2", Function="2")
group = GroupDatasets(Input=[cal1, cal2])

view = CreateView("SpreadSheetView")
Show()
Render()

pvview = view.GetClientSideObject()
pvview.GetValue(0, 0)

var1Col = pvview.GetColumnByName("Var1")
assert var1Col >= 0

var2Col = pvview.GetColumnByName("Var2")
assert var2Col >= 0

assert pvview.GetValue(0, var1Col).ToInt() == 1 and pvview.IsDataValid(0, var1Col)
assert pvview.GetValue(0, var2Col).ToInt() == 0 and not pvview.IsDataValid(0, var2Col)

assert pvview.GetValue(9, var1Col).ToInt() == 0 and not pvview.IsDataValid(9, var1Col)
assert pvview.GetValue(9, var2Col).ToInt() == 2 and pvview.IsDataValid(9, var2Col)
