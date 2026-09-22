import os
import sys
import tempfile

from paraview.simple import LoadDistributedPlugin
from paraview import servermanager

LoadDistributedPlugin("CAVEInteraction", remote=False, ns=globals())

MODULE = "paraview.incubator.vtkPVIncubatorCAVEInteractionStyles"
assert MODULE not in sys.modules, "the test must not import the styles module itself"

pxm = servermanager.ProxyManager().SMProxyManager

# Make sure creating the proxy returns an object that is correctly typed
proxy = pxm.NewProxy("cave_interaction", "Python")
assert MODULE in sys.modules, "constructing the style proxy did not import its module"
assert type(proxy).__name__ == "vtkSMVRPythonInteractorStyleProxy", type(proxy).__name__
assert hasattr(proxy, "ClearAllRoles"), "vtkSelf would be a bare vtkSMProxy"

# Make sure a style that doesn't import the incubator module can
# use vtkSelf methods defined in vtkSMVRInteractorStyleProxy
STYLE = """
from paraview.incubator.pythoninteractorbase import PythonInteractorBase

def create_interactor_style():
    return Probe()

class Probe(PythonInteractorBase):
    def Initialize(self, vtkSelf):
        vtkSelf.ClearAllRoles()
        vtkSelf.AddTrackerRole("Probe")
"""

with tempfile.NamedTemporaryFile("w", suffix=".py", delete=False) as f:
    f.write(STYLE)
    path = f.name

try:
    proxy.GetProperty("FileName").SetElement(0, path)

    # Trigger reading the file, creating the style, and calling Initialize()
    proxy.UpdateVTKObjects()

    # If AddTrackerRole("Probe") didn't work, SetTrackerName returns False for
    # the "Probe" role.
    assert proxy.SetTrackerName("Probe", "tracker0"), "Initialize() did not add the role"
    assert proxy.GetTrackerName("Probe") == "tracker0"
finally:
    os.remove(path)
    # Unregister the reference given to us by NewProxy()
    proxy.UnRegister(None)

print("TestPythonInteractorStyleSelf: OK")
