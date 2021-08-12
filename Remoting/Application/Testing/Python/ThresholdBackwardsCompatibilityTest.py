import paraview
from paraview.simple import *

# This script tests backwards compatibility for the Threshold filter.

# Create a wavelet source
wavelet = Wavelet()

# Set threshold filter
thresholdFilter = Threshold(Input=wavelet)
thresholdFilter.UpdatePipeline()
thresholdFilter.Scalars = ['POINTS', 'RTData']

# Verify that exception is raised
try:
    thresholdFilter.ThresholdRange = [100.0, 200.0]
except paraview.NotSupportedException:
    pass
else:
    raise RuntimeError("NotSupportedException should have been thrown.")

# Move to older version and check compatibility
paraview.compatibility.major = 5
paraview.compatibility.minor = 9

thresholdFilter.ThresholdRange = [100.0, 200.0]
assert (thresholdFilter.LowerThreshold == 100.0), "'LowerThreshold' was not set correctly."
assert (thresholdFilter.UpperThreshold == 200.0), "'UpperThreshold' was not set correctly."
assert (thresholdFilter.ThresholdMethod == "Between"), "'ThresholdMethod' was not set correctly."
