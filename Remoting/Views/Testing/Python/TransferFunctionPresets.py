from paraview.simple import *
from paraview import smtesting

smtesting.ProcessCommandLineArguments()

transferFunctionPresets = servermanager.vtkSMTransferFunctionPresets()

numberOfPresets = transferFunctionPresets.GetNumberOfPresets()
transferFunctionPresets.ImportPresets(smtesting.DataDir + '/Testing/Data/x_ray_copy_1.json')
assert(transferFunctionPresets.GetNumberOfPresets() == numberOfPresets + 1)
assert(transferFunctionPresets.GetPresetName(numberOfPresets) == "X_Ray_Copy_1")
