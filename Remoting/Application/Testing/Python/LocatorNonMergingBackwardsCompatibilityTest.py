import paraview
from paraview.simple import *

# This script tests backwards compatibility for the proxy property Locator.
# That should accept "Don't Merge Points" as a value

paraview.compatibility.major = 5
paraview.compatibility.minor = 11

wavelet = Wavelet()

sliceFilter = Slice(Input=wavelet)
sliceFilter.PointMergeMethod = "Don't Merge Points"
