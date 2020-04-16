#### import the simple module from the paraview
from paraview.simple import *
from paraview import smtesting

import os
import os.path

#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

smtesting.ProcessCommandLineArguments()

file1 = os.path.join(smtesting.DataDir, "Testing/Data/mg_diff_0000.vtm")
file2 = os.path.join(smtesting.DataDir, "Testing/Data/mg_diff_0062.vtm")


# create a new 'XML MultiBlock Data Reader'
mg_diff_0 = XMLMultiBlockDataReader(FileName=[file1, file2])
mg_diff_0.CellArrayStatus = ['rho', 'grd', 'mat', 'prs', 'tev', 'vel']
mg_diff_0.TimeArray = 'Default'

# get animation scene
animationScene1 = GetAnimationScene()

# get the time-keeper
timeKeeper1 = GetTimeKeeper()

# update animation scene based on data timesteps
animationScene1.UpdateAnimationUsingDataTimeSteps()

mg_diff_0.UpdatePipelineInformation()

if mg_diff_0.TimestepValues[0] != 0.0 or mg_diff_0.TimestepValues[1] != 1.0:
    raise RuntimeError('Incorect default time value')

# Properties modified on mg_diff_0
mg_diff_0.TimeArray = 'T'

mg_diff_0.UpdatePipelineInformation()

if mg_diff_0.TimestepValues[0] != 0.0 or mg_diff_0.TimestepValues[1] != 0.00010724659762818111:
    raise RuntimeError('Incorect read time value')
