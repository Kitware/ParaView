import sys
if len(sys.argv) != 3:
    print "command is 'python <python driver code> <script name> <number of time steps>'"
    sys.exit(1)
import paraview
import paraview.vtk as vtk
import paraview.simple as pvsimple

# initialize and read input parameters
paraview.options.batch = True
paraview.options.symmetric = True

def _refHolderMaker(obj):
    def _refHolder(obj2, string):
        tmp = obj
    return _refHolder

def coProcess(grid, time, step, scriptname):
    import vtkCoProcessorPython
    import os
    scriptpath, scriptname = os.path.split(scriptname)
    sys.path.append(scriptpath)
    if scriptname.endswith(".py"):
        print 'script name is ', scriptname
        scriptname = scriptname[0:len(scriptname)-3]
    try:
        cpscript = __import__(scriptname)
    except:
        print 'Cannot find ', scriptname, ' -- no coprocessing will be performed.'
        return

    datadescription = vtkCoProcessorPython.vtkCPDataDescription()
    datadescription.SetTimeData(time, step)
    datadescription.AddInput("input")
    cpscript.RequestDataDescription(datadescription)
    inputdescription = datadescription.GetInputDescriptionByName("input")
    if inputdescription.GetIfGridIsNecessary() == False:
        return

    inputdescription.SetGrid(grid)
    cpscript.DoCoProcessing(datadescription)

try:
    numsteps = int(sys.argv[2])
except ValueError:
    print 'the last argument should be a number'
    numsteps = 10


#imageData2 = vtk.vtkImageData()

for step in range(numsteps):
    # assume simulation time starts at 0
    time = step/float(numsteps)

    # create the input to the coprocessing library.  normally
    # this will come from the adaptor
    wavelet = pvsimple.Wavelet()
    #wavelet.WholeExtent = [0, 250, 0, 250, 0, 10]
    wavelet.UpdatePipeline()
    imageData = pvsimple.servermanager.Fetch(wavelet)

    # note that we delete wavelet now since.  if not, it will
    # get deleted automatically in the coprocessing script
    pvsimple.Delete(wavelet)
    wavelet = None

    # "perform" coprocessing.  results are outputted only if
    # the passed in script says we should at time/step
    coProcess(imageData, time, step, sys.argv[1])
    imageData = None
