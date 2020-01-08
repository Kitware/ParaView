from paraview.simple import *
from paraview import smtesting
import vtk
from paraview.vtk.vtkCommonCore import vtkCollection;
import vtk.vtkRenderingVolume
import os

paraview.simple._DisableFirstRenderCameraReset()

smtesting.ProcessCommandLineArguments()
smtesting.LoadServerManagerState(smtesting.StateXMLFileName)

view = GetRenderView()
view.ViewSize = [400, 400]
view.RemoteRenderThreshold = 0
SetActiveView(view)
Render()

# Select cells from a wavelet volume
wav = FindSource('Wavelet1')
SetActiveSource(wav)

rep = vtkCollection()
sources = vtkCollection()
view.SelectSurfaceCells([0, 0, 200, 200], rep, sources)
sel = sources.GetItemAsObject(0)
selPyProxy = servermanager._getPyProxy(sel)
extract = ExtractSelection(Selection = selPyProxy)
Show()

# Hide the volume and show the ExtractSelection filter
wav_rep = GetRepresentation(wav)
wav_rep.Visibility = False

extract_rep = GetRepresentation(extract)
extract_rep.Visibility = True

## Compare with baseline
if not smtesting.DoRegressionTesting(view.SMProxy):
  raise smtesting.TestError ('Test failed.')

print ('\nTest passed.')
