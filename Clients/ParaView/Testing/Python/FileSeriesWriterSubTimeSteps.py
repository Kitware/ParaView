import sys
from paraview.simple import *

dataFile = None
tempDir = None
for i, arg in enumerate(sys.argv):
    if arg == "-D" and i+1 < len(sys.argv):
        dataFile = sys.argv[i+1] + '/Testing/Data/can.ex2'
    elif arg == "-T" and i+1 < len(sys.argv):
        tempDir = sys.argv[i+1]

if dataFile == None:
    print("Must pass in data directory as '-D <datadir>'")
    sys.exit(1)

if tempDir == None:
    print("Must pass in temp directory as '-T <tempdir>'")
    sys.exit(1)

# create a new 'ExodusIIReader'
canex2 = ExodusIIReader(FileName=[dataFile])
canex2.GenerateObjectIdCellArray = 1
canex2.GenerateGlobalElementIdArray = 1
canex2.ElementVariables = []
canex2.FaceVariables = []
canex2.EdgeVariables = []
canex2.SideSetResultArrayStatus = []
canex2.NodeSetResultArrayStatus = []
canex2.FaceSetResultArrayStatus = []
canex2.EdgeSetResultArrayStatus = []
canex2.GenerateGlobalNodeIdArray = 1
canex2.ElementSetResultArrayStatus = []
canex2.PointVariables = []
canex2.GlobalVariables = []
canex2.ApplyDisplacements = 1
canex2.DisplacementMagnitude = 1.0
canex2.EdgeBlocks = []
canex2.NodeSetArrayStatus = []
canex2.SideSetArrayStatus = []
canex2.FaceSetArrayStatus = []
canex2.EdgeSetArrayStatus = []
canex2.ElementSetArrayStatus = []
canex2.NodeMapArrayStatus = []
canex2.EdgeMapArrayStatus = []
canex2.FaceMapArrayStatus = []
canex2.ElementMapArrayStatus = []
canex2.ElementBlocks = []
canex2.FaceBlocks = []
canex2.HasModeShapes = 0
canex2.ModeShape = 1
canex2.AnimateVibrations = 1
canex2.GenerateFileIdArray = 0

# Properties modified on canex2
canex2.ElementBlocks = ['Unnamed block ID: 1 Type: HEX', 'Unnamed block ID: 2 Type: HEX']

# create a new 'Slice'
slice1 = Slice(Input=canex2)
slice1.SliceType = 'Plane'
slice1.Crinkleslice = 0
slice1.Triangulatetheslice = 1
slice1.SliceOffsetValues = [0.0]

# init the 'Plane' selected for 'SliceType'
slice1.SliceType.Origin = [0.21706008911132812, 4.0, -5.110947132110596]
slice1.SliceType.Normal = [1.0, 0.0, 0.0]
slice1.SliceType.Offset = 0.0

# Properties modified on slice1.SliceType
slice1.SliceType.Origin = [0.217060089111328, 4.0, -5.1109471321106]

# Properties modified on slice1.SliceType
slice1.SliceType.Origin = [0.217060089111328, 4.0, -5.1109471321106]

# save data
from os.path import join
SaveData(join(tempDir,'canslices.vtm'), proxy=slice1, Writetimestepsasfileseries=1,
    Firsttimestep=10,
    Lasttimestep=20,
    Timestepstride=3,
    Filenamesuffix='_%d',
    DataMode='Appended',
    HeaderType='UInt64',
    EncodeAppendedData=0,
    CompressorType='None')

# create a new 'XML MultiBlock Data Reader'
canslices = XMLMultiBlockDataReader(FileName=[join(tempDir, 'canslices_10.vtm'),
                                              join(tempDir, 'canslices_13.vtm'),
                                              join(tempDir, 'canslices_16.vtm'),
                                              join(tempDir, 'canslices_19.vtm')])

canslices.UpdatePipeline()
if len(canslices.TimestepValues) != 4:
    print("ERROR: error writing out files and reading them back in")
    sys.exit(1)

# list of files that should not have been written out
otherFileNames = [join(tempDir, 'canslices_7.vtm'),
                  join(tempDir, 'canslices_8.vtm'),
                  join(tempDir, 'canslices_9.vtm'),
                  join(tempDir+'canslices_11.vtm'),
                  join(tempDir+'canslices_12.vtm'),
                  join(tempDir+'canslices_20.vtm')]
import os.path
for f in otherFileNames:
    if os.path.isfile(f):
        print("ERROR: wrote file ", f, " which should not have been written out")
        sys.exit(1)

print("success")
