r"""This module is used to by the CoProcessingScriptGenerator plugin to aid in
capturing ParaView state as CoProcessing python script.

This can capture the ParaView state in a Pipeline object that can then be used
in CoProcessing scripts. The entry point into this module is the function
DumpPipeline() which returns the Python trace script. Most of the other
functions can be considered internal.

Also refer to paraview.cpexport Module which is used to generate a complete
Python CoProcessing script that can be used with in a vtkCPPythonScriptPipeline.
"""

from paraview import smstate, smtrace, servermanager

class cpstate_globals: pass

def reset_cpstate_globals():
    cpstate_globals.write_frequencies = {}
    cpstate_globals.simulation_input_map = {}
    cpstate_globals.view_proxies = []
    cpstate_globals.screenshot_info = {}
    cpstate_globals.export_rendering = False

reset_cpstate_globals()

# -----------------------------------------------------------------------------
def locate_simulation_inputs(proxy):
    """Given any sink/filter proxy, returns a list of upstream proxies that have
       been flagged as 'simulation input' in the state exporting wizard."""
    if hasattr(proxy, "cpSimulationInput"):
        return [ proxy.cpSimulationInput ]

    input_proxies = []
    for property in smtrace.servermanager.PropertyIterator(proxy):
        if property.IsA("vtkSMInputProperty"):
            ip = smtrace.servermanager.InputProperty(proxy, property)
            input_proxies = input_proxies + ip[:]

    simulation_inputs = []
    for input in input_proxies:
        cur_si = locate_simulation_inputs(input.SMProxy)
        for cur in cur_si:
            if not cur in simulation_inputs:
                simulation_inputs.append(cur)
    return simulation_inputs

# -----------------------------------------------------------------------------
def locate_simulation_inputs_for_view(view_proxy):
    """Given a view proxy, retruns a list of source proxies that have been
        flagged as the 'simulation input' in the state exporting wizard."""
    reprProp = smtrace.servermanager.ProxyProperty(view_proxy, view_proxy.GetProperty("Representations"))
    reprs = reprProp[:]
    all_sim_inputs = []
    for repr in reprs:
        sim_inputs = locate_simulation_inputs(repr)
        all_sim_inputs = all_sim_inputs + sim_inputs
    return all_sim_inputs

def cp_hook(info, ctorMethod, ctorArgs, extraCtorCommands):
    """Callback registered with the smtrace to control the code recorded by the
       trace for simulation inputs and writers, among other things."""
    if info.ProxyName in cpstate_globals.simulation_input_map.keys():
        # mark this proxy as a simulation input to make it easier to locate the
        # simulation input for the writers.
        info.Proxy.cpSimulationInput = cpstate_globals.simulation_input_map[info.ProxyName]
        return ('coprocessor.CreateProducer',\
          [ 'datadescription', '\"%s\"' % (cpstate_globals.simulation_input_map[info.ProxyName]) ], '')

    # handle views
    proxy = info.Proxy
    if proxy.GetXMLGroup() == 'views' and cpstate_globals.export_rendering:
        proxyName = servermanager.ProxyManager().GetProxyName("views", proxy)
        ctorArgs = [ctorMethod,
                    "\"%s\"" % cpstate_globals.screenshot_info[proxyName][0],
                    cpstate_globals.screenshot_info[proxyName][1],
                    cpstate_globals.screenshot_info[proxyName][2],
                    cpstate_globals.screenshot_info[proxyName][3],
                    cpstate_globals.screenshot_info[proxyName][4],
                    cpstate_globals.screenshot_info[proxyName][5]]
        cpstate_globals.view_proxies.append(proxy)
        return ("coprocessor.CreateView", ctorArgs, extraCtorCommands)

    # handle writers.
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
                write_frequency]
    ctorMethod = "coprocessor.CreateWriter"

    # Locate which simulation input this write is connected to, if any. If so,
    # we update the write_frequencies datastructure accordingly.
    sim_inputs = locate_simulation_inputs(proxy)
    for sim_input_name in sim_inputs:
        if not write_frequency in cpstate_globals.write_frequencies[sim_input_name]:
            cpstate_globals.write_frequencies[sim_input_name].append(write_frequency)
            cpstate_globals.write_frequencies[sim_input_name].sort()

    return (ctorMethod, ctorArgs, '')

# -----------------------------------------------------------------------------
def DumpPipeline(export_rendering, simulation_input_map, screenshot_info):
    """
        Method that will dump the current pipeline and return it as a string trace
        - export_rendering    : boolean telling if we want to export rendering
        - simulation_input_map: string->string map with key being the proxyname
                                while value being the simulation input name.
        - screenshot_info     : map with information about screenshots
                                key -> view proxy name
                                value -> [filename, writefreq, fitToScreen,
                                          magnification, width, height]
    """

    # reset the global variables.
    reset_cpstate_globals()

    cpstate_globals.export_rendering = export_rendering
    cpstate_globals.simulation_input_map = simulation_input_map
    cpstate_globals.screenshot_info = screenshot_info

    # Initialize the write frequency map
    for key in cpstate_globals.simulation_input_map.values():
        cpstate_globals.write_frequencies[key] = []

    # Start trace
    smtrace.start_trace(CaptureAllProperties=True, UseGuiName=True)

    # Disconnect the smtrace module's observer.  It should not be
    # active while tracing the state.
    smtrace.reset_trace_observer()

    # update trace globals.
    smtrace.trace_globals.proxy_ctor_hook = staticmethod(cp_hook)
    smtrace.trace_globals.trace_output = []

    # Get list of proxy lists
    proxy_lists = smstate.get_proxy_lists_ordered_by_group(WithRendering=cpstate_globals.export_rendering)
    # Now register the proxies with the smtrace module
    for proxy_list in proxy_lists:
        smstate.register_proxies_by_dependency(proxy_list)

    # Calling append_trace causes the smtrace module to sort out all the
    # registered proxies and their properties and write them as executable
    # python.
    smtrace.append_trace()

    # Stop trace and print it to the console
    smtrace.stop_trace()

    # During tracing, cp_hook() will fill up the cpstate_globals.view_proxies
    # list with view proxies, if rendering was enabled.
    for view_proxy in cpstate_globals.view_proxies:
        # Locate which simulation input this write is connected to, if any. If so,
        # we update the write_frequencies datastructure accordingly.
        sim_inputs = locate_simulation_inputs_for_view(view_proxy)
        proxyName = servermanager.ProxyManager().GetProxyName("views", view_proxy)
        image_write_frequency = cpstate_globals.screenshot_info[proxyName][1]
        for sim_input_name in sim_inputs:
            if not image_write_frequency in cpstate_globals.write_frequencies[sim_input_name]:
                cpstate_globals.write_frequencies[sim_input_name].append(image_write_frequency)
                cpstate_globals.write_frequencies[sim_input_name].sort()

    # Create global fields values
    pipelineClassDef = "\n"
    pipelineClassDef += "# ----------------------- CoProcessor definition -----------------------\n\n"

    # Create the resulting string that will contains the pipeline definition
    pipelineClassDef += "def CreateCoProcessor():\n"
    pipelineClassDef += "  def _CreatePipeline(coprocessor, datadescription):\n"
    pipelineClassDef += "    class Pipeline:\n";

    # add the traced code.
    for original_line in smtrace.trace_globals.trace_output:
        for line in original_line.split("\n"):
            pipelineClassDef += "      " + line + "\n";
    smtrace.clear_trace()
    pipelineClassDef += "    return Pipeline()\n";
    pipelineClassDef += "\n"
    pipelineClassDef += "  class CoProcessor(coprocessing.CoProcessor):\n"
    pipelineClassDef += "    def CreatePipeline(self, datadescription):\n"
    pipelineClassDef += "      self.Pipeline = _CreatePipeline(self, datadescription)\n"
    pipelineClassDef += "\n"
    pipelineClassDef += "  coprocessor = CoProcessor()\n";
    pipelineClassDef += "  freqs = " + str(cpstate_globals.write_frequencies) + "\n"
    pipelineClassDef += "  coprocessor.SetUpdateFrequencies(freqs)\n"
    pipelineClassDef += "  return coprocessor\n"
    return pipelineClassDef

#------------------------------------------------------------------------------
def run(filename=None):
    """Create a dummy pipeline and save the coprocessing state in the filename
        specified, if any, else dumps it out on stdout."""

    from paraview import simple, servermanager
    wavelet = simple.Wavelet(registrationName="Wavelet1")
    contour = simple.Contour()
    display = simple.Show()
    view = simple.Render()

    viewname = servermanager.ProxyManager().GetProxyName("views", view.SMProxy)
    script = DumpPipeline(export_rendering=True,
        simulation_input_map={"Wavelet1" : "input"},
        screenshot_info={viewname : [ 'image.png', '1', '1', '2', '400', '400']})
    if filename:
        f = open(filename, "w")
        f.write(script)
        f.close()
    else:
        print "# *** Generated Script Begin ***"
        print script
        print "# *** Generated Script End ***"

if __name__ == "__main__":
    run()
