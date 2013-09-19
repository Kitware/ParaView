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

reader = servermanager.sources.stlreader(FileNames=(fname,))

view = servermanager.CreateRenderView();
view.Background = (.5,.1,.5)
if view.GetProperty("RemoteRenderThreshold"):
    view.RemoteRenderThreshold = 100;

repr = servermanager.CreateRepresentation(reader, view);

# view.UseOffscreenRenderingForScreenshots = 0
# Hackery to ensure that we don't end up with overlapping windows when running
# this test.
try:
    pm = servermanager.vtkProcessModule.GetProcessModule()
    if pm.GetPartitionId() == 0:
        window = view.GetRenderWindow()
        window.SetPosition(450, 0)
except:
    pass
view.StillRender()
view.ResetCamera()
view.StillRender()

smtesting.DoRegressionTesting(view.SMProxy)
if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError, 'Test failed.'
