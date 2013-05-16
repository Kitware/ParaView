# boolean telling if we want to export rendering.
export_rendering = %1

# string->string map with key being the proxyname while value being the
# file name on the system the generated python script is to be run on.
reader_input_map = { %2 };

# list of views along with a file name and magnification flag
screenshot_info = {%3}

# the number of processes working together on a single time step
timeCompartmentSize = %4

# the name of the Python script to be outputted
scriptFileName = "%5"

# this method replaces construction of proxies with methods
# that will work on the remote machine
def tp_hook(info, ctorMethod, ctorArgs, extraCtorCommands):
    global reader_input_map, export_rendering
    if info.ProxyName in reader_input_map.keys():
        # mark this proxy as a reader input to make it easier to locate the
        # reader input for the writers.
        info.Proxy.tpReaderInput = reader_input_map[info.ProxyName]
        # take out the guiName argument if it exists
        newArgs = []
        import re
        for arg in ctorArgs:
            if re.match("^FileName", arg) == None and re.match("^guiName", arg) == None:
                newArgs.append(arg)
        newArgs = [ctorMethod, newArgs, "\"%s\"" % info.Proxy.tpReaderInput]
        ctorMethod = "CreateReader"
        extraCtorCommands = "timeSteps = GetActiveSource().TimestepValues if len(GetActiveSource().TimestepValues)!=0 else [0]"
        return (ctorMethod, newArgs, extraCtorCommands)
    proxy = info.Proxy
    # handle views
    if proxy.GetXMLGroup() == 'views' and export_rendering:
        proxyName = servermanager.ProxyManager().GetProxyName("views", proxy)
        ctorArgs = [ ctorMethod, "\"%s\"" % screenshot_info[proxyName][0], \
                         screenshot_info[proxyName][1], screenshot_info[proxyName][2], \
                         screenshot_info[proxyName][3], "tp_views" ]
        return ("CreateView", ctorArgs, extraCtorCommands)

    # handle writers.
    if not proxy.GetHints() or \
      not proxy.GetHints().FindNestedElementByName("WriterProxy"):
        return (ctorMethod, ctorArgs, extraCtorCommands)
    # this is a writer we are dealing with.
    xmlElement = proxy.GetHints().FindNestedElementByName("WriterProxy")
    xmlgroup = xmlElement.GetAttribute("group")
    xmlname = xmlElement.GetAttribute("name")
    pxm = smtrace.servermanager.ProxyManager()
    writer_proxy = pxm.GetPrototypeProxy(xmlgroup, xmlname)
    ctorMethod =  \
      smtrace.servermanager._make_name_valid(writer_proxy.GetXMLLabel())
    ctorArgs = [ctorMethod, \
                "\"%s\"" % proxy.GetProperty("FileName").GetElement(0), "tp_writers" ]
    ctorMethod = "CreateWriter"

    return (ctorMethod, ctorArgs, '')

try:
    from paraview import smstate, smtrace
except:
    raise RuntimeError('could not import paraview.smstate')


# Start trace
smtrace.start_trace(CaptureAllProperties=True, UseGuiName=True)

# update trace globals.
smtrace.trace_globals.proxy_ctor_hook = staticmethod(tp_hook)
smtrace.trace_globals.trace_output = []

# Get list of proxy lists
proxy_lists = smstate.get_proxy_lists_ordered_by_group(WithRendering=export_rendering)
# Now register the proxies with the smtrace module
for proxy_list in proxy_lists:
    smstate.register_proxies_by_dependency(proxy_list)

# Calling append_trace causes the smtrace module to sort out all the
# registered proxies and their properties and write them as executable
# python.
smtrace.append_trace()

# Stop trace and print it to the console
smtrace.stop_trace()

output_contents = """
try: paraview.simple
except: from paraview.simple import *

import sys
import os
import paraview

paraview.servermanager.misc.GlobalMapperProperties.GlobalImmediateModeRendering = 1

# trying to import the library where I can specify the global and subcontrollers
import vtkParallelCorePython

paraview.options.batch = True # this may not be necessary
paraview.simple._DisableFirstRenderCameraReset()

def CreateTimeCompartments(globalController, timeCompartmentSize):
    if globalController.GetNumberOfProcesses() == 1:
        print 'single process'
        return
    elif globalController.GetNumberOfProcesses() %% timeCompartmentSize != 0:
        print 'number of processes must be an integer multiple of time compartment size'
        return
    elif timeCompartmentSize == globalController.GetNumberOfProcesses():
        return globalController

    gid = globalController.GetLocalProcessId()
    timeCompartmentGroupId = int (gid / timeCompartmentSize )
    newController = globalController.PartitionController(timeCompartmentGroupId, gid %% timeCompartmentSize)
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

currentTimeStep = -1
def UpdateCurrentTimeStep(globalController, timeCompartmentSize):
    global currentTimeStep
    if currentTimeStep == -1:
        currentTimeStep = globalController.GetLocalProcessId() / timeCompartmentSize
        return currentTimeStep

    numTimeStepsPerIteration = globalController.GetNumberOfProcesses() / timeCompartmentSize
    currentTimeStep = currentTimeStep + numTimeStepsPerIteration
    return currentTimeStep

def WriteImages(currentTimeStep, currentTime, views):
    for view in views:
        filename = view.tpFileName.replace("%%t", str(currentTimeStep))
        view.ViewTime = currentTime
        WriteImage(filename, view, Magnification=view.tpMagnification)

def WriteFiles(currentTimeStep, currentTime, writers):
    for writer in writers:
        originalfilename = writer.FileName
        fname = originalfilename.replace("%%t", str(currentTimeStep))
        writer.FileName = fname
        writer.UpdatePipeline(currentTime)
        writer.FileName = originalfilename

def IterateOverTimeSteps(globalController, timeCompartmentSize, timeSteps, writers, views):
    currentTimeStep = UpdateCurrentTimeStep(globalController, timeCompartmentSize)
    while currentTimeStep < len(timeSteps):
        #print globalController.GetLocalProcessId(), " is working on ", currentTimeStep
        WriteImages(currentTimeStep, timeSteps[currentTimeStep], views)
        WriteFiles(currentTimeStep, timeSteps[currentTimeStep], writers)
        currentTimeStep = UpdateCurrentTimeStep(globalController, timeCompartmentSize)

def CreateReader(ctor, args, fileInfo):
    "Creates a reader, checks if it can be used, and sets the filenames"
    reader = ctor()
    CheckReader(reader)
    import glob
    files = glob.glob(fileInfo)
    files.sort() # assume there is a logical ordering of the filenames that corresponds to time ordering
    reader.FileName = files
    for a in args:
        s = "reader."+a
        exec (s)

    return reader

def CreateWriter(ctor, filename, tp_writers):
    writer = ctor()
    writer.FileName = filename
    tp_writers.append(writer)
    return writer

def CreateView(proxy_ctor, filename, magnification, width, height, tp_views):
    view = proxy_ctor()
    view.add_attribute("tpFileName", filename)
    view.add_attribute("tpMagnification", magnification)
    tp_views.append(view)
    view.ViewSize = [width, height]
    return view

tp_writers = []
tp_views = []
# ==================== end of specialized temporal parallelism sections ==================

timeCompartmentSize = %s
globalController, temporalController, timeCompartmentSize = CreateControllers(timeCompartmentSize)

%s

IterateOverTimeSteps(globalController, timeCompartmentSize, timeSteps, tp_writers, tp_views)
"""

pipeline_trace = ""
for original_line in smtrace.trace_globals.trace_output:
    for line in original_line.split("\n"):
        pipeline_trace += line + "\n";

outFile = open(scriptFileName, 'w')

outFile.write(output_contents % (timeCompartmentSize, pipeline_trace))
outFile.close()
