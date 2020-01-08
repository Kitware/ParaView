# There were issues with D3 and/or Python calculator which
# caused the following to generated cells with negative volumes in ParaView 5.2.
# It has been since fixed. This test will ensure it stays fixed.
# Ref: Helpdesk Ticket #KW00001698.

from paraview.simple import *

# create a new 'Wavelet'
wavelet1 = Wavelet()
wavelet1.WholeExtent = [-20, 20, -20, 20, -20, 20]

# create a new 'Clip'
clip1 = Clip(Input=wavelet1)
clip1.ClipType = 'Plane'
clip1.Scalars = ['POINTS', 'RTData']
clip1.Value = 157.0909652709961
clip1.Invert = 1

# init the 'Plane' selected for 'ClipType'
clip1.ClipType.Origin = [1.0, 0.0, 0.0]
clip1.ClipType.Normal = [1.0, 2.0, 4.0]

# create a new 'D3'
d31 = D3(Input=clip1)

calculator = PythonCalculator(Expression="volume(inputs[0])", ArrayAssociation="Cell Data", CopyArrays=0, ArrayName="cellVolume")
assert calculator.CellData["cellVolume"].GetRange(0)[0] >= 0
