#==============================================================================
#
#  Program:   ParaView
#  Module:    catalyst.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================
r"""catalyst module provides various ParaView Catalyst related functions. These
functions include those that are used by to export Catalyst Python scripts from
ParaView Desktop and, those that can be used by the generated Catalyst Python
scripts (and other Catalyst Python scripts) to assist with come common tasks as
well live visualization.
"""

# -----------------------------------------------------------------------------
# This file "cp_helper.py" contains all the helper methods that could be
# used inside a generated Python coprocessing script as they will be
# automatically loaded at the initialization of the Python interpretor
# of the vtkCPProcessor when used with Python.
#
# The Python interpreter initialization with those methods definition is
# performed inside vtkCPPythonHelper.cxx
# -----------------------------------------------------------------------------

__cp_helper_script_loaded__ = True

# -----------------------------------------------------------------------------
# List of globals variables
# -----------------------------------------------------------------------------

# boolean telling if we want to export rendering.
export_rendering = False

# Live institu visualization
live_insitu = None

# string->string map with key being the proxyname while value being the
# simulation input name.
simulation_input_map = { };

# string->producer map with key being the simulation input name and the
# value being the vtkPVTrivialProducer that produces that input
simulation_producers_map = {}

# This is map of lists of write frequencies. This is used to populate the
# RequestDataDescription() method so that grids are requested only when needed.
write_frequencies = {}

# we do the views last and only if export_rendering is true
view_proxies = []
cp_views     = []
cp_writers   = []

try:
   from paraview import smstate, smtrace
except:
   raise RuntimeError('could not import paraview.smstate')

try:
   import vtkPVVTKExtensionsCorePython
except:
   raise RuntimeError('could not import vtkPVVTKExtensionsCorePython')


# -----------------------------------------------------------------------------
# From a Proxy return a list of "cpSimulationInput"
# -----------------------------------------------------------------------------

def cp_locate_simulation_inputs(proxy):
    if hasattr(proxy, "cpSimulationInput"):
        return [ proxy.cpSimulationInput ]

    input_proxies = []
    for property in smtrace.servermanager.PropertyIterator(proxy):
        if property.IsA("vtkSMInputProperty"):
            ip = smtrace.servermanager.InputProperty(proxy, property)
            input_proxies = input_proxies + ip[:]

    simulation_inputs = []
    for input in input_proxies:
        cur_si = cp_locate_simulation_inputs(input.SMProxy)
        for cur in cur_si:
            if not cur in simulation_inputs:
                simulation_inputs.append(cur)
    return simulation_inputs

# -----------------------------------------------------------------------------
# From a view proxy find the input of all representations and return that list
# -----------------------------------------------------------------------------

def cp_locate_simulation_inputs_for_view(view_proxy):
    reprProp = smtrace.servermanager.ProxyProperty(view_proxy, view_proxy.GetProperty("Representations"))
    reprs = reprProp[:]
    all_sim_inputs = []
    for repr in reprs:
        sim_inputs = cp_locate_simulation_inputs(repr)
        all_sim_inputs = all_sim_inputs + sim_inputs
    return all_sim_inputs

# -----------------------------------------------------------------------------
# Attach coprocessing hooks for input and writers
# -----------------------------------------------------------------------------

def cp_hook(info, ctorMethod, ctorArgs, extraCtorCommands):
    global write_frequencies, simulation_input_map, export_rendering, view_proxies
    if info.ProxyName in simulation_input_map.keys():
        # mark this proxy as a simulation input to make it easier to locate the
        # simulation input for the writers.
        info.Proxy.cpSimulationInput = simulation_input_map[info.ProxyName]
        return ('CreateProducer',\
          [ 'datadescription', '\"%s\"' % (simulation_input_map[info.ProxyName]) ], '')
    # handle writers.
    proxy = info.Proxy
    if proxy.GetXMLGroup() == 'views' and export_rendering:
        proxyName = servermanager.ProxyManager().GetProxyName("views", proxy)
        ctorArgs = [ ctorMethod, "\"%s\"" % screenshot_info[proxyName][0], screenshot_info[proxyName][1], \
                     screenshot_info[proxyName][2], screenshot_info[proxyName][3], \
                         screenshot_info[proxyName][4], screenshot_info[proxyName][5], "cp_views" ]
        view_proxies.append(proxy)
        return ("CreateCPView", ctorArgs, extraCtorCommands)


        ctorArgs = [ ctorMethod ]
        ctorArgs += \
            screenshot_info[ servermanager.ProxyManager().GetProxyName("views", proxy) ]
        ctorArgs.append("cp_views")
        xmlgroup = xmlElement.GetAttribute("group")
        xmlname = xmlElement.GetAttribute("name")
        pxm = smtrace.servermanager.ProxyManager()
        writer_proxy = pxm.GetPrototypeProxy(xmlgroup, xmlname)
        ctorMethod = "CreateCPView"
        return (ctorMethod, ctorArgs, extraCtorCommands)

    if not proxy.GetHints() or \
      not proxy.GetHints().FindNestedElementByName("CoProcessing"):
        return (ctorMethod, ctorArgs, extraCtorCommands)
    # this is a writer we are dealing with.
    xmlElement = proxy.GetHints().FindNestedElementByName("CoProcessing")
    xmlgroup = xmlElement.GetAttribute("group")
    xmlname = xmlElement.GetAttribute("name")
    pxm = smtrace.servermanager.ProxyManager()
    ctorMethod = None
    writer_proxy = pxm.GetPrototypeProxy(xmlgroup, xmlname)
    if writer_proxy:
        # we have a valid prototype based on the writer stub
        ctorMethod =  \
            smtrace.servermanager._make_name_valid(writer_proxy.GetXMLLabel())
    else:
        # a bit of a hack but we assume that there's a stub of some
        # writer that's not available in this build but is available
        # with the build used by the simulation code (probably through a plugin)
        # this stub must have the proper name in the coprocessing hints
        print "WARNING: Could not find", xmlname, "writer in", xmlgroup, \
            "XML group. This is not a problem as long as the writer is available with " \
            "the ParaView build used by the simulation code."
        ctorMethod =  \
            smtrace.servermanager._make_name_valid(xmlname)

    write_frequency = proxy.GetProperty("WriteFrequency").GetElement(0)
    ctorArgs = [ctorMethod, \
                "\"%s\"" % proxy.GetProperty("FileName").GetElement(0),\
                write_frequency, "cp_writers"]
    ctorMethod = "CreateCPWriter"

    # Locate which simulation input this write is connected to, if any. If so,
    # we update the write_frequencies datastructure accordingly.
    sim_inputs = cp_locate_simulation_inputs(proxy)
    for sim_input_name in sim_inputs:
        if not write_frequency in write_frequencies[sim_input_name]:
            write_frequencies[sim_input_name].append(write_frequency)
            write_frequencies[sim_input_name].sort()

    return (ctorMethod, ctorArgs, '')

# -----------------------------------------------------------------------------
# Based on timestep, toggle fields that need to be read
# -----------------------------------------------------------------------------

def LoadRequestedData(datadescription, input_name):
    freqs = write_frequencies[input_name]
    if len(freqs) == 0:
        return

    timestep = datadescription.GetTimeStep()
    if IsInModulo(timestep, freqs) :
        datadescription.GetInputDescriptionByName(input_name).AllFieldsOn()
        datadescription.GetInputDescriptionByName(input_name).GenerateMeshOn()
    else:
        datadescription.GetInputDescriptionByName(input_name).AllFieldsOff()
        datadescription.GetInputDescriptionByName(input_name).GenerateMeshOff()

# -----------------------------------------------------------------------------
# Delete all the proxy that have been created
# -----------------------------------------------------------------------------

def CleanupProxies():
    tobedeleted = GetNextProxyToDelete()
    while tobedeleted != None:
        Delete(tobedeleted)
        tobedeleted = GetNextProxyToDelete()

# -----------------------------------------------------------------------------
# Write data to disk using the cp_writers based on the frequency set to them
# -----------------------------------------------------------------------------

def WriteAllData(datadescription, cp_writers, timestep):
    for writer in cp_writers:
        if timestep % writer.cpFrequency == 0 or datadescription.GetForceOutput() == True:
            writer.FileName = writer.cpFileName.replace("%t", str(timestep))
            writer.UpdatePipeline()

# -----------------------------------------------------------------------------
# Write images to disk based on the frequency for the set of cp_views
# -----------------------------------------------------------------------------

def WriteAllImages(datadescription, cp_views, timestep, rescale_lookuptable):
    if rescale_lookuptable:
        RescaleDataRange(datadescription, cp_views, timestep)

    for view in cp_views:
        if timestep % view.cpFrequency == 0 or datadescription.GetForceOutput() == True:
            fname = view.cpFileName
            fname = fname.replace("%t", str(timestep))
            if view.cpFitToScreen != 0:
                if view.IsA("vtkSMRenderViewProxy") == True:
                    view.ResetCamera()
                elif view.IsA("vtkSMContextViewProxy") == True:
                    view.ResetDisplay()
                else:
                    print ' do not know what to do with a ', view.GetClassName()

            view.ViewTime = datadescription.GetTime()
            WriteImage(fname, view, Magnification=view.cpMagnification)

# -----------------------------------------------------------------------------
# DataRange can change across time, sometime we want to rescale the color map
# to match to the closer actual data range.
# -----------------------------------------------------------------------------

def RescaleDataRange(datadescription, cp_views, timestep):
    import math
    for view in cp_views:
        if timestep % view.cpFrequency == 0 or datadescription.GetForceOutput() == True:
            reps = view.Representations
            for rep in reps:
                if hasattr(rep, 'Visibility') and rep.Visibility == 1 and hasattr(rep, 'MapScalars') and rep.MapScalars != '':
                    input = rep.Input
                    input.UpdatePipeline() #make sure range is up-to-date
                    lut = rep.LookupTable
                    if lut == None:
                        continue
                    if rep.ColorAttributeType == 'POINT_DATA':
                        datainformation = input.GetPointDataInformation()
                    elif rep.ColorAttributeType == 'CELL_DATA':
                        datainformation = input.GetCellDataInformation()
                    else:
                        print 'something strange with color attribute type', rep.ColorAttributeType

                    if datainformation.GetArray(rep.ColorArrayName) == None:
                        # there is no array on this process. it's possible
                        # that this process has no points or cells
                        continue

                    if lut.VectorMode != 'Magnitude' or \
                       datainformation.GetArray(rep.ColorArrayName).GetNumberOfComponents() == 1:
                        datarange = datainformation.GetArray(rep.ColorArrayName).GetRange(lut.VectorComponent)
                    else:
                        datarange = [0,0]
                        for i in range(datainformation.GetArray(rep.ColorArrayName).GetNumberOfComponents()):
                            for j in range(2):
                                datarange[j] += datainformation.GetArray(rep.ColorArrayName).GetRange(i)[j]*datainformation.GetArray(rep.ColorArrayName).GetRange(i)[j]
                        datarange[0] = math.sqrt(datarange[0])
                        datarange[1] = math.sqrt(datarange[1])


                    import vtkParallelCorePython
                    import paraview.vtk as vtk
                    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
                    globalController = pm.GetGlobalController()
                    localarray = vtk.vtkDoubleArray()
                    localarray.SetNumberOfTuples(2)
                    localarray.SetValue(0, -datarange[0]) # negate so that MPI_MAX gets min instead of doing a MPI_MIN and MPI_MAX
                    localarray.SetValue(1, datarange[1])
                    globalarray = vtk.vtkDoubleArray()
                    globalarray.SetNumberOfTuples(2)
                    globalController.AllReduce(localarray, globalarray, 0)
                    globaldatarange = [-globalarray.GetValue(0), globalarray.GetValue(1)]
                    rgbpoints = lut.RGBPoints.GetData()
                    numpts = len(rgbpoints)/4
                    if globaldatarange[0] != rgbpoints[0] or globaldatarange[1] != rgbpoints[(numpts-1)*4]:
                        # rescale all of the points
                        oldrange = rgbpoints[(numpts-1)*4] - rgbpoints[0]
                        newrange = globaldatarange[1] - globaldatarange[0]
                        # only readjust if the new range isn't zero.
                        if newrange != 0:
                           newrgbpoints = list(rgbpoints)
                           # if the old range isn't 0 then we use that ranges distribution
                           if oldrange != 0:
                              for v in range(numpts-1):
                                 newrgbpoints[v*4] = globaldatarange[0]+(rgbpoints[v*4] - rgbpoints[0])*newrange/oldrange

                              # avoid numerical round-off, at least with the last point
                              newrgbpoints[(numpts-1)*4] = rgbpoints[(numpts-1)*4]
                           else: # the old range is 0 so the best we can do is to space the new points evenly
                              for v in range(numpts+1):
                                 newrgbpoints[v*4] = globaldatarange[0]+v*newrange/(1.0*numpts)

                           lut.RGBPoints.SetData(newrgbpoints)

# -----------------------------------------------------------------------------
# Find the first proxy that can be deleted (not a prototype, helper or the timekeeper)
# -----------------------------------------------------------------------------

def GetNextProxyToDelete():
    proxyiterator = servermanager.ProxyIterator()
    for proxy in proxyiterator:
        group = proxyiterator.GetGroup()
        if group.find("prototypes") != -1:
            continue
        if group != 'timekeeper' and group.find("pq_helper_proxies") == -1 :
            return proxy
    return None

# -----------------------------------------------------------------------------
# Create a producer
# -----------------------------------------------------------------------------

def CreateProducer(datadescription, gridname):
    "Creates a producer proxy for the grid"
    global simulation_producers_map
    if not datadescription.GetInputDescriptionByName(gridname):
        raise RuntimeError, "Simulation input name '%s' does not exist" % gridname
    grid = datadescription.GetInputDescriptionByName(gridname).GetGrid()
    producer = PVTrivialProducer()
    producer.GetClientSideObject().SetOutput(grid, datadescription.GetTime())
    if grid.IsA("vtkImageData") == True or grid.IsA("vtkStructuredGrid") == True or grid.IsA("vtkRectilinearGrid") == True:
        extent = datadescription.GetInputDescriptionByName(gridname).GetWholeExtent()
        producer.WholeExtent= [ extent[0], extent[1], extent[2], extent[3], extent[4], extent[5] ]

    simulation_producers_map[gridname] = producer
    producer.UpdatePipeline()
    return producer

# -----------------------------------------------------------------------------
# Update the producer's output and simulation time for each producer/input
# -----------------------------------------------------------------------------

def UpdateProducers(datadescription):
   global simulation_producers_map
   simtime = datadescription.GetTime()
   for name,producer in simulation_producers_map.iteritems():
      producer.GetClientSideObject().SetOutput(datadescription.GetInputDescriptionByName(name).GetGrid(), simtime)

# -----------------------------------------------------------------------------
# Create a CoProcessing writer with the add-on frequency information
# -----------------------------------------------------------------------------

def CreateCPWriter(proxy_ctor, filename, freq, cp_writers):
    writer = proxy_ctor()
    writer.FileName = filename
    writer.add_attribute("cpFrequency", freq)
    writer.add_attribute("cpFileName", filename)
    cp_writers.append(writer)
    return writer

# -----------------------------------------------------------------------------
# Create a CoProcessing view for image capture with extra meta-data such as
# magnification, size and frequency
# -----------------------------------------------------------------------------

def CreateCPView(proxy_ctor, filename, freq, fittoscreen, magnification, width, height, cp_views):
    view = proxy_ctor()
    view.add_attribute("cpFileName", filename)
    view.add_attribute("cpFrequency", freq)
    view.add_attribute("cpFileName", filename)
    view.add_attribute("cpFitToScreen", fittoscreen)
    view.add_attribute("cpMagnification", magnification)
    view.ViewSize = [ width, height ]
    cp_views.append(view)
    print "Append view: ", str(view)
    return view

# -----------------------------------------------------------------------------
# Method that will dump the current pipeline and return it as a string trace
#   - export_rendering    : boolean telling if we want to export rendering
#   - simulation_input_map: string->string map with key being the proxyname
#                           while value being the simulation input name.
# -----------------------------------------------------------------------------

def DumpPipeline(export_rendering, simulation_input_map, screenshot_info):
    global write_frequencies, cp_views, cp_writers

    # Initialize the write frequency map
    for key in simulation_input_map.values():
        write_frequencies[key] = []

    # Start trace
    smtrace.start_trace(CaptureAllProperties=True, UseGuiName=True)

    # update trace globals.
    smtrace.trace_globals.proxy_ctor_hook = staticmethod(cp_hook)
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

    for view_proxy in view_proxies:
        # Locate which simulation input this write is connected to, if any. If so,
        # we update the write_frequencies datastructure accordingly.
        sim_inputs = cp_locate_simulation_inputs_for_view(view_proxy)
        proxyName = servermanager.ProxyManager().GetProxyName("views", view_proxy)
        image_write_frequency = screenshot_info[proxyName][1]
        for sim_input_name in sim_inputs:
            if not image_write_frequency in write_frequencies[sim_input_name]:
                write_frequencies[sim_input_name].append(image_write_frequency)
                write_frequencies[sim_input_name].sort()

    # Create global fields values
    pipelineClassDef =  "write_frequencies    = " + str(write_frequencies) + "\n"
    pipelineClassDef += "simulation_input_map = " + str(simulation_input_map) + "\n"
    pipelineClassDef += "\n"
    pipelineClassDef += "# ----------------------- Pipeline definition -----------------------\n\n"

    # Create the resulting string that will contains the pipeline definition
    pipelineClassDef += "def CreatePipeline(datadescription):\n"
    pipelineClassDef += "  class Pipeline:\n"
    pipelineClassDef += "    global cp_views, cp_writers\n"
    for original_line in smtrace.trace_globals.trace_output:
        for line in original_line.split("\n"):
            pipelineClassDef += "    " + line + "\n";

    smtrace.clear_trace()

    pipelineClassDef += "  return Pipeline()\n";
    return pipelineClassDef

# -----------------------------------------------------------------------------
# Return True if the given timestep is in one of the provided frequency.
# This is the same thing as doing that if frequencyArray = [2,3,7]
# return (timestep % 2 == 0) or (timestep % 3 == 0) or (timestep % 7 == 0)
# -----------------------------------------------------------------------------

def IsInModulo(timestep, frequencyArray):
   for freqence in frequencyArray:
      if (timestep % freqence == 0):
         return True
   return False

# -----------------------------------------------------------------------------
# Live Insitu Visualization
# -----------------------------------------------------------------------------

def DoLiveInsitu(timestep, pv_host="localhost", pv_port=22222):
   global live_insitu

   # make sure the live insitu is initialized
   if not live_insitu:
      # Create the vtkLiveInsituLink i.e.  the "link" to the visualization processes.
      live_insitu = servermanager.vtkLiveInsituLink()

      # Tell vtkLiveInsituLink what host/port must it connect to for the visualization
      # process.
      live_insitu.SetHostname(pv_host)
      live_insitu.SetInsituPort(pv_port)

      # Initialize the "link"
      live_insitu.SimulationInitialize(servermanager.ActiveConnection.Session.GetSessionProxyManager())

   # For every new timestep, update the simulation state before proceeding.
   live_insitu.SimulationUpdate(timestep)

   # sources need to be updated by insitu code. vtkLiveInsituLink never updates
   # the pipeline, it simply uses the data available at the end of the pipeline,
   # if any.
   for source in GetSources().values():
      source.UpdatePipeline(timestep)

   # push extracts to the visualization process.
   live_insitu.SimulationPostProcess(timestep)
