import sys
if len(sys.argv) != 3:
    print("command is 'python <python driver code> <script name> <number of time steps>'")
    sys.exit(1)
import paraview
import paraview.vtk as vtk
import paraview.simple as pvsimple
import math

# initialize and read input parameters
paraview.options.batch = True
paraview.options.symmetric = True

def _refHolderMaker(obj):
    def _refHolder(obj2, string):
        tmp = obj
    return _refHolder

def coProcess(grid, time, step, scriptname, wholeExtent):
    import vtkPVCatalystPython
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

    datadescription = vtkPVCatalystPython.vtkCPDataDescription()
    datadescription.SetTimeData(time, step)
    datadescription.AddInput("input")
    cpscript.RequestDataDescription(datadescription)
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


#imageData2 = vtk.vtkImageData()

for step in range(numsteps):
    # assume simulation time starts at 0
    time = step/float(numsteps)

    # create the input to the coprocessing library.  normally
    # this will come from the adaptor
    wavelet = pvsimple.Wavelet()
    wholeExtent = wavelet.WholeExtent
    # put in some variation in the point data that changes with time
    wavelet.Maximum = 255+200*math.sin(step)
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
