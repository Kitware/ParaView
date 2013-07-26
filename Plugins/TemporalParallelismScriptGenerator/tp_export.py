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
        ctorMethod = "STP.CreateReader"
        extraCtorCommands = "timeSteps = GetActiveSource().TimestepValues if len(GetActiveSource().TimestepValues)!=0 else [0]"
        return (ctorMethod, newArgs, extraCtorCommands)
    proxy = info.Proxy
    # handle views
    if proxy.GetXMLGroup() == 'views' and export_rendering:
        proxyName = servermanager.ProxyManager().GetProxyName("views", proxy)
        ctorArgs = [ ctorMethod, "\"%s\"" % screenshot_info[proxyName][0], \
                         screenshot_info[proxyName][1], screenshot_info[proxyName][2], \
                         screenshot_info[proxyName][3], "tp_views" ]
        return ("STP.CreateView", ctorArgs, extraCtorCommands)

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
    ctorMethod = "STP.CreateWriter"

    return (ctorMethod, ctorArgs, '')

try:
    from paraview import smstate, smtrace
except:
    raise RuntimeError('could not import paraview.smstate')


# Start trace
capture_modified_properties = not smstate._save_full_state
smtrace.start_trace(CaptureAllProperties=True,
                    CaptureModifiedProperties=capture_modified_properties,
                    UseGuiName=True)

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

from paraview import spatiotemporalparallelism as STP

tp_writers = []
tp_views = []

timeCompartmentSize = %s
globalController, temporalController, timeCompartmentSize = STP.CreateControllers(timeCompartmentSize)

%s

STP.IterateOverTimeSteps(globalController, timeCompartmentSize, timeSteps, tp_writers, tp_views)
"""

pipeline_trace = ""
for original_line in smtrace.trace_globals.trace_output:
    for line in original_line.split("\n"):
        pipeline_trace += line + "\n";

outFile = open(scriptFileName, 'w')

outFile.write(output_contents % (timeCompartmentSize, pipeline_trace))
outFile.close()
