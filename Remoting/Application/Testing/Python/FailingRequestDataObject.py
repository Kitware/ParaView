# Tests BUG #21069

from paraview.simple import *
from paraview import smtesting
from vtkmodules.vtkCommonCore import vtkLogger
smtesting.ProcessCommandLineArguments()

programmable_source = ProgrammableSource()
programmable_source.OutputDataSetType = 'vtkImageData'
programmable_source.Script = """from vtkmodules.vtkCommonCore import VTK_FLOAT
output = self.GetOutputDataObject(0)
output.SetExtent(0, 9, 0, 9, 0, 9)
output.AllocateScalars(VTK_FLOAT, 1)"""
programmable_source.ScriptRequestInformation = """executive = self.GetExecutive()
outInfo = executive.GetOutputInformation(0)
outInfo.Set(executive.WHOLE_EXTENT(), 0, 9, 0, 9, 0, 9)"""
Show(programmable_source)

subset = ExtractSubset(Input=programmable_source)
subset.VOI = [0, 9, 0, 9, 0, 0]
Show(subset)

view = GetRenderView()
view.StillRender()

assert subset.GetDataInformation().GetExtent() == (0, 9, 0, 9, 0, 0)

logger_verbosity = vtkLogger.GetCurrentVerbosityCutoff()
try:
  # suppress VTK errors, since we need the following error to happen
  vtkLogger.SetStderrVerbosity(vtkLogger.VERBOSITY_OFF)

  # change input type such that REQUEST_DATA_OBJECT of vtkPVExtractVOI fails
  programmable_source.OutputDataSetType = 'vtkTable'
  view.StillRender()
finally:
  # re-enable VTK errors
  vtkLogger.SetStderrVerbosity(logger_verbosity)

programmable_source.OutputDataSetType = 'vtkImageData'
subset.VOI = [0, 9, 0, 9, 9, 9]
view.StillRender()

assert subset.GetDataInformation().GetExtent() == (0, 9, 0, 9, 9, 9)
