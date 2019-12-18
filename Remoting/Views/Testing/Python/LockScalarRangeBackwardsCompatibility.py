from paraview.simple import *
import paraview

# This tests backwards compatibility for the transition from LockScalarRange
# to AutomaticRescale

lut = GetColorTransferFunction("Normals")
try:
    lsr = lut.LockScalarRange
except paraview.NotSupportedException:
    pass
else:
    raise RuntimeError("Accessing 'LockScalarRange' must have raised an exception")

# Switch backwards compatibility to 5.4
paraview.compatibility.major = 5
paraview.compatibility.minor = 4

lsr = lut.LockScalarRange
assert (type(lsr) == int), "'LockScalarRange' must return an int"

# Locking the scalar range corresponds to never automatically updating it
lut.LockScalarRange = 1
from paraview.modules.vtkRemotingSettings import vtkPVGeneralSettings
assert (lut.AutomaticRescaleRangeMode == 'Never')

# Unlocking the scalar range corresponds to setting the reset mode to the global
# setting
lut.LockScalarRange = 0
pxm = lut.SMProxy.GetSessionProxyManager()
settingsProxy = pxm.GetProxy("settings", "GeneralSettings")

# Compare the integer value. Evaluating lut.AutomaticRescaleRangeMode yields
# the enumeration string instead of the integer value, so we get it via
# GetProperty().
settingsMode = settingsProxy.GetProperty("TransferFunctionResetMode").GetElement(0)
lutMode = lut.GetProperty('AutomaticRescaleRangeMode').SMProperty.GetElement(0)
assert (lutMode == settingsMode)
