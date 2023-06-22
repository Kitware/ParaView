# Tests that vtkSMRangeDomainTemplate will only accept values within its range
from paraview import servermanager

servermanager.Connect()
sphere = servermanager.CreateProxy("sources", "SphereSource")

# start theta should be in [0,360]
p = sphere.GetProperty("StartTheta")

p.SetUncheckedElement(0, 0)
assert p.IsInDomains() == 1

p.SetUncheckedElement(0, 90)
assert p.IsInDomains() == 1

p.SetUncheckedElement(0, 180)
assert p.IsInDomains() == 1

p.SetUncheckedElement(0, 360)
assert p.IsInDomains() == 1

p.SetUncheckedElement(0, 361)
assert p.IsInDomains() == 0

p.SetUncheckedElement(0, -90)
assert p.IsInDomains() == 0

p.SetUncheckedElement(0, -180)
assert p.IsInDomains() == 0

# Radius is greater or equal to 0
p = sphere.GetProperty("Radius")

p.SetUncheckedElement(0, 0)
assert p.IsInDomains() == 1

p.SetUncheckedElement(0, 1.0)
assert p.IsInDomains() == 1

p.SetUncheckedElement(0, -1.0)
assert p.IsInDomains() == 0
