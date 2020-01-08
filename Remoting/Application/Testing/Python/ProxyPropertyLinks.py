# Tests links between proxies and properties.

import os
import os.path
import sys

from paraview import servermanager

from paraview import smtesting

smtesting.ProcessCommandLineArguments()
servermanager.Connect()

pvsm_file = os.path.join(smtesting.SMStatesDir, "ProxyPropertyLinks.pvsm")
print("State file: %s" % pvsm_file)

smtesting.LoadServerManagerState(pvsm_file)

pxm = servermanager.ProxyManager()
sphere1 = pxm.GetProxy("sources", "Sphere1")
sphere2 = pxm.GetProxy("sources", "Sphere2")
sphere3 = pxm.GetProxy("sources", "Sphere3")

# Create links.
proxyLink = servermanager.vtkSMProxyLink()
proxyLink.AddLinkedProxy(sphere1.SMProxy, 1) # Input
proxyLink.AddLinkedProxy(sphere2.SMProxy, 2) # Output
pxm.RegisterLink("MyProxyLink", proxyLink)
proxyLink = None

propertyLink = servermanager.vtkSMPropertyLink()
propertyLink.AddLinkedProperty(sphere3.SMProxy, "EndTheta", 1) # Input.
propertyLink.AddLinkedProperty(sphere1.SMProxy, "StartTheta", 2) # Output.
pxm.RegisterLink("MyPropertyLink", propertyLink)
propertyLink = None

temp_state = os.path.join(smtesting.TempDir,"links_temp.pvsm")
pxm.SaveState(temp_state)

sphere1 = None
sphere2 = None
sphere3 = None
pxm.UnRegisterProxies()
pxm.UnRegisterAllLinks()

# Load the saved state which also has the links.
smtesting.LoadServerManagerState(temp_state)
try:
  os.remove(saved_state)
except:
  pass
sphere1 = pxm.GetProxy("sources", "Sphere1")
sphere2 = pxm.GetProxy("sources", "Sphere2")
sphere3 = pxm.GetProxy("sources", "Sphere3")

# Do some changes.
sphere1.GetProperty("StartPhi").SetElement(0, 25)
sphere1.UpdateVTKObjects()

sphere3.GetProperty("EndTheta").SetElement(0, 100)
sphere3.GetProperty("ThetaResolution").SetElement(0, 10)
sphere3.GetProperty("PhiResolution").SetElement(0, 10)
sphere3.UpdateVTKObjects()

rmProxy = servermanager.GetRenderView()
rmProxy.StillRender()

if not smtesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)
