from paraview.simple import *
import sys
from paraview import smtesting
import os
import shutil

smtesting.ProcessCommandLineArguments()
canex2 = ExodusIIReader(FileName=smtesting.DataDir+'/Testing/Data/can.ex2')
canex2.NodeSetArrayStatus = []
canex2.SideSetArrayStatus = []

# Properties modified on canex2
canex2.ElementBlocks = ['Unnamed block ID: 1 Type: HEX', 'Unnamed block ID: 2 Type: HEX']

# create a new 'Merge Blocks'
mergeBlocks1 = MergeBlocks(Input=canex2)

# save data with all time steps
path = smtesting.TempDir+'/PVDWriter/'
if os.path.exists(path):
    shutil.rmtree(path)
fileName = path+'PVDWriter.pvd'
SaveData(fileName, proxy=mergeBlocks1, WriteTimeSteps=1)

# read the newly created file to see if we have the same number of time steps
pvdreader = PVDReader(FileName=fileName)
pvdreader.UpdatePipeline();

if len(pvdreader.TimestepValues) != len(canex2.TimestepValues):
    print("Failed to properly process all of the time steps from the input file (can.ex2)")
    sys.exit(1)


# save data with a single time step
shutil.rmtree(path)
fileName = path+'PVDWriterSingleTimeStep.pvd'
SaveData(fileName, proxy=mergeBlocks1, WriteTimeSteps=0)

# read the newly created file to see if we have just a single time step
pvdreaderSingleTimeStep = PVDReader(FileName=fileName)
pvdreaderSingleTimeStep.UpdatePipeline();
if len(pvdreaderSingleTimeStep.TimestepValues) > 0:
    print("Failed to properly write only a single time step")
    sys.exit(1)

print("success")
