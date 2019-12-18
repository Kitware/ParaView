"""This is a test to test the paraview proxy manager API."""

import paraview
paraview.compatibility.major = 3
paraview.compatibility.minor = 4
from paraview import servermanager

def Error(message):
  raise Exception("ERROR: %s" % message)

servermanager.Connect()
pxm = servermanager.ProxyManager()

p1 = servermanager.sources.SphereSource()
p2 = servermanager.sources.ConeSource()
p3 = servermanager.sources.ArrowSource()

pxm.RegisterProxy("sources", "source1_2", p1);
pxm.RegisterProxy("sources", "source1_2", p2);
pxm.RegisterProxy("sources", "source2", p2);
pxm.RegisterProxy("filters", "s1", p1);
pxm.RegisterProxy("filters", "s2", p2);
pxm.RegisterProxy("filters", "s2", p3);
pxm.RegisterProxy("filters", "s3", p3);

iter = pxm.__iter__();
for proxy in iter:
  print("%s.%s ==> %s" % (iter.GetGroup(), iter.GetKey(), proxy.GetXMLName()))

print("Number of sources: %d" % pxm.GetNumberOfProxies("sources"))
print("Number of filters: %d" % pxm.GetNumberOfProxies("filters"))
print("Number of non-existant: %d" % pxm.GetNumberOfProxies("non-existant"))
if pxm.GetNumberOfProxies("sources") != 3:
  Error("Number of proxies in \"sources\" group reported incorrect.");

if pxm.GetNumberOfProxies("filters") != 4:
  Error("Number of proxies in \"filters\" group reported incorrect.");

if pxm.GetNumberOfProxies("non-existant") != 0:
  Error("Number of proxies in \"non-existant\" group reported incorrect.");

print("\nProxies under filters.s2")
for proxy in pxm.GetProxies("filters","s2"):
  print(proxy.GetXMLName())
