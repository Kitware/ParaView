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

view = servermanager.CreateRenderView();

# using offscreen avoids issues with overlapping windows and such.
view.UseOffscreenRendering = 1
view.Background = (.5,.1,.5)
if view.GetProperty("RemoteRenderThreshold"):
    view.RemoteRenderThreshold = 100;

repr = servermanager.CreateRepresentation(sphere, view);
repr.Input.append(sphere)

view.StillRender()
view.ResetCamera()
view.StillRender()

if not smtesting.DoRegressionTesting(view.SMProxy):
    raise smtesting.TestError('Test failed.')
