# Tests links between proxies and properties.

import os
import os.path
import sys

from paraview import servermanager

import SMPythonTesting
  
SMPythonTesting.ProcessCommandLineArguments()
servermanager.Connect()

pvsm_file = os.path.join(SMPythonTesting.SMStatesDir, "ProxyPropertyLinks.pvsm")
print "State file: %s" % pvsm_file

SMPythonTesting.LoadServerManagerState(pvsm_file)

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

temp_state = os.path.join(SMPythonTesting.TempDir,"links_temp.pvsm")
pxm.SaveState(temp_state)

sphere1 = None
sphere2 = None
sphere3 = None
pxm.UnRegisterProxies()

# Load the saved state which also has the links.
SMPythonTesting.LoadServerManagerState(temp_state)
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

rmProxy = pxm.GetProxy("rendermodules","RenderModule0")
rmProxy.StillRender()

if not SMPythonTesting.DoRegressionTesting():
  # This will lead to VTK object leaks.
  sys.exit(1)
