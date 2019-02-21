#/usr/bin/env python
from paraview.simple import *
import sys

wavelet1 = Wavelet()

wavelet2 = Wavelet()
pythonCalculator1 = PythonCalculator(Input=wavelet2)
pythonCalculator1.ArrayName = 'RTData'
pythonCalculator1.Expression = 'RTData+200'
pythonCalculator1.CopyArrays = 0

# this one should be ignored in the output since it has a different
# amount of points and cells than the first one
sphereSource = Sphere()

appendAttributes1 = AppendAttributes(Input=[wavelet1, sphereSource, pythonCalculator1])
appendAttributes1.UpdatePipeline()

if appendAttributes1.PointData.GetNumberOfArrays() != 2:
    # should have RTData and RTData_input_1
    print("ERROR: wrong number of arrays ", appendAttributes1.PointData.GetNumberOfArrays())
    sys.exit(1)

arrayRange = appendAttributes1.PointData['RTData'].GetRange()
if arrayRange[0] < 37 or arrayRange[0] > 38 or arrayRange[1] < 276 or arrayRange[0] > 277:
    print("ERROR: RTData has wrong array range ", arrayRange)
    sys.exit(1)

arrayRange = appendAttributes1.PointData['RTData_input_2'].GetRange()
if arrayRange[0] < 237 or arrayRange[0] > 238 or arrayRange[1] < 476 or arrayRange[0] > 477:
    print("ERROR: RTData_input_2 has wrong array range ", arrayRange)
    sys.exit(1)

# now try with the can.ex2 exodus file for multiblock testing
for i, arg in enumerate(sys.argv):
    if arg == "-D" and i+1 < len(sys.argv):
        dataFile = sys.argv[i+1] + '/Testing/Data/can.ex2'

canex2 = ExodusIIReader(FileName=[dataFile])
canex2.ElementVariables = ['EQPS']
canex2.PointVariables = ['DISPL', 'VEL', 'ACCL']
canex2.GlobalVariables = ['KE', 'XMOM', 'YMOM', 'ZMOM', 'NSTEPS', 'TMSTEP']

calculator1 = Calculator(Input=canex2)
calculator1.AttributeType = 'Point Data'
calculator1.CoordinateResults = 0
calculator1.ResultNormals = 0
calculator1.ResultTCoords = 0
calculator1.ReplaceInvalidResults = 1
calculator1.ReplacementValue = 0.0
calculator1.ResultArrayName = 'VEL_X'
calculator1.Function = 'VEL_X+100'

appendAttributes2 = AppendAttributes(Input=[canex2, calculator1])
appendAttributes2.UpdatePipeline()
print("success")
