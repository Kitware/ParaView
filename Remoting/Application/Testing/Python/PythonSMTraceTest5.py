# This test verifies that default proxy properties do not show up in the trace.
#
# For this test, we will use the clip's ClipType, which is a proxy property.
#
# If the ClipType is set to `Plane`, which is the default, it should not
# appear in the trace. If a property of the Plane is modified, however, that
# modification should still appear in the trace.
#
# If a clip's ClipType is set to `Sphere` instead, which is not the default,
# it should appear in the trace.
#
# Unfortunately, I had to use `smstate.get_state()` for this test instead of
# a regular Python trace, because we want to verify that modifications to
# default proxy properties still show up in the trace, and you cannot track
# changes to proxies after they are initialized in a regular trace due to this
# issue:
# https://gitlab.kitware.com/paraview/paraview/-/issues/22940

from paraview.simple import *
from paraview import smstate

wavelet = Wavelet()

# Make one clip plane, and one clip sphere
clip_plane = Clip(wavelet)
clip_plane.Scalars = ['POINTS', 'RTData']
clip_plane.ClipType.Offset = 1

clip_sphere = Clip(wavelet)
clip_sphere.Scalars = ['POINTS', 'RTData']
clip_sphere.ClipType = 'Sphere'

# Show these so we can test changes to rendering state features too
Show(clip_plane)
Show(clip_sphere)

state_string = smstate.get_state()

# For `clip_plane`, the ClipType should not be specified, since it
# is the default for the proxy property
assert "ClipType='Plane'" not in state_string.replace(' ', '')

# Even though the clip type is not specified, the modification of
# a property on the current clip type should still appear
assert 'ClipType.Offset = 1' in state_string

# For `clip_sphere`, the ClipType should show up, since it is not
# the default
assert "ClipType='Sphere'" in state_string.replace(' ', '')

# 2D transfer functions used to always show up in state files, even
# when they were not used at all. The recent changes to proxy properties
# also allowed us to remove 2D transfer functions from state files when
# they are not used. So verify that those are absent as well.
assert "TransferFunction2D" not in state_string
assert "TF2D" not in state_string
