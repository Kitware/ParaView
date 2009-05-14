# Simple Test for pvbatch.

import SMPythonTesting
from paraview import servermanager

import sys

SMPythonTesting.ProcessCommandLineArguments()

servermanager.Connect()

sphere = servermanager.sources.SphereSource()

view = servermanager.CreateRenderView();
view.Background = (.5,.1,.5)
if view.GetProperty("RemoteRenderThreshold"):
    view.RemoteRenderThreshold = 100;

repr = servermanager.CreateRepresentation(sphere, view);
repr.Input.append(sphere)

view.UseOffscreenRenderingForScreenshotsOff()
view.ResetCamera()
view.StillRender()

if not SMPythonTesting.DoRegressionTesting(view.SMProxy):
    raise SMPythonTesting.TestError, 'Test failed.'
