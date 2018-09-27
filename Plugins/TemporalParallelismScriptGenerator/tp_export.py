from paraview import smtrace, servermanager, smstate

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


class ReaderFilter(smtrace.PipelineProxyFilter):
    def should_never_trace(self, prop):
        if smtrace.PipelineProxyFilter.should_never_trace(self, prop): return True
        # skip filename related properties.
        return prop.get_property_name() in [\
            "FilePrefix", "XMLFileName", "FilePattern", "FileRange", "FileName",
            "FileNames"]

# -----------------------------------------------------------------------------
class ReaderAccessor(smtrace.RealProxyAccessor):
    """This accessor is created instead of the standard one for proxies that
    are the temporal readers. This accessor override the
    trace_ctor() method to trace the constructor as the RegisterReader() call,
    since the proxy is a dummy, in this case.
    """
    def __init__(self, varname, proxy, filenameglob):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        self.FileNameGlob = filenameglob

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        # FIXME: ensures thate FileName doesn't get traced.

        # change to call STP.CreateReader instead.
        ctor_args = "%s, fileInfo='%s'" % (ctor, self.FileNameGlob)
        ctor = "STP.CreateReader"
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, ReaderFilter(), ctor_args, skip_assignment)

        trace = smtrace.TraceOutput()
        trace.append(original_trace)
        trace.append(\
            "timeSteps = %s.TimestepValues if len(%s.TimestepValues) != 0 else [0]" % (self, self))
        return trace.raw_data()

# -----------------------------------------------------------------------------
class ViewAccessor(smtrace.RealProxyAccessor):
    """Accessor for views. Overrides trace_ctor() to trace registering of the
    view with the STP. (I wonder if this registering should be moved to
    the end of the state for better readability of the generated state files.
    """
    def __init__(self, varname, proxy, screenshot_info):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)
        self.ScreenshotInfo = screenshot_info

    def trace_ctor(self, ctor, filter, ctor_args=None, skip_assignment=False):
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, filter, ctor_args, skip_assignment)
        trace = smtrace.TraceOutput(original_trace)
        trace.append_separated(["# register the view with coprocessor",
          "# and provide it with information such as the filename to use,",
          "# how frequently to write the images, etc."])
        params = self.ScreenshotInfo
        assert len(params) == 4
        trace.append([
            "STP.RegisterView(%s," % self,
            "    filename='%s', magnification=%s, width=%s, height=%s, tp_views=tp_views)" %\
                (params[0], params[1], params[2], params[3])])
        trace.append_separator()
        return trace.raw_data()

# -----------------------------------------------------------------------------
class WriterAccessor(smtrace.RealProxyAccessor):
    """Accessor for writers. Overrides trace_ctor() to use the actual writer
    proxy name instead of the dummy-writer proxy's name.
    """
    def __init__(self, varname, proxy):
        smtrace.RealProxyAccessor.__init__(self, varname, proxy)

    def get_proxy_label(self, xmlgroup, xmlname):
        pxm = servermanager.ProxyManager()
        prototype = pxm.GetPrototypeProxy(xmlgroup, xmlname)
        if not prototype:
            # a bit of a hack but we assume that there's a stub of some
            # writer that's not available in this build but is available
            # with the build used by the simulation code (probably through a plugin)
            # this stub must have the proper name in the coprocessing hints
            print("WARNING: Could not find %s writer in %s" \
                "XML group. This is not a problem as long as the writer is available with " \
                "the ParaView build used by the simulation code." % (xmlname, xmlgroup))
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

        filename = self.get_object().FileName
        ctor = self.get_proxy_label(xmlgroup, xmlname)
        original_trace = smtrace.RealProxyAccessor.trace_ctor(\
            self, ctor, filter, ctor_args, skip_assignment)

        trace = smtrace.TraceOutput(original_trace)
        trace.append_separated(["# register the writer with STP",
          "# and provide it with information such as the filename to use"])
        trace.append("STP.RegisterWriter(%s, '%s', tp_writers)" % \
            (self, filename))
        trace.append_separator()
        return trace.raw_data()

class tpstate_filter_proxies_to_serialize(object):
    """filter used to skip views and representations a when export_rendering is
    disabled."""
    def __call__(self, proxy):
        global export_rendering
        if not smstate.visible_representations()(proxy): return False
        if (not export_rendering) and \
            (proxy.GetXMLGroup() in ["views", "representations"]): return False
        return True

def tp_hook(varname, proxy):
    global export_rendering, screenshot_info, reader_input_map
    """callback to create our special accessors instead of the default ones."""
    pname = smtrace.Trace.get_registered_name(proxy, "sources")
    if pname and pname in reader_input_map:
        # this is a reader.
        return ReaderAccessor(varname, proxy, reader_input_map[pname])
    if pname and proxy.GetHints() and proxy.GetHints().FindNestedElementByName("WriterProxy"):
        return WriterAccessor(varname, proxy)
    pname = smtrace.Trace.get_registered_name(proxy, "views")
    if pname:
        # since view is being accessed, ensure that we were indeed saving
        # rendering components.
        assert export_rendering
        return ViewAccessor(varname, proxy, screenshot_info[pname])
    raise NotImplementedError


# Start trace
filter = tpstate_filter_proxies_to_serialize()
smtrace.RealProxyAccessor.register_create_callback(tp_hook)
state = smstate.get_state(filter=filter)
smtrace.RealProxyAccessor.unregister_create_callback(tp_hook)

output_contents = """
from paraview.simple import *
from paraview import spatiotemporalparallelism as STP

tp_writers = []
tp_views = []

timeCompartmentSize = %s
globalController, temporalController, timeCompartmentSize = STP.CreateControllers(timeCompartmentSize)

%s

STP.IterateOverTimeSteps(globalController, timeCompartmentSize, timeSteps, tp_writers, tp_views)
""" % (timeCompartmentSize, state)


outFile = open(scriptFileName, 'w')
outFile.write(output_contents)
outFile.close()
