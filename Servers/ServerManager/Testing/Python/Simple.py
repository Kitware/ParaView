# Simple Test for pvbatch.

import SMPythonTesting
import paraview

import sys

SMPythonTesting.ProcessCommandLineArguments()

sphere = paraview.CreateProxy("sources","SphereSource");
sphere.UpdateVTKObjects();

connection = paraview.ActiveConnection;
if connection.IsRemote():
    proxy_xml_name = "IceTDesktopRenderView"
else:
    if connection.GetNumberOfDataPartitions() > 1:
        proxy_xml_name = "IceTCompositeView"
    else:
        proxy_xml_name = "RenderView"

print "RenderView--- %s" % proxy_xml_name
view = paraview.CreateProxy("newviews", proxy_xml_name);
view.SetBackground(.5,.1,.5);
if view.GetProperty("RemoteRenderThreshold"):
    view.SetRemoteRenderThreshold(100);
view.UpdateVTKObjects();

repr = view.CreateDefaultRepresentation(sphere);
repr.UnRegister(None)
repr.AddToInput(sphere);
repr.UpdateVTKObjects();

view.AddToRepresentations(repr);
view.UpdateVTKObjects();

view.ResetCamera();
view.StillRender();

if not SMPythonTesting.DoRegressionTesting(view.SMProxy):
    # This will lead to VTK object leaks.
    sys.exit(1)
