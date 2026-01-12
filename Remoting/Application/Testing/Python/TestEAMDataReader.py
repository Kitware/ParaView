# Test the EAMDataReader for E3SM/EAM atmospheric data

from paraview import smtesting
from paraview import servermanager
from paraview.simple import EAMDataReader

import os
import sys

smtesting.ProcessCommandLineArguments()

# Test data files
dataFile = os.path.join(smtesting.DataDir, "Testing/Data/EAM/EAMxx_ne4pg2_202407.nc")
connFile = os.path.join(smtesting.DataDir, "Testing/Data/EAM/connectivity_ne4pg2_TEMPEST.scrip.nc")

# create a new 'EAM Data Reader'
reader = EAMDataReader(registrationName='EAMxx_ne4pg2_202407.nc',
                       DataFile=dataFile,
                       ConnectivityFile=connFile)

# Properties modified on reader
reader.SurfaceVariables = ['ps']
reader.MiddleLayerVariables = ['T_mid']
reader.InterfaceLayerVariables = ['p_int']

# update the pipeline
reader.UpdatePipeline()

# Test Output 0: Surface data
# Expected: 384 quad cells, 1536 points (384 cells * 4 corners)
outputSurface = servermanager.Fetch(reader, idx=0)
npts_surface = outputSurface.GetNumberOfPoints()
ncells_surface = outputSurface.GetNumberOfCells()

if ncells_surface != 384:
    print(f"ERROR: Surface output has {ncells_surface} cells, expected 384")
    sys.exit(1)

if npts_surface != 1536:
    print(f"ERROR: Surface output has {npts_surface} points, expected 1536")
    sys.exit(1)

# Verify surface output has the enabled array
if outputSurface.GetCellData().GetArray("ps") is None:
    print("ERROR: Surface output missing 'ps' cell data array")
    sys.exit(1)

print(f"Surface output: {ncells_surface} cells, {npts_surface} points - OK")

# Test Output 1: Middle layer (lev) data
# Expected: 384 * 71 = 27264 hex cells, 72 * 1536 = 110592 points
outputMiddle = servermanager.Fetch(reader, idx=1)
npts_middle = outputMiddle.GetNumberOfPoints()
ncells_middle = outputMiddle.GetNumberOfCells()

if ncells_middle != 27264:
    print(f"ERROR: Middle layer output has {ncells_middle} cells, expected 27264")
    sys.exit(1)

if npts_middle != 110592:
    print(f"ERROR: Middle layer output has {npts_middle} points, expected 110592")
    sys.exit(1)

# Verify middle layer output has the enabled array
if outputMiddle.GetCellData().GetArray("T_mid") is None:
    print("ERROR: Middle layer output missing 'T_mid' cell data array")
    sys.exit(1)

print(f"Middle layer output: {ncells_middle} cells, {npts_middle} points - OK")

# Test Output 2: Interface layer (ilev) data
# Expected: 384 * 72 = 27648 hex cells, 73 * 1536 = 112128 points
outputInterface = servermanager.Fetch(reader, idx=2)
npts_interface = outputInterface.GetNumberOfPoints()
ncells_interface = outputInterface.GetNumberOfCells()

if ncells_interface != 27648:
    print(f"ERROR: Interface layer output has {ncells_interface} cells, expected 27648")
    sys.exit(1)

if npts_interface != 112128:
    print(f"ERROR: Interface layer output has {npts_interface} points, expected 112128")
    sys.exit(1)

# Verify interface layer output has the enabled array
if outputInterface.GetCellData().GetArray("p_int") is None:
    print("ERROR: Interface layer output missing 'p_int' cell data array")
    sys.exit(1)

print(f"Interface layer output: {ncells_interface} cells, {npts_interface} points - OK")

print("All EAMDataReader tests passed!")
