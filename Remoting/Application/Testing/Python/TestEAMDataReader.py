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
reader.a2DVariables = ['ps']
reader.a3DMiddleLayerVariables = ['T_mid']
reader.a3DInterfaceLayerVariables = ['p_int']

# update the pipeline
reader.UpdatePipeline()

# Test Output 0: 2D surface data
# Expected: 384 quad cells, 1536 points (384 cells * 4 corners)
output2D = servermanager.Fetch(reader, idx=0)
npts_2d = output2D.GetNumberOfPoints()
ncells_2d = output2D.GetNumberOfCells()

if ncells_2d != 384:
    print(f"ERROR: 2D output has {ncells_2d} cells, expected 384")
    sys.exit(1)

if npts_2d != 1536:
    print(f"ERROR: 2D output has {npts_2d} points, expected 1536")
    sys.exit(1)

# Verify 2D output has the enabled array
if output2D.GetCellData().GetArray("ps") is None:
    print("ERROR: 2D output missing 'ps' cell data array")
    sys.exit(1)

print(f"2D output: {ncells_2d} cells, {npts_2d} points - OK")

# Test Output 1: 3D middle layer (lev) data
# Expected: 384 * 71 = 27264 hex cells, 72 * 1536 = 110592 points
output3Dm = servermanager.Fetch(reader, idx=1)
npts_3dm = output3Dm.GetNumberOfPoints()
ncells_3dm = output3Dm.GetNumberOfCells()

if ncells_3dm != 27264:
    print(f"ERROR: 3D-lev output has {ncells_3dm} cells, expected 27264")
    sys.exit(1)

if npts_3dm != 110592:
    print(f"ERROR: 3D-lev output has {npts_3dm} points, expected 110592")
    sys.exit(1)

# Verify 3D-lev output has the enabled array
if output3Dm.GetCellData().GetArray("T_mid") is None:
    print("ERROR: 3D-lev output missing 'T_mid' cell data array")
    sys.exit(1)

print(f"3D-lev output: {ncells_3dm} cells, {npts_3dm} points - OK")

# Test Output 2: 3D interface layer (ilev) data
# Expected: 384 * 72 = 27648 hex cells, 73 * 1536 = 112128 points
output3Di = servermanager.Fetch(reader, idx=2)
npts_3di = output3Di.GetNumberOfPoints()
ncells_3di = output3Di.GetNumberOfCells()

if ncells_3di != 27648:
    print(f"ERROR: 3D-ilev output has {ncells_3di} cells, expected 27648")
    sys.exit(1)

if npts_3di != 112128:
    print(f"ERROR: 3D-ilev output has {npts_3di} points, expected 112128")
    sys.exit(1)

# Verify 3D-ilev output has the enabled array
if output3Di.GetCellData().GetArray("p_int") is None:
    print("ERROR: 3D-ilev output missing 'p_int' cell data array")
    sys.exit(1)

print(f"3D-ilev output: {ncells_3di} cells, {npts_3di} points - OK")

print("All EAMDataReader tests passed!")
