r"""This module is used to by the CoProcessingScriptGenerator plugin to aid in
capturing ParaView state as CoProcessing python script.

This can capture the ParaView state in a Pipeline object that can then be used
in CoProcessing scripts. The entry point into this module is the function
DumpPipeline() which returns the Python trace script. Most of the other
functions can be considered internal.
"""

from paraview import smtrace, smstate, servermanager
import numbers

_cpstate_globals = None

def reset_globals():
    global _cpstate_globals
    _cpstate_globals = None

def initialize_globals():
    global _cpstate_globals
    class _state(object):
        def __init__(self):
            self.write_frequencies = {}
            self.simulation_input_map = {}
            self.view_proxies = []
            self.screenshot_info = {}
            self.export_rendering = False
            self.cinema_tracks = {}
            self.cinema_arrays = {}
            self.channels_needed = []
            self.enable_live_viz = False
            self.live_viz_frequency = 0
            self.variable_to_name_map = {}
    _cpstate_globals = _state()
    return _cpstate_globals

def get_globals():
    global _cpstate_globals
    if _cpstate_globals is None:
        raise RuntimeError("get_globals called before initialize_globals!")
    return _cpstate_globals

# -----------------------------------------------------------------------------
def locate_simulation_inputs(proxy):
    """Given any sink/filter proxy, returns a list of upstream proxies that have
       been flagged as 'simulation input' in the state exporting wizard."""
    if hasattr(proxy, "cpSimulationInput"):
        return [ proxy.cpSimulationInput ]

    input_proxies = []
    for property in servermanager.PropertyIterator(proxy):
        if property.IsA("vtkSMInputProperty"):
            ip = servermanager.InputProperty(proxy, property)
            input_proxies = input_proxies + ip[:]

    simulation_inputs = []
    for input in input_proxies:
        if input:
            cur_si = locate_simulation_inputs(input.SMProxy)
            for cur in cur_si:
                if not cur in simulation_inputs:
                    simulation_inputs.append(cur)
    return simulation_inputs

# -----------------------------------------------------------------------------
def locate_simulation_inputs_for_view(view_proxy):
    """Given a view proxy, returns a list of source proxies that have been
        flagged as the 'simulation input' in the state exporting wizard."""
    reprProp = servermanager.ProxyProperty(view_proxy, view_proxy.GetProperty("Representations"))
    reprs = reprProp[:]
    all_sim_inputs = []
    for repr in reprs:
        sim_inputs = locate_simulation_inputs(repr)
        all_sim_inputs = all_sim_inputs + sim_inputs
    return all_sim_inputs

# -----------------------------------------------------------------------------
class ProducerAccessor(smtrace.RealProxyAccessor):
    """This accessor is created instead of the standard one for proxies that
    have been marked as simulation inputs. This accessor override the
    trace_ctor() method to trace the constructor as the CreateProducer() call,
    since the proxy is a dummy, in this case.
    """
    def __init__(self, varname, proxy, simname):
        self.SimulationInputName = simname
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        # this cpSimulationInput attribute is used to locate the proxy later on.
        proxy.SMProxy.cpSimulationInput = simname
        self.varname = varname

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        trace = smtrace.TraceOutput()
        trace.append("# create a producer from a simulation input")
        trace.append("%s = coprocessor.CreateProducer(datadescription, '%s')" % \
            (self, self.SimulationInputName))

        # self.varname has .'s and *'s stripped. Knowing this, the key in cpstate_globals.cinema_arrays
        # should also be stripped of .'s and *'s. Refers to /Qt/ApplicationComponents/pqExportCatalystScript.cxx
        cpstate_globals = get_globals()
        if self.varname in cpstate_globals.cinema_arrays:
            arrays = cpstate_globals.cinema_arrays[self.varname]
            trace.append_separated(["# define target arrays of filters."])
            trace.append(["coprocessor.AddArraysToCinemaTrack(%s, 'arraySelection', %s)" % (self, arrays)])
            trace.append_separator()
        return trace.raw_data()

# -----------------------------------------------------------------------------
class SliceAccessor(smtrace.RealProxyAccessor):
    """
    augments traces of slice filters with information to explore the
    parameter space for cinema playback (if enabled)
    """
    def __init__(self, varname, proxy):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        self.varname = varname

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, filter, ctor_args, skip_assignment)
        trace = smtrace.TraceOutput(original_trace)
        cpstate_globals = get_globals()
        if cpstate_globals.cinema_tracks and self.varname in cpstate_globals.cinema_tracks:
            valrange = cpstate_globals.cinema_tracks[self.varname]
            trace.append_separated(["# register the filter with the coprocessor's cinema generator"])
            trace.append(["coprocessor.RegisterCinemaTrack('slice', %s, 'SliceOffsetValues', %s)" % (self, valrange)])

        if self.varname in cpstate_globals.cinema_arrays:
            arrays = cpstate_globals.cinema_arrays[self.varname]
            trace.append_separated(["# define target arrays of filters."])
            trace.append(["coprocessor.AddArraysToCinemaTrack(%s, 'arraySelection', %s)" % (self, arrays)])
            trace.append_separator()
        return trace.raw_data()

# -----------------------------------------------------------------------------
class ContourAccessor(smtrace.RealProxyAccessor):
    """
    augments traces of contour filters with information to explore the
    parameter space for cinema playback (if enabled)
    """
    def __init__(self, varname, proxy):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        self.varname = varname

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, filter, ctor_args, skip_assignment)
        trace = smtrace.TraceOutput(original_trace)
        if cpstate_globals.cinema_tracks and self.varname in cpstate_globals.cinema_tracks:
            valrange = cpstate_globals.cinema_tracks[self.varname]
            trace.append_separated(["# register the filter with the coprocessor's cinema generator"])
            trace.append(["coprocessor.RegisterCinemaTrack('contour', %s, 'Isosurfaces', %s)" % (self, valrange)])

        if self.varname in cpstate_globals.cinema_arrays:
            arrays = cpstate_globals.cinema_arrays[self.varname]
            trace.append_separated(["# define target arrays of filters."])
            trace.append(["coprocessor.AddArraysToCinemaTrack(%s, 'arraySelection', %s)" % (self, arrays)])
            trace.append_separator()
        return trace.raw_data()

# -----------------------------------------------------------------------------
class ClipAccessor(smtrace.RealProxyAccessor):
    """
    augments traces of clip filters with information to explore the
    parameter space for cinema playback (if enabled)
    """
    def __init__(self, varname, proxy):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        self.varname = varname

    def trace_ctor(self, ctor, filter, ctor_args = None, skip_assignment = False):
        original_trace = smtrace.RealProxyAccessor.trace_ctor( \
            self, ctor, filter, ctor_args, skip_assignment)
        trace = smtrace.TraceOutput(original_trace)
        cpstate_globals = get_globals()
        if cpstate_globals.cinema_tracks and self.varname  in cpstate_globals.cinema_tracks:
            valrange = cpstate_globals.cinema_tracks[self.varname]
            trace.append_separated(["# register the filter with the coprocessor's cinema generator"])
            trace.append(["coprocessor.RegisterCinemaTrack('clip', %s, 'OffsetValues', %s)" % (self, valrange)])

        if self.varname in cpstate_globals.cinema_arrays:
            arrays = cpstate_globals.cinema_arrays[self.varname]
            trace.append_separated(["# define target arrays of filters."])
            trace.append(["coprocessor.AddArraysToCinemaTrack(%s, 'arraySelection', %s)" % (self, arrays)])
            trace.append_separator()
        return trace.raw_data()

# TODO: Make Slice, Contour & Clip Accessors to share an interface to reduce code
# duplication. ArrayAccessor could be used as this interface.
# -----------------------------------------------------------------------------
class ArrayAccessor(smtrace.RealProxyAccessor):
    ''' Augments traces of filters by defining names of arrays to be explored.'''
    def __init__(self, varname, proxy):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        self.varname = varname

    def trace_ctor(self, ctor, filter, ctor_args = None, skip_assignment = False):
        original_trace = smtrace.RealProxyAccessor.trace_ctor( \
            self, ctor, filter, ctor_args, skip_assignment)

        trace = smtrace.TraceOutput(original_trace)
        cpstate_globals = get_globals()
        if self.varname in cpstate_globals.cinema_arrays:
            arrays = cpstate_globals.cinema_arrays[self.varname]
            trace.append_separated(["# define target arrays of filters."])
            trace.append(["coprocessor.AddArraysToCinemaTrack(%s, 'arraySelection', %s)" % (self, arrays)])
            trace.append_separator()
        return trace.raw_data()

# -----------------------------------------------------------------------------
class ViewAccessor(smtrace.RealProxyAccessor):
    """Accessor for views. Overrides trace_ctor() to trace registering of the
    view with the coprocessor. (I wonder if this registering should be moved to
    the end of the state for better readability of the generated state files.
    """
    def __init__(self, varname, proxy, proxyname):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        self.ProxyName = proxyname

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, filter, ctor_args, skip_assignment)
        trace = smtrace.TraceOutput(original_trace)
        cpstate_globals = get_globals()
        if self.ProxyName in cpstate_globals.screenshot_info:
           trace.append_separated(["# register the view with coprocessor",
          "# and provide it with information such as the filename to use,",
          "# how frequently to write the images, etc."])
           params = cpstate_globals.screenshot_info[self.ProxyName]
           assert len(params) == 8
           trace.append([
              "coprocessor.RegisterView(%s," % self,
               "    filename='%s', freq=%s, fittoscreen=%s, magnification=%s, width=%s, height=%s, cinema=%s, compression=%s)" %\
                  (params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7]),
              "%s.ViewTime = datadescription.GetTime()" % self])
           trace.append_separator()
        return trace.raw_data()

# -----------------------------------------------------------------------------
class WriterFilter(smtrace.PipelineProxyFilter):
    def should_never_trace(self, prop):
        """overridden to never trace 'WriteFrequency', 'FileName' and
           'PaddingAmount' properties on writers."""
        if prop.get_property_name() in ["WriteFrequency", "FileName", "PaddingAmount"]:
            return True
        return super(WriterFilter, self).should_never_trace(prop)

# -----------------------------------------------------------------------------
class WriterAccessor(smtrace.RealProxyAccessor):
    """Accessor for writers. Overrides trace_ctor() to use the actual writer
    proxy name instead of the dummy-writer proxy's name. Also updates the
    write_frequencies maintained in cpstate_globals with the write frequencies
    for the writer.
    """
    def __init__(self, varname, proxy):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        write_frequency = proxy.GetProperty("WriteFrequency").GetElement(0)

        # Locate which simulation input this write is connected to, if any. If so,
        # we update the write_frequencies datastructure accordingly.
        sim_inputs = locate_simulation_inputs(proxy)
        cpstate_globals = get_globals()
        for sim_input_name in sim_inputs:
            if not write_frequency in cpstate_globals.write_frequencies[sim_input_name]:
                cpstate_globals.write_frequencies[sim_input_name].append(write_frequency)
                cpstate_globals.write_frequencies[sim_input_name].sort()

            if not sim_input_name in cpstate_globals.channels_needed:
                cpstate_globals.channels_needed.append(sim_input_name)

    def get_proxy_label(self, xmlgroup, xmlname):
        pxm = servermanager.ProxyManager()
        prototype = pxm.GetPrototypeProxy(xmlgroup, xmlname)
        if not prototype:
            # a bit of a hack but we assume that there's a stub of some
            # writer that's not available in this build but is available
            # with the build used by the simulation code (probably through a plugin)
            # this stub must have the proper name in the coprocessing hints
            print ("WARNING: Could not find", xmlname, "writer in", xmlgroup, \
                   "XML group. This is not a problem as long as the writer is available with " \
                   "the ParaView build used by the simulation code.")
            ctor = servermanager._make_name_valid(xmlname)
        else:
            ctor = servermanager._make_name_valid(prototype.GetXMLLabel())
        # TODO: use servermanager.ProxyManager().NewProxy() instead
        # we create the writer proxy such that it is not registered with the
        # ParaViewPipelineController, so its state is not sent to ParaView Live.
        return "servermanager.%s.%s" % (xmlgroup, ctor)

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        xmlElement = self.get_object().GetHints().FindNestedElementByName("WriterProxy")
        xmlgroup = xmlElement.GetAttribute("group")
        xmlname = xmlElement.GetAttribute("name")
        write_frequency = self.get_object().GetProperty("WriteFrequency").GetElement(0)
        filename = self.get_object().GetProperty("FileName").GetElement(0)
        padding_amount = self.get_object().GetProperty("PaddingAmount").GetElement(0)
        ctor = self.get_proxy_label(xmlgroup, xmlname)
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, WriterFilter(), ctor_args, skip_assignment)

        trace = smtrace.TraceOutput(original_trace)
        trace.append_separated(["# register the writer with coprocessor",
          "# and provide it with information such as the filename to use,",
          "# how frequently to write the data, etc."])
        trace.append("coprocessor.RegisterWriter(%s, filename='%s', freq=%s, paddingamount=%s)" % \
                     (self, filename, write_frequency, padding_amount))

        trace.append_separator()
        return trace.raw_data()

# -----------------------------------------------------------------------------
def cp_hook(varname, proxy):
    """callback to create our special accessors instead of the standard ones."""
    pname = smtrace.Trace.get_registered_name(proxy, "sources")
    cpstate_globals = get_globals()
    cpstate_globals.variable_to_name_map[proxy] = varname
    if pname:
        if pname in cpstate_globals.simulation_input_map:
            return ProducerAccessor(varname, proxy, cpstate_globals.simulation_input_map[pname])
        elif proxy.GetHints() and proxy.GetHints().FindNestedElementByName("WriterProxy"):
            return WriterAccessor(varname, proxy)
        elif ("servermanager.Slice" in proxy.__class__().__str__() and
            "Plane object" in proxy.__getattribute__("SliceType").__str__()):
            return SliceAccessor(varname, proxy)
        elif ("servermanager.Clip" in proxy.__class__().__str__() and
            "Plane object" in proxy.__getattribute__("ClipType").__str__()):
            return ClipAccessor(varname, proxy)
        elif "servermanager.Contour" in proxy.__class__().__str__():
            return ContourAccessor(varname, proxy)
        else:
            return ArrayAccessor(varname, proxy)
    raise NotImplementedError

# -----------------------------------------------------------------------------
class cpstate_filter_proxies_to_serialize(object):
    """filter used to skip views and representations a when export_rendering is
    disabled."""
    def __call__(self, proxy):
        cpstate_globals = get_globals()
        if not smstate.visible_representations()(proxy): return False
        if (not cpstate_globals.export_rendering) and \
            (proxy.GetXMLGroup() in ["views", "representations"]): return False
        return True

# -----------------------------------------------------------------------------
class NewStyleWriters(object):
    """Helper to dump configured writer proxies, which are not in the pipeline,
    into the script."""

    def __init__(self, make_temporal_script=False):
        self.__cnt = 1
        self.__make_temporal_script = make_temporal_script

    def __make_name(self, name):
        """
        emulating name uniqueness that trace brings to variable names.
        This may not be necessary because we register everything we make immediately
        so var names can probably conflict.
        """
        ret = smtrace.Trace.get_varname(name)+str(self.__cnt)
        self.__cnt = self.__cnt + 1
        return ret

    def make_trace(self):
        """gather trace for the writer proxies that are not in the trace pipeline but
        rather in the new export state.
        """
        res = []
        res.append("")
        res.append("# Now any catalyst writers")
        pxm = servermanager.ProxyManager()
        globalepxy = pxm.GetProxy("export_global", "catalyst")
        exports = pxm.GetProxiesInGroup("export_writers") #todo should use ExportDepot
        for x in exports:
            xs = x[0]
            pxy = pxm.GetProxy('export_writers', xs)
            if not pxy.HasAnnotation('enabled'):
                continue

            xmlname = pxy.GetXMLName()
            if xmlname == "Cinema image options":
                # skip the array and property export information we stuff in this proxy
                continue

            # note: this logic is not truly correct. the way this is setup,
            # there is no good way to really find the variable name used for the input since the names
            # that smtrace assigns are already cleaned up at this point.
            # Ideally, this class should have been written as a true `Accessor` so it could
            # be traced correctly. Right now, I am hacking this to attempt to get a reasonable name
            # that works in most cases.
            inputname = xs.split('|')[0]
            inputname = servermanager._make_name_valid(inputname)
            inputname = inputname[0].lower() + inputname[1:]

            writername = xs.split('|')[1]

            xmlgroup = pxy.GetXMLGroup()

            padding_amount = globalepxy.GetProperty("FileNamePadding").GetElement(0)
            write_frequency = pxy.GetProperty("WriteFrequency").GetElement(0)
            filename = pxy.GetProperty("CatalystFilePattern").GetElement(0)

            cpstate_globals = get_globals()
            sim_inputs = locate_simulation_inputs(pxy)
            for sim_input_name in sim_inputs:
                if not write_frequency in cpstate_globals.write_frequencies[sim_input_name]:
                    cpstate_globals.write_frequencies[sim_input_name].append(write_frequency)
                    cpstate_globals.write_frequencies[sim_input_name].sort()

                if not sim_input_name in cpstate_globals.channels_needed:
                    cpstate_globals.channels_needed.append(sim_input_name)

            prototype = pxm.GetPrototypeProxy(xmlgroup, xmlname)
            if not prototype:
                varname = self.__make_name(xmlname)
            else:
                varname = self.__make_name(prototype.GetXMLLabel())
            # Write pass array proxy
            if pxy.GetProperty("ChooseArraysToWrite").GetElement(0) == 1:
                point_arrays = []
                cell_arrays = []
                arrays_property = pxy.GetProperty("PointDataArrays")
                for i in range(arrays_property.GetNumberOfElements()):
                    point_arrays.append(arrays_property.GetElement(i))
                arrays_property = pxy.GetProperty("CellDataArrays")
                for i in range(arrays_property.GetNumberOfElements()):
                    cell_arrays.append(arrays_property.GetElement(i))
                f = "%s_arrays = PassArrays(Input=%s, PointDataArrays=%s, CellDataArrays=%s)" % \
                    (inputname, inputname, str(point_arrays), str(cell_arrays))
                inputname = "%s_arrays" % inputname
                res.append(f)
            # Actual writer
            f = "%s = servermanager.writers.%s(Input=%s)" % (varname, writername, inputname)
            res.append(f)
            # set various writer properties, except for the filename since that will be set later.
            # point, cell and field arrays should already be set
            notNeededProperties = ['CatalystFilePattern', 'CellDataArrays', 'ChooseArraysToWrite', 'EdgeDataArrays',
                                   'FieldDataArrays', 'FileName', 'FileNameSuffix', 'Filenamesuffix', 'Input',
                                   'PointDataArrays', 'WriteFrequency',]
            for p in pxy.ListProperties():
                if p not in notNeededProperties and len(pxy.GetProperty(p)):
                    if isinstance(pxy.GetProperty(p).GetData(), servermanager.Proxy):
                        proxyvalue = pxy.GetProperty(p).GetData()
                        cpstate_globals = get_globals()
                        if proxyvalue in cpstate_globals.variable_to_name_map:
                            f = "%s.%s = %s" % (varname, p, cpstate_globals.variable_to_name_map[proxyvalue])
                            res.append(f)
                    elif hasattr(pxy.GetProperty(p), 'GetElement'):
                        value = pxy.GetProperty(p).GetElement(0)
                        if isinstance(value, numbers.Number):
                            f = "%s.%s = %s" % (varname, p, value)
                        elif  isinstance(value, str):
                            f = "%s.%s = '%s'" % (varname, p, value)
                        else:
                            f = "%s.%s = %s" % (varname, p, str(value))
                        res.append(f)

            if self.__make_temporal_script:
                f = "STP.RegisterWriter(%s, '%s', tp_writers)" % (
                    varname, filename)
            else:
                f = "coprocessor.RegisterWriter(%s, filename='%s', freq=%s, paddingamount=%s)" % (
                    varname, filename, write_frequency, padding_amount)
            res.append(f)
            res.append("")
        if len(res) == 2:
            return [] # don't clutter output if there are no writers
        return res

# -----------------------------------------------------------------------------
def DumpPipeline(export_rendering, simulation_input_map, screenshot_info,
                 cinema_tracks, cinema_arrays, enable_live_viz, live_viz_frequency):
    """Method that will dump the current pipeline and return it as a string trace.

    export_rendering
      boolean telling if we want to export rendering

    simulation_input_map
      string->string map with key being the proxyname while value being the
      simulation input name.

    screenshot_info
      map with information about screenshots

      * key -> view proxy name

      * value -> [filename, writefreq, fitToScreen, magnification, width, height,
        cinemacamera options, compressionlevel]

    cinema_tracks
      map with information about cinema tracks to record

      * key -> proxy name

      * value -> argument ranges

    cinema_arrays
      map with information about value arrays to be exported

      * key -> proxy name

      * value -> list of array names

    enable_live_viz
      boolean telling if we want to enable Catalyst Live connection

    live_viz_frequency
      integer telling how often to update Live connection. only used if
      enable_live_viz is True
    """

    # reset the global variables.
    initialize_globals()
    cpstate_globals = get_globals()

    cpstate_globals.export_rendering = export_rendering
    cpstate_globals.simulation_input_map = simulation_input_map
    cpstate_globals.screenshot_info = screenshot_info
    cpstate_globals.cinema_tracks = cinema_tracks
    cpstate_globals.cinema_arrays = cinema_arrays
    cpstate_globals.enable_live_viz = enable_live_viz
    cpstate_globals.live_viz_frequency = live_viz_frequency

    # Initialize the write frequency map
    for key in cpstate_globals.simulation_input_map.values():
        cpstate_globals.write_frequencies[key] = []

    # Start trace
    filter = cpstate_filter_proxies_to_serialize()
    smtrace.RealProxyAccessor.register_create_callback(cp_hook)
    state = smstate.get_state(filter=filter, raw=True)
    smtrace.RealProxyAccessor.unregister_create_callback(cp_hook)

    # add in the new style writer proxies
    state = state + NewStyleWriters().make_trace()

    # iterate over all views that were saved in state and update write requencies
    if export_rendering:
        pxm = servermanager.ProxyManager()
        for key, vtuple in screenshot_info.items():
            view = pxm.GetProxy("views", key)
            if not view: continue
            image_write_frequency = int(vtuple[1])
            # Locate which simulation input this write is connected to, if any. If so,
            # we update the write_frequencies datastructure accordingly.
            sim_inputs = locate_simulation_inputs_for_view(view)
            for sim_input_name in sim_inputs:
                if not image_write_frequency in cpstate_globals.write_frequencies[sim_input_name]:
                    cpstate_globals.write_frequencies[sim_input_name].append(image_write_frequency)
                    cpstate_globals.write_frequencies[sim_input_name].sort()

                if not sim_input_name in cpstate_globals.channels_needed:
                    cpstate_globals.channels_needed.append(sim_input_name)

    if enable_live_viz:
        for key in simulation_input_map:
            sim_input_name = simulation_input_map[key]
            if not live_viz_frequency in cpstate_globals.write_frequencies[sim_input_name]:
                cpstate_globals.write_frequencies[sim_input_name].append(live_viz_frequency)
                cpstate_globals.write_frequencies[sim_input_name].sort()

            if not sim_input_name in cpstate_globals.channels_needed:
                cpstate_globals.channels_needed.append(sim_input_name)

    pxm = servermanager.ProxyManager()
    arrays = {}
    for channel_name in cpstate_globals.channels_needed:
        arrays[channel_name] = []
        p = pxm.GetProxy("sources", channel_name)
        if p:
            for i in range(p.GetPointDataInformation().GetNumberOfArrays()):
                arrays[channel_name].append([p.GetPointDataInformation().GetArray(i).GetName(), 0])
            for i in range(p.GetCellDataInformation().GetNumberOfArrays()):
                arrays[channel_name].append([p.GetCellDataInformation().GetArray(i).GetName(), 1])

    # Create global fields values
    pipelineClassDef = "\n"
    pipelineClassDef += "# ----------------------- CoProcessor definition -----------------------\n\n"

    # Create the resulting string that will contains the pipeline definition
    pipelineClassDef += "def CreateCoProcessor():\n"
    pipelineClassDef += "  def _CreatePipeline(coprocessor, datadescription):\n"
    pipelineClassDef += "    class Pipeline:\n";

    # add the traced code.
    for original_line in state:
        for line in original_line.split("\n"):
            if line.find("import *") != -1 or \
                line.find("#### import the simple") != -1:
                continue
            if line:
                pipelineClassDef += "      " + line + "\n"
            else:
                pipelineClassDef += "\n"
    pipelineClassDef += "    return Pipeline()\n";
    pipelineClassDef += "\n"
    pipelineClassDef += "  class CoProcessor(coprocessing.CoProcessor):\n"
    pipelineClassDef += "    def CreatePipeline(self, datadescription):\n"
    pipelineClassDef += "      self.Pipeline = _CreatePipeline(self, datadescription)\n"
    pipelineClassDef += "\n"
    pipelineClassDef += "  coprocessor = CoProcessor()\n";
    pipelineClassDef += "  # these are the frequencies at which the coprocessor updates.\n"
    pipelineClassDef += "  freqs = " + str(cpstate_globals.write_frequencies) + "\n"
    pipelineClassDef += "  coprocessor.SetUpdateFrequencies(freqs)\n"
    if arrays:
        pipelineClassDef += "  if requestSpecificArrays:\n"
        for channel_name in arrays:
            pipelineClassDef += "    arrays = " + str(arrays[channel_name]) + "\n"
            pipelineClassDef += "    coprocessor.SetRequestedArrays('" + channel_name + "', arrays)\n"
    pipelineClassDef += "  coprocessor.SetInitialOutputOptions(timeStepToStartOutputAt,forceOutputAtFirstCall)\n"
    pipelineClassDef += "\n"
    pipelineClassDef += "  if imageRootDirectory:\n"
    pipelineClassDef += "      coprocessor.SetImageRootDirectory(imageRootDirectory)\n"
    pipelineClassDef += "  if dataRootDirectory:\n"
    pipelineClassDef += "      coprocessor.SetDataRootDirectory(dataRootDirectory)\n"
    pipelineClassDef += "\n"
    pipelineClassDef += "  if make_cinema_table:\n"
    pipelineClassDef += "      coprocessor.EnableCinemaDTable()\n"
    pipelineClassDef += "\n"
    pipelineClassDef += "  return coprocessor\n"

    # cleanup globals state
    reset_globals()
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
    # create a new 'Parallel PolyData Writer'
    parallelPolyDataWriter0 = simple.ParallelPolyDataWriter()

    viewname = servermanager.ProxyManager().GetProxyName("views", view.SMProxy)
    script = DumpPipeline(export_rendering=True,
        simulation_input_map={"Wavelet1" : "input"},
        screenshot_info={viewname : [ 'image.png', '1', '1', '2', '400', '400']},
        cinema_tracks = {},
        cinema_arrays = {})
    if filename:
        f = open(filename, "w")
        f.write(script)
        f.close()
    else:
        print ("# *** Generated Script Begin ***")
        print (script)
        print ("# *** Generated Script End ***")

if __name__ == "__main__":
    run()
