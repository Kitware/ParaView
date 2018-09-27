import sys
import os
import paraview
import paraview.simple as pvsimple
can_savecinema = True
try:
    from vtkmodules.numpy_interface.algorithms import *
    from mpi4py import MPI
except:
    can_savecinema = False

paraview.options.batch = True # this may not be necessary
paraview.simple._DisableFirstRenderCameraReset()

def CreateTimeCompartments(globalController, timeCompartmentSize):
    if globalController.GetNumberOfProcesses() == 1:
        print ('single process')
        return
    elif globalController.GetNumberOfProcesses() % timeCompartmentSize != 0:
        print ('number of processes must be an integer multiple of time compartment size')
        return
    elif timeCompartmentSize == globalController.GetNumberOfProcesses():
        return globalController

    gid = globalController.GetLocalProcessId()
    timeCompartmentGroupId = int (gid / timeCompartmentSize )
    newController = globalController.PartitionController(timeCompartmentGroupId, gid % timeCompartmentSize)
    # must unregister if the reference count is greater than 1
    if newController.GetReferenceCount() > 1:
        newController.UnRegister(None)

    #print (gid, ' of global comm is ', newController.GetLocalProcessId())
    globalController.SetGlobalController(newController)
    return newController

def CheckReader(reader):
    if hasattr(reader, "FileName") == False:
        print ("ERROR: Don't know how to set file name for ", reader.SMProxy.GetXMLName())
        sys.exit(-1)

    if hasattr(reader, "TimestepValues") == False:
        print ("ERROR: ", reader.SMProxy.GetXMLName(), " doesn't have time information")
        sys.exit(-1)

def CreateControllers(timeCompartmentSize):
    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    globalController = pm.GetGlobalController()
    if timeCompartmentSize > globalController.GetNumberOfProcesses():
        timeCompartmentSize = globalController.GetNumberOfProcesses()

    temporalController = CreateTimeCompartments(globalController, timeCompartmentSize)
    return globalController, temporalController, timeCompartmentSize

def WriteImages(currentTimeStep, currentTime, views):
    cinemaLines = []
    cnt = 0
    for view in views:
        filename = view.tpFileName.replace("%t", str(currentTimeStep))
        view.ViewTime = currentTime
        cinemaLines.append(str(currentTime) + "," + view.GetXMLName()+"_"+str(cnt) + "," + filename + "\n")
        pvsimple.WriteImage(filename, view, Magnification=view.tpMagnification)
        cnt = cnt + 1
    return cinemaLines

def WriteFiles(currentTimeStep, currentTime, writers):
    cinemaLines = []
    cnt = 0
    for writer in writers:
        originalfilename = writer.FileName
        fname = originalfilename.replace("%t", str(currentTimeStep))
        writer.FileName = fname
        cinemaLines.append(str(currentTime) + "," + writer.GetXMLName()+"_"+str(cnt) + "," + fname + "\n")
        writer.UpdatePipeline(currentTime)
        writer.FileName = originalfilename
        cnt = cnt + 1
    return cinemaLines

def IterateOverTimeSteps(globalController, timeCompartmentSize, timeSteps, writers, views, make_cinema_table=False):
    if make_cinema_table and not can_savecinema:
        print ("WARNING: Can not save cinema table because MPI4PY is not available.")
        make_cinema_table = False

    numProcs = globalController.GetNumberOfProcesses()
    numTimeCompartments = numProcs//timeCompartmentSize
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

    myStartTimeStep = int(myStartTimeStep)
    myEndTimeStep = int(myEndTimeStep)
    cinemaLines = []
    for currentTimeStep in range(myStartTimeStep,myEndTimeStep):
        #print (globalController.GetLocalProcessId(), " is working on ", currentTimeStep)
        ret = WriteImages(currentTimeStep, timeSteps[currentTimeStep], views)
        if ret:
            cinemaLines.extend(ret)
        ret = WriteFiles(currentTimeStep, timeSteps[currentTimeStep], writers)
        if ret:
            cinemaLines.extend(ret)

    if make_cinema_table:
        # gather the file list from each time compartment to the root and save it
        myGID = globalController.GetLocalProcessId()
        myLID = int(myGID) % int(timeCompartmentSize)
        mystring = ''
        # only one node per time compartment should participate
        if myLID == 0:
            mystring = ''.join(x for x in cinemaLines)
        mylen = len(mystring)
        result = ''
        comm = vtkMPI4PyCommunicator.ConvertToPython(globalController.GetCommunicator())
        gathered_lines = comm.gather(mystring, root=0)
        if myGID == 0:
            # root writes the file, prepending header
            f = open("data.csv", "w")
            f.write("timestep,producer,FILE\n")
            for x in gathered_lines:
                if x:
                    f.write(x)
            f.close()

def CreateReader(ctor, fileInfo, **kwargs):
    "Creates a reader, checks if it can be used, and sets the filenames"
    import glob
    files = glob.glob(fileInfo)
    files.sort() # assume there is a logical ordering of the filenames that corresponds to time ordering
    reader = paraview.simple.OpenDataFile(files)
    CheckReader(reader)
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

def RegisterView(view, filename='filename_%t.vti', magnification=1.0, width=1024, height=1024, tp_views=[]):
    view.add_attribute("tpFileName", filename)
    view.add_attribute("tpMagnification", magnification)
    tp_views.append(view)
    view.ViewSize = [width, height]
    return view
