import sys
if len(sys.argv) != 3:
    print("command is 'python <python driver code> <script name> <number of time steps>'")
    sys.exit(1)
import math

# initialize and read input parameters
import paraview
paraview.options.batch = True
paraview.options.symmetric = True

import paraview.simple as pvsimple
from paraview.modules import vtkPVCatalyst

def _refHolderMaker(obj):
    def _refHolder(obj2, string):
        tmp = obj
    return _refHolder

def coProcess(grid, time, step, scriptname, wholeExtent):
    import os
    scriptpath, scriptname = os.path.split(scriptname)
    sys.path.append(scriptpath)
    if scriptname.endswith(".py"):
        print('script name is %s' % scriptname)
        scriptname = scriptname[0:len(scriptname)-3]
    try:
        cpscript = __import__(scriptname)
    except:
        print(sys.exc_info())
        print('Cannot find %s -- no coprocessing will be performed.' % scriptname)
        sys.exit(1)
        return

    datadescription = vtkPVCatalyst.vtkCPDataDescription()
    datadescription.SetTimeData(time, step)
    datadescription.AddInput("input")
    # add another input/channel that should never be requested by Live. we check later that
    # this input/channel indeed is NOT requested to verify that Catalyst is only
    # requesting what it should be.
    datadescription.AddInput("other_input")
    cpscript.RequestDataDescription(datadescription)

    inputdescription = datadescription.GetInputDescriptionByName("other_input")
    if inputdescription.GetIfGridIsNecessary() == True:
        print('ERROR: Should not be requesting Catalyst channel named "other_input" but am')
        sys.exit(1)
        return

    inputdescription = datadescription.GetInputDescriptionByName("input")
    if inputdescription.GetIfGridIsNecessary() == False:
        return

    inputdescription.SetGrid(grid)
    if grid.IsA("vtkImageData") == True or grid.IsA("vtkRectilinearGrid") == True \
            or grid.IsA("vtkStructuredGrid") == True:
        inputdescription.SetWholeExtent(wholeExtent)

    cpscript.DoCoProcessing(datadescription)



try:
    numsteps = int(sys.argv[2])
except ValueError:
    print('the last argument should be a number')
    numsteps = 10


pm = pvsimple.servermanager.vtkProcessModule.GetProcessModule()
rank = pm.GetPartitionId()
nranks = pm.GetNumberOfLocalPartitions()

for step in range(numsteps):
    # assume simulation time starts at 0
    time = step/float(numsteps)
    print("[%d/%d]: Timestep %d of %d (time=%f)" % (rank, nranks, step, numsteps, time))

    # create the input to the coprocessing library.  normally
    # this will come from the adaptor
    wavelet = pvsimple.Wavelet()
    wholeExtent = wavelet.WholeExtent
    # put in some variation in the point data that changes with time
    wavelet.Maximum = 255+200*math.sin(step * math.pi / 100)
    wavelet.UpdatePipeline()
    imageData = pvsimple.servermanager.Fetch(wavelet)

    # note that we delete wavelet now since.  if not, it will
    # get deleted automatically in the coprocessing script
    pvsimple.Delete(wavelet)
    wavelet = None

    # "perform" coprocessing.  results are outputted only if
    # the passed in script says we should at time/step
    coProcess(imageData, time, step, sys.argv[1], wholeExtent)
    imageData = None
    import time
    time.sleep(1)
print("done")
