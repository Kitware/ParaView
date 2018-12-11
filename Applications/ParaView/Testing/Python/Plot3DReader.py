#/usr/bin/env python
from paraview.simple import *
import sys

def isEqual(l1, l2):
    if (len(l1) != len(l2)):
        return False
    for i in range(len(l1)):
        if (abs(l1[i] - l2[i]) > 1e-5):
            return False
    return True

def checkPointScalar(data, variables, expected):
    if (len(variables) != len(expected)):
        return False
    for i in range(len(variables)):
        var = variables[i]
        varRange = data.PointData[var].GetRange()
        print(var + ': ' + str(varRange))
        if not isEqual(varRange, expected[i]):
            print('Incorrect range of ' + var + '! Expected ' + str(expected[i]))
            return False
    return True

def checkPointVector(data, variables, expected):
    if (len(variables) != len(expected)):
        return False
    for i in range(len(variables)):
        var = variables[i]
        for j in range(3):
            varRange = data.PointData[var].GetRange(j)
            print(var + '[' + str(j) + ']: ' + str(varRange))
            if not isEqual(varRange, expected[i][j]):
                print('Incorrect range of ' + var + '[' + str(j) + ']! Expected ' + str(expected[i][j]))
                return False
    return True

# Read the file and compute all the available variables
# Note the Entropy, MachNumber and PressureCoefficient will not be checked because they are infinity in this test case
for i, arg in enumerate(sys.argv):
    if arg == "-D" and i+1 < len(sys.argv):
        fname = sys.argv[i+1] + '/Testing/Data/combxyz.bin'
        qname = sys.argv[i+1] + '/Testing/Data/comb.q'

combxyzbin = PLOT3DReader(QFileName=qname, FileName=fname, FunctionFileName='')
combxyzbin.Functions = [110, 111, 112, 113, 120, 130, 140, 144, 153, 170, 184, 200, 201, 210, 211, 212]

# Check the point scalars
scalars = ['Density', 'Enthalpy', 'KineticEnergy', 'Pressure', 'SoundSpeed', 'StagnationEnergy',
    'Swirl', 'Temperature', 'VelocityMagnitude', 'VorticityMagnitude']
scalarRanges = [(0.19781309366226196, 0.710419237613678), (-1636349.625, 0.0), (0.0, 1168821.125),
                (-118486.359375, 0.0), (0.0, 0.0), (0.0, 0.0), (-2069.485107421875, 83.2387924194336),
                (-467528.4375, 0.0), (0.0, 1528.93505859375), (0.0, 9402.171875)]
if not checkPointScalar(combxyzbin, scalars, scalarRanges):
    raise RuntimeError('Incorrect range of scalar variables')

# Check the point vectors
vectors = ['Momentum', 'PressureGradient', 'StrainRate', 'Velocity', 'Vorticity']
vectorRanges = [((-368.5411682128906, 368.3779602050781), (-392.2895812988281, 380.3102111816406), (-287.27032470703125, 297.8510437011719)),
                ((-191192.78125, 492975.34375), (-117900.875, 110535.109375), (-645498.5, 335471.0)),
                ((-5366.2587890625, 2693.397705078125), (-1097.124267578125, 1348.7108154296875), (-1589.5201416015625, 1779.9736328125)),
                ((-576.0213012695312, 1423.640869140625), (-757.9921875, 988.3786010742188), (-470.63970947265625, 642.0108642578125)),
                ((-4280.72265625, 2005.639892578125), (-5797.0546875, 9061.9541015625), (-4268.16845703125, 1990.39990234375))]
if not checkPointVector(combxyzbin, vectors, vectorRanges):
    raise RuntimeError('Incorrect range of vector variables')

# Check the field scalar
properties = combxyzbin.FieldData['Properties'].GetRange()
propertiesRange = (0.0, 1.399999976158142)
print('Properties: ' + str(properties))
if not isEqual(properties, propertiesRange):
    raise RuntimeError('Incorrect range of Properties! Expected ' + str(propertiesRange))
