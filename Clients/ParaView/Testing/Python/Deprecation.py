# /usr/bin/env python
from paraview.simple import *
from paraview.simple.deprecated import DeprecationError, TestDeprecationError
from contextlib import suppress


# -----------------------------------------------------------------------------
# This test aims to run all the deprecated function from simple.
# Some of those test will need to move to an expected Exception after 2 release.
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# Warnings
# -----------------------------------------------------------------------------

# MakeBlueToRedLT
lut = MakeBlueToRedLT(0, 1)

# SetProperties
c = Cone()
assert c.Radius == 0.5
SetProperties(c, Radius=5)
assert c.Radius == 5

# GetProperty
ps1 = GetProperty(proxy=c, name="Radius")
ps2 = GetProperty(c, "Radius")
ps3 = GetProperty("Radius")
ps4 = GetProperty(name="Radius")
assert ps1 is ps2 and ps3 is ps4 and ps1 is ps4

# GetRepresentationProperty
r = Show(c)
pr1 = GetRepresentationProperty(proxy=r, name="Visibility")
pr2 = GetRepresentationProperty(r, "Visibility")
pr3 = GetRepresentationProperty("Visibility")
pr4 = GetRepresentationProperty(name="Visibility")
assert pr1 is pr2 and pr3 is pr4 and pr1 is pr4

# GetDisplayProperty
pd1 = GetDisplayProperty(proxy=r, name="Visibility")
pd2 = GetDisplayProperty(r, "Visibility")
pd3 = GetDisplayProperty("Visibility")
pd4 = GetDisplayProperty(name="Visibility")
assert pd1 is pd2 and pd3 is pd4 and pd1 is pd4

# GetViewProperties
active_view = GetViewProperties()

# GetViewProperty
pv1 = GetViewProperty(proxy=active_view, name="Visibility")
pv2 = GetViewProperty(active_view, "Visibility")
pv3 = GetViewProperty("Visibility")
pv4 = GetViewProperty(name="Visibility")
assert pv1 is pv2 and pv3 is pv4 and pv1 is pv4

# AssignLookupTable
# TODO

# GetLookupTableNames
old_way = GetLookupTableNames()
new_way = ListColorPresetNames()
assert old_way == new_way

# LoadLookupTable
# TODO

# -----------------------------------------------------------------------------
# Error
# -----------------------------------------------------------------------------

with suppress(DeprecationError):
    TestDeprecationError()
    assert False, "Should not reach that point"
