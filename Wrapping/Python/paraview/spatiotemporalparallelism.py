import sys
import os
import paraview
import paraview.simple as pvsimple

paraview.servermanager.misc.GlobalMapperProperties.GlobalImmediateModeRendering = 1

# trying to import the library where I can specify the global and subcontrollers
import vtkParallelCorePython

paraview.options.batch = True # this may not be necessary
paraview.simple._DisableFirstRenderCameraReset()

def CreateTimeCompartments(globalController, timeCompartmentSize):
    if globalController.GetNumberOfProcesses() == 1:
        print 'single process'
        return
    elif globalController.GetNumberOfProcesses() % timeCompartmentSize != 0:
        print 'number of processes must be an integer multiple of time compartment size'
        return
    elif timeCompartmentSize == globalController.GetNumberOfProcesses():
        return globalController

    gid = globalController.GetLocalProcessId()
    timeCompartmentGroupId = int (gid / timeCompartmentSize )
    newController = globalController.PartitionController(timeCompartmentGroupId, gid % timeCompartmentSize)
    # must unregister if the reference count is greater than 1
    if newController.GetReferenceCount() > 1:
        newController.UnRegister(None)

    #print gid, ' of global comm is ', newController.GetLocalProcessId()
    globalController.SetGlobalController(newController)
    return newController

def CheckReader(reader):
    if hasattr(reader, "FileName") == False:
        print "ERROR: Don't know how to set file name for ", reader.SMProxy.GetXMLName()
        sys.exit(-1)

    if hasattr(reader, "TimestepValues") == False:
        print "ERROR: ", reader.SMProxy.GetXMLName(), " doesn't have time information"
        sys.exit(-1)

def CreateControllers(timeCompartmentSize):
    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    globalController = pm.GetGlobalController()
    if timeCompartmentSize > globalController.GetNumberOfProcesses():
        timeCompartmentSize = globalController.GetNumberOfProcesses()

    temporalController = CreateTimeCompartments(globalController, timeCompartmentSize)
    return globalController, temporalController, timeCompartmentSize

def WriteImages(currentTimeStep, currentTime, views):
    for view in views:
        filename = view.tpFileName.replace("%t", str(currentTimeStep))
        view.ViewTime = currentTime
        pvsimple.WriteImage(filename, view, Magnification=view.tpMagnification)

def WriteFiles(currentTimeStep, currentTime, writers):
    for writer in writers:
        originalfilename = writer.FileName
        fname = originalfilename.replace("%t", str(currentTimeStep))
        writer.FileName = fname
        writer.UpdatePipeline(currentTime)
        writer.FileName = originalfilename

def IterateOverTimeSteps(globalController, timeCompartmentSize, timeSteps, writers, views):
    numProcs = globalController.GetNumberOfProcesses()
    numTimeCompartments = numProcs/timeCompartmentSize
    tpp = len(timeSteps)/numTimeCompartments
    remainder = len(timeSteps)%numTimeCompartments
    timeCompartmentIndex = int(globalController.GetLocalProcessId()/timeCompartmentSize)
    myStartTimeStep = tpp*timeCompartmentIndex
    myEndTimeStep = myStartTimeStep+tpp
    if timeCompartmentIndex < remainder:
        myStartTimeStep = myStartTimeStep+timeCompartmentIndex
        myEndTimeStep = myStartTimeStep+tpp+1
    else:
        myStartTimeStep = myStartTimeStep+remainder
        myEndTimeStep = myStartTimeStep+tpp

    for currentTimeStep in range(myStartTimeStep,myEndTimeStep):
        #print globalController.GetLocalProcessId(), " is working on ", currentTimeStep
        WriteImages(currentTimeStep, timeSteps[currentTimeStep], views)
        WriteFiles(currentTimeStep, timeSteps[currentTimeStep], writers)

def CreateReader(ctor, fileInfo, **kwargs):
    "Creates a reader, checks if it can be used, and sets the filenames"
    reader = ctor()
    CheckReader(reader)
    import glob
    files = glob.glob(fileInfo)
    files.sort() # assume there is a logical ordering of the filenames that corresponds to time ordering
    reader.FileName = files
    if kwargs:
        pvsimple.SetProperties(reader, **kwargs)
    return reader

def CreateWriter(ctor, filename, tp_writers):
    writer = ctor()
    return RegisterWriter(writer, filename, tp_writers)

def RegisterWriter(writer, filename, tp_writers):
    writer.FileName = filename
    tp_writers.append(writer)
    return writer

def CreateView(proxy_ctor, filename, magnification, width, height, tp_views):
    view = proxy_ctor()
    return RegisterView(view, filename, magnification, width, height, tp_views)

def RegisterView(view, filename, magnification, width, height, tp_views):
    view.add_attribute("tpFileName", filename)
    view.add_attribute("tpMagnification", magnification)
    tp_views.append(view)
    view.ViewSize = [width, height]
    return view
