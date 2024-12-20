# Tests a few of the lut API provided by paraview.simple

from paraview.simple import *

w = Wavelet()
UpdatePipeline()

assert AssignFieldToColorPreset("RTData", "Cool to Warm") == True

try:
    AssignFieldToColorPreset("RTData", "--non-existent--")

    # this should never happen!
    assert False
except RuntimeError:
    pass

assert len(ListColorPresetNames()) > 0
