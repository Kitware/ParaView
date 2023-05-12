from paraview.simple import *
from paraview.vtk.util.misc import vtkGetDataRoot, vtkGetTempDir
from os.path import join
from os import remove
import sys
import glob

datadir = join(vtkGetDataRoot(), "Testing/Data")

# load the well-known can ExodusII file
exFile = join(datadir, "can.ex2")
reader = ExodusIIReader(FileName=exFile)
reader.UpdatePipeline()

# write it to CGNS with all timesteps enabled
writer = CGNSWriter(Input=reader)
writer.WriteAllTimeSteps = 1
target = join(vtkGetTempDir(), "CGNSTest.cgns")
writer.FileName = target
writer.UpdatePipeline()

# ensure that each time step has a file
writtenFiles = glob.glob('./CGNSTest*.cgns')
if writtenFiles is None:
    raise ValueError('no files written')

if len(writtenFiles) != len(reader.TimestepValues):
    raise ValueError('number of files written ({0}) does not match the expected number ({1})'.format(len(writtenFiless), len(reader.TimestepValues)))

for file in writtenFiles:
    remove(file)
