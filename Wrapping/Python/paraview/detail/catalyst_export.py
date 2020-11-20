r"""Module used to generate Catalyst export scripts"""
from .. import simple, smstate, smtrace, servermanager

def _get_catalyst_state(options):
    # build a `source_set` comprising of the extractor proxies.
    # if not extracts have been configured, then there's nothing to generate.
    extractors = simple.GetExtractors()
    if not extractors:
        return None
    # convert catalyst options to PythonStateOptions.
    soptions = servermanager.ProxyManager().NewProxy("pythontracing", "PythonStateOptions")
    soptions = servermanager._getPyProxy(soptions)
    soptions.PropertiesToTraceOnCreate = smstate.RECORD_MODIFIED_PROPERTIES
    soptions.SkipHiddenDisplayProperties = True
    soptions.SkipRenderingComponents = False
    soptions.ExtractsOutputDirectory = options.ExtractsOutputDirectory
    return smstate.get_state(options=soptions, source_set=extractors,
            preamble=_get_catalyst_preamble(options),
            postamble=_get_catalyst_postamble(options))

def _get_catalyst_preamble(options):
    """returns the preamble text"""
    return ["# script-version: 2.0",
            "# Catalyst state generated using %s" % simple.GetParaViewSourceVersion()]

def _get_catalyst_postamble(options):
    """returns the postamble text"""
    trace_config = smtrace.start_trace(preamble="")
    trace_config.SetFullyTraceSupplementalProxies(True)

    # flush out some of the header since its not applicable here.
    smtrace.get_current_trace_output_and_reset()

    trace = smtrace.TraceOutput()
    trace.append_separated([\
        "# " + "-"*78,
        '# Catalyst options',
        "from paraview import catalyst"])
    accessor = smtrace.ProxyAccessor("options", options)
    trace.append(accessor.trace_ctor("catalyst.Options",
        smtrace.ProxyFilter()))
    del accessor
    smtrace.stop_trace()
    del trace_config
    trace.append_separated([\
        "# " + "-"*78,
        "if __name__ == '__main__':",
        "    from paraview.simple import SaveExtractsUsingCatalystOptions",
        "    # Code for non in-situ environments; if executing in post-processing",
        "    # i.e. non-Catalyst mode, let's generate extracts using Catalyst options",
        "    SaveExtractsUsingCatalystOptions(options)"])
    return str(trace)

def save_catalyst_state(fname, options):
    options = servermanager._getPyProxy(options)
    state = _get_catalyst_state(options)
    if not state:
        raise RuntimeError("No state generated")

    with open(fname, 'w') as file:
        file.write(state)
        file.write('\n')
