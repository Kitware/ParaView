# Simple Test for pvbatch.

import SMPythonTesting
from paraview import servermanager

import sys

SMPythonTesting.ProcessCommandLineArguments()

servermanager.Connect()

sphere = servermanager.sources.SphereSource()
fname = SMPythonTesting.TempDir+"/stlfile.stl"
writer = servermanager.writers.PSTLWriter(Input=sphere, FileName=fname)
writer.UpdatePipeline()

reader = servermanager.sources.stlreader(FileNames=(fname,))

view = servermanager.CreateRenderView();
view.Background = (.5,.1,.5)
if view.GetProperty("RemoteRenderThreshold"):
    view.RemoteRenderThreshold = 100;

repr = servermanager.CreateRepresentation(reader, view);

view.UseOffscreenRenderingForScreenshotsOff()
view.ResetCamera()
view.StillRender()

SMPythonTesting.DoRegressionTesting(view.SMProxy)
if not SMPythonTesting.DoRegressionTesting(view.SMProxy):
    raise SMPythonTesting.TestError, 'Test failed.'
