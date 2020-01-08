#/usr/bin/env python
import sys
#import QtTesting
#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'Unstructured Cell Types'
unstructuredCellTypes1 = UnstructuredCellTypes()
# create a new 'Python Calculator'
pythonCalculator1 = PythonCalculator(Input=unstructuredCellTypes1)
pythonCalculator1.Expression = 'volume(inputs[0])'

pythonCalculator1.UpdatePipeline()
arrayRange = pythonCalculator1.CellData['result'].GetRange()
if arrayRange[0] < .999 or arrayRange[0] > 1.001 or \
   arrayRange[1] < .999 or arrayRange[1] > 1.001:
    print("ERROR: incorrect volume computed for UnstructuredCellTypes source")
    sys.exit(1)

# create a new 'ExodusIIReader'
for i, arg in enumerate(sys.argv):
    if arg == "-D" and i+1 < len(sys.argv):
        dataFile = sys.argv[i+1] + '/Testing/Data/can.ex2'

canex2 = ExodusIIReader(FileName=[dataFile])
canex2.ElementVariables = []
canex2.PointVariables = []
canex2.GlobalVariables = []
canex2.NodeSetArrayStatus = []
canex2.SideSetArrayStatus = []

# get animation scene
animationScene1 = GetAnimationScene()

# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

# Properties modified on canex2
canex2.ElementBlocks = ['Unnamed block ID: 1 Type: HEX', 'Unnamed block ID: 2 Type: HEX']

# create a new 'Python Calculator'
pythonCalculator2 = PythonCalculator(Input=canex2)
pythonCalculator2.Expression = 'volume(inputs[0])'

pythonCalculator2.UpdatePipeline(0.0)
arrayRange = pythonCalculator2.CellData['result'].GetRange()
if arrayRange[0] < .0098 or arrayRange[0] > .0099 or \
   arrayRange[1] < .13 or arrayRange[1] > .14:
    print("ERROR: incorrect volume computed for can.ex2")
    sys.exit(1)
print("success")
