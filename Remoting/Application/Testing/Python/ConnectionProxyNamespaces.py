# This tests is for creation of class types for proxies.
# This can grow to validate different components of the logic
# create classes for known proxy types.

from paraview import servermanager as sm
from paraview import simple, print_warning

c = sm.ActiveConnection
assert c

# get the proxy manager.
smpxm = c.Session.GetSessionProxyManager()

#==============================================================================
# create a proxy and confirm we can obtain a class for it.
sphere = smpxm.NewProxy("sources", "SphereSource")
sphere.UnRegister(None)

cls = c.ProxiesNS.getClass(sphere)
assert cls is not None
assert type(getattr(cls, "Radius")) == property

# let's create another proxy of the same type using a different route and
# confirm the same class type is returned.
sphere2 = simple.Sphere()
sphere3 = simple.Sphere()
assert sphere2.__class__ == cls
assert sphere3.__class__ == cls

#==============================================================================
# now, confirm that is two proxies with same name but different groups are created
# we get different classes for each. This avoids issues like #20672.
p1 = smpxm.NewProxy("extract_writers", "PNG")
p1.UnRegister(None)
cls1 = c.ProxiesNS.getClass(p1)

p2 = smpxm.NewProxy("animation_writers", "PNG")
p2.UnRegister(None)
cls2 = c.ProxiesNS.getClass(p2)

assert cls1 is not cls2

#==============================================================================
# create a proxy not in one of the standard proxy groups.
assert not hasattr(c.ProxiesNS, "delivery_managers")
dm = smpxm.NewProxy("delivery_managers", "RenderViewDeliveryManager")
dm.UnRegister(None)

cls = c.ProxiesNS.getClass(dm)

assert cls is not None
help(cls)

try:
    simple.LoadDistributedPlugin("Moments")
    test_plugin = True
except RuntimeError:
    print_warning("Moments plugin cannot be loaded. Skipping plugin testing")
    test_plugin = False

if test_plugin:
    assert "MomentVectors" in dir(c.ProxiesNS.filters)
    mv = simple.MomentVectors()
