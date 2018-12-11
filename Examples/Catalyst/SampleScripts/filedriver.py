r"""
This script is meant to act as a Catalyst instrumented simulation code.
Instead of actually computing values though it just reads in files that
are passed in through the command line and sets that up to act like
it's coming in as simulation computed values. The script must be run
with pvbatch with the following arguments:
* -sym if run with more than a single MPI process so that pvbatch will
       have all processes run the script.
* a list of files in quotes (wildcards are acceptable) but these are
  treated as a single argument to the script.
* a list of Catalyst Python scripts.

An example of how to use this script would be:
mpirun -np 5 <path>/pvbatch -sym "input_*.pvtu" makeanimage.py makeaslice.py

This script currently only handles a single channel. It will try to find
an appropriate reader for the list of filenames and loop through the timesteps.
It attempts to sort the filenames as well, first by name length and second
alphabetically.
"""

# for Python2 print statmements to output like Python3 print statements
from __future__ import print_function
import sys
import math
import glob

# initialize and read input parameters
import paraview
paraview.options.batch = True
paraview.options.symmetric = True

import paraview.simple as pvsimple
from paraview.modules import vtkPVCatalyst, vtkPVCatalystPython, vtkPVPythonCatalystPython
pm = pvsimple.servermanager.vtkProcessModule.GetProcessModule()
rank = pm.GetPartitionId()
nranks = pm.GetNumberOfLocalPartitions()

if len(sys.argv) < 2:
    if rank == 0:
        print("ERROR: must pass in a set of files to read in")
    sys.exit(1)

files = glob.glob(sys.argv[1])

# In case the filenames aren't padded we sort first by shorter length and then
# alphabetically. This is a slight modification based on the question by Adrian and answer by
# Jochen Ritzel at:
# https://stackoverflow.com/questions/4659524/how-to-sort-by-length-of-string-followed-by-alphabetical-order
files.sort(key=lambda item: (len(item), item))
if rank == 0:
    print("Reading in ", files)
reader = pvsimple.OpenDataFile(files)

if pm.GetSymmetricMPIMode() == False and nranks > 1:
    if rank == 0:
        print("ERROR: must run pvbatch with -sym when running with more than a single MPI process")
    sys.exit(1)

catalyst = vtkPVCatalyst.vtkCPProcessor()
# We don't need to initialize Catalyst since we run from pvbatch
# with the -sym argument which acts exactly like we're running
# Catalyst from a simulation code.
#catalyst.Initialize()

for script in sys.argv[2:]:
    pipeline = vtkPVPythonCatalystPython.vtkCPPythonScriptPipeline()
    if rank == 0:
        print("Adding script ", script)
    pipeline.Initialize(script)
    catalyst.AddPipeline(pipeline)


# we get the channel name here from the reader's dataset. if there
# isn't a channel name there we just assume that the channel name
# is 'input' since that's the convention for a single input
reader.UpdatePipeline()
dataset = pvsimple.servermanager.Fetch(reader)
array = dataset.GetFieldData().GetArray(catalyst.GetInputArrayName())
if array:
    channelname = array.GetValue(0)
else:
    channelname = 'input'

if rank == 0:
    print("The channel name is ", channelname)

if hasattr(reader, "TimestepValues"):
    timesteps = reader.TimestepValues
    if not timesteps:
	timesteps = [0]
else:
    timesteps = [0]

step = -1
for time in timesteps:
    step = step + 1
    datadescription = vtkPVCatalyst.vtkCPDataDescription()
    datadescription.SetTimeData(time, step)
    datadescription.AddInput(channelname)
    if time == timesteps[-1]:
         # last time step so we force the output
        datadescription.ForceOutputOn()

    retval = catalyst.RequestDataDescription(datadescription)

    if retval == 1:
        reader.UpdatePipeline(time)
        dataset = pvsimple.servermanager.Fetch(reader)
        inputdescription = datadescription.GetInputDescriptionByName(channelname)
        inputdescription.SetGrid(dataset)
        if dataset.IsA("vtkImageData") == True or dataset.IsA("vtkRectilinearGrid") == True \
           or dataset.IsA("vtkStructuredGrid") == True:
            from mpi4py import MPI
            extent = dataset.GetExtent()
            wholeextent = [extent[0], -extent[1], extent[2], -extent[3], extent[4], -extent[5]]
            MPI.COMM_WORLD.allreduce(wholeextent, op=MPI.MIN)
            for i in range(3):
                wholeextent[2*i] = -wholeextent[2*i]

            inputdescription.SetWholeExtent(wholeextent)

        catalyst.CoProcess(datadescription)
