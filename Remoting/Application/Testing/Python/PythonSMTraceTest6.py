# This test verifies that, when possible, RGBPoints for lookup tables are
# created using the `GenerateRGBPoints()` function, which can be significantly
# less verbose in state files than writing out every RGB point.

import math
from typing import List

from paraview.simple import *
from paraview import smstate

# Make a wavelet with a contour, which will dump RGBPoints in
# the state file.
wavelet = Wavelet()
contour = Contour(wavelet)
contour.Set(
    ContourBy=['POINTS', 'RTData'],
    Isosurfaces=[157],
)
Show(contour)

lut = GetColorTransferFunction('RTData')


def get_kwargs(state_string: str) -> dict:
    # This is a utility function for getting the kwargs for
    # `GenerateRGBPoints()` as a dict.
    # If `GenerateRGBPoints()` is not present, or if it is called
    # more than one time, `None` is returned.
    search_str = 'RGBPoints=GenerateRGBPoints('
    num_occurrences = state_string.count(search_str)
    if num_occurrences != 1:
        return None

    # Find the start index of the kwargs
    start_idx = state_string.find(search_str) + len(search_str)
    # Find the next '),' and assume it is the end of the kwargs
    end_idx = state_string.find('),', start_idx)

    # Extract the section of code
    section = state_string[start_idx:end_idx]

    # Convert the arguments into a kwargs dictionary
    return eval(f'dict({section})')


def are_close(array1: List[float], array2: List[float]) -> bool:
    # Compare two lists of floats to see if they are very close
    if len(array1) != len(array2):
        return False

    for v1, v2 in zip(array1, array2):
        if not math.isclose(v1, v2):
            return False

    return True


state_string = smstate.get_state()
kwargs = get_kwargs(state_string)
assert kwargs and 'range_min' in kwargs and 'range_max' in kwargs

# Generate the RGBPoints using the same kwargs to verify we reproduced
# the RGBPoints on the LUT.
rgb_points = GenerateRGBPoints(**kwargs)
assert are_close(rgb_points, lut.RGBPoints)

# In the past, applying Viridis would produce RGBPoints that were ~250 lines
# long. Verify that we now use `GenerateRGBPoints()` instead.
preset_name = 'Viridis (matplotlib)'
lut.SMProxy.ApplyPreset(preset_name)

# The LUT points should no longer match the old rgb_points
assert not are_close(rgb_points, lut.RGBPoints)

state_string = smstate.get_state()
kwargs = get_kwargs(state_string)
assert (
    kwargs and
    'preset_name' in kwargs and
    'range_min' in kwargs and
    'range_max' in kwargs
)
assert kwargs['preset_name'] == preset_name

rgb_points = GenerateRGBPoints(**kwargs)
assert are_close(rgb_points, lut.RGBPoints)

# Now also rescale the colormap to a specific range, and verify
# that the kwargs show that range.
new_range = [51, 199]
lut.SMProxy.RescaleTransferFunction(new_range)

# The LUT points should no longer match the old rgb_points
assert not are_close(rgb_points, lut.RGBPoints)

state_string = smstate.get_state()
kwargs = get_kwargs(state_string)
assert (
    kwargs and
    'preset_name' in kwargs and
    'range_min' in kwargs and
    'range_max' in kwargs
)
assert kwargs['preset_name'] == preset_name
assert math.isclose(kwargs['range_min'], new_range[0])
assert math.isclose(kwargs['range_max'], new_range[1])

rgb_points = GenerateRGBPoints(**kwargs)
assert are_close(rgb_points, lut.RGBPoints)

# Now manually adjust one of the points, and verify that we instead
# dump out the whole RGBPoints
lut.RGBPoints[4] += 0.01
lut.SMProxy.UpdateVTKObjects()

state_string = smstate.get_state()
kwargs = get_kwargs(state_string)
assert kwargs is None

assert 'RGBPoints=[' in state_string
assert 'GenerateRGBPoints(' not in state_string
