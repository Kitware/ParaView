# This test verifies that multiline strings get saved correctly in the trace.
#
# This test is motivated by Python state files previously breaking for
# multiline programmable filters
# See here: https://gitlab.kitware.com/paraview/paraview/-/issues/22989

from paraview.simple import *
from paraview import smstate

wavelet = Wavelet()

script = 'print("a")\nprint("b")'
filter = ProgrammableFilter(Script=script, Input=wavelet)

state_string = smstate.get_state()

# There should be no spaces before "print('b')"
assert ' print("b")' not in state_string

# This should be in the state string exactly
assert '"""print("a")\nprint("b")"""' in state_string
