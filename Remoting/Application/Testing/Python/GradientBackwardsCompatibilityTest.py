import paraview
from paraview.simple import *

# This script tests backwards compatibility for the Gradient filter.
# The new 'Gradient' filter replaces and merges the previous
# 'Gradient' and 'GradientOfUnstructuredDataSet' filters.
# The previous 'Gradient' filter has been marked as 'Legacy'.

# Create a wavelet source
wavelet = Wavelet()

# Check that the 'Gradient' filter is created
gradFilter = Gradient(Input=wavelet)
assert (gradFilter.GetXMLName() == "Gradient"), ("'Gradient' should have been created.")

# Move to older version and check compatibility
paraview.compatibility.major = 5
paraview.compatibility.minor = 9

# Check that the 'GradientLegacy' filter is created
gradFilter = Gradient(Input=wavelet)
assert (gradFilter.GetXMLName() == "GradientLegacy"), ("'Gradient' should have been changed to 'GradientLegacy.")
