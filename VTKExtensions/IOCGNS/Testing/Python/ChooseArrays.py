from paraview.simple import *
from paraview.vtk.util.misc import vtkGetDataRoot, vtkGetTempDir
from os.path import join
import sys

datadir = join(vtkGetDataRoot(), "Testing/Data")

cgnsFile = join(datadir, "channelBump_solution.cgns")

reader = CGNSSeriesReader(FileNames=[cgnsFile])
# load 4 fields on all bases
reader.Bases = ["Base_Volume_elements", "Base_Surface_elements"]
reader.CellArrayStatus = ["Velocity", "Pressure", "AirVolumeFraction", "Vorticity"]
reader.UpdatePipeline()
if reader.CellData.NumberOfArrays != len(reader.CellArrayStatus):
    raise ValueError('expected number of arrays not found in original dataset.')

writer = CGNSWriter(Input=reader)
# activate ChooseArrays and select two arrays to write
writer.ChooseArraysToWrite = 1
writer.CellDataArrays = ["Velocity", "Pressure"]

target = join(vtkGetTempDir(), "channelBump_velopres.cgns")
writer.FileName = target
writer.UpdatePipeline()

reader = CGNSSeriesReader(FileNames=[target])
# get the information. This should contain ONLY the requested arrays
info = reader.GetProperty('CellArrayInfo')
if len(info) != 4: # CellArrayInfo is an array of [ name1, status1-int, name2, status2-int, ..., nameN, statusN-int]
    raise ValueError('expected to find 2 arrays, found '+str(info))

if not all([x in info for x in writer.CellDataArrays]):
    raise ValueError('expected to find Velocity and Pressure, found '+str(info))

# load the 2 fields on all bases
reader.Bases = ["Base_Volume_Elements", "Base_Surface_Elements"]
reader.CellArrayStatus =  ["Velocity", "Pressure"]
reader.UpdatePipeline()

# ensure that the data is there
if reader.CellData.NumberOfArrays != len(reader.CellArrayStatus):
    raise ValueError('expected number of arrays not found in written dataset.')
