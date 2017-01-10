# Simple Test for pvbatch.

from paraview import smtesting
import paraview
paraview.compatibility.major = 3
paraview.compatibility.minor = 4
from paraview import servermanager

import sys

smtesting.ProcessCommandLineArguments()

servermanager.Connect()

sphere = servermanager.sources.SphereSource()
fname = smtesting.TempDir+"/stlfile.stl"
writer = servermanager.writers.PSTLWriter(Input=sphere, FileName=fname)
writer.UpdatePipeline()

pm = servermanager.vtkProcessModule.GetProcessModule()
if pm.GetSymmetricMPIMode() == True:
    # need to barrier to ensure that all ranks have updated.
    controller = pm.GetGlobalController()
    controller.Barrier()

reader = servermanager.sources.stlreader(FileNames=(fname,))

view = servermanager.CreateRenderView();
# using offscreen avoids issues with overlapping windows and such.
view.UseOffscreenRendering = 1
view.Background = (.5,.1,.5)
if view.GetProperty("RemoteRenderThreshold"):
    view.RemoteRenderThreshold = 100;

repr = servermanager.CreateRepresentation(reader, view);
view.StillRender()
view.ResetCamera()
view.StillRender()

smtesting.DoRegressionTesting(view.SMProxy)
if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError('Test failed.')
