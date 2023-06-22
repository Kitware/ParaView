import paraview
from paraview.simple import *

# This script tests backwards compatibility for the Slice filter.
# Replaces MergePoints property with Locator one

paraview.compatibility.major = 5
paraview.compatibility.minor = 11

wavelet = Wavelet()

sliceFilter = Slice(Input=wavelet)
assert(sliceFilter.MergePoints == 1)
assert(sliceFilter.GetProperty("Locator").GetData().SMProxy.GetXMLLabel() == "Uniform Binning")

sliceFilter.MergePoints = 0
assert(sliceFilter.MergePoints == 0)
assert(sliceFilter.GetProperty("Locator").GetData().SMProxy.GetXMLLabel() == "Not Merging Points")
