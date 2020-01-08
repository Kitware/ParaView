"""
    This test ensures that when helper proxies are loaded from state files, they
    get registered in groups that match the proxy's global id
    (Issue #16862, #16863)
"""

from paraview.simple import *
from paraview import smtesting
from os.path import join, isfile
import os

smtesting.ProcessCommandLineArguments()
assert smtesting.TempDir
statefile = join(smtesting.TempDir, "TestHelperProxySerialization.pvsm")
if isfile(statefile):
    os.remove(statefile)

pxm = servermanager.ProxyManager()

# Create a sphere.
s = Sphere()
gid = s.GetGlobalIDAsString()

# PV should have create a `RepresentationAnimationHelper` for it.
assert pxm.GetProxy("pq_helper_proxies." + gid, "RepresentationAnimationHelper") != None

# Save the state
SaveState(statefile)
Delete(s)

# Ensure that the `RepresentationAnimationHelper` helper is gone too.
assert pxm.GetProxy("pq_helper_proxies." + gid, "RepresentationAnimationHelper") == None

assert isfile(statefile)
# Load the that
LoadState(statefile)

# Ensure that the `RepresentationAnimationHelper` helper is not created
# using the name saved in the state file.
assert pxm.GetProxy("pq_helper_proxies." + gid, "RepresentationAnimationHelper") == None
s = FindSource("Sphere1")
assert s != None
gid = s.GetGlobalIDAsString()

# Ensure that `RepresentationAnimationHelper` is present, but under the new
# name.
assert pxm.GetProxy("pq_helper_proxies." + gid, "RepresentationAnimationHelper") != None
os.remove(statefile)
