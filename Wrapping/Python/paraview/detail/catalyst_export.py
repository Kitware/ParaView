r"""Module used to generate Catalyst export scripts"""
from .. import simple, smstate, smtrace, servermanager

from paraview.modules.vtkRemotingCore import vtkPVSession


def _get_catalyst_state(options):
    # build a `source_set` comprising of the extractor proxies.
    # if not extracts have been configured, then there's nothing to generate.
    extractors = simple.GetExtractors()
    if not extractors:
        from .. import print_warning
        print_warning('No extractors defined. This script will do nothing until LiveVisualisation is on.')

    # convert catalyst options to PythonStateOptions.
    soptions = servermanager.ProxyManager().NewProxy("pythontracing", "PythonStateOptions")
    soptions = servermanager._getPyProxy(soptions)
    soptions.PropertiesToTraceOnCreate = smstate.RECORD_ACTIVE_MODIFIED_PROPERTIES
    soptions.SkipHiddenDisplayProperties = True
    soptions.SkipRenderingComponents = False
    soptions.SkipActiveComponents = True
    soptions.SkipLayoutComponents = True
    soptions.ExtractsOutputDirectory = options.ExtractsOutputDirectory
    return smstate.get_state(options=soptions, source_set=extractors,
                             preamble=_get_catalyst_preamble(options),
                             postamble=_get_catalyst_postamble(options))


def _get_catalyst_preamble(options):
    """returns the preamble text"""
    return ["# script-version: 2.0",
            "# Catalyst state generated using %s" % simple.GetParaViewSourceVersion(),
            "import paraview",
            "paraview.compatibility.major = %d" % servermanager.vtkSMProxyManager.GetVersionMajor(),
            "paraview.compatibility.minor = %d" % servermanager.vtkSMProxyManager.GetVersionMinor()]


def _get_catalyst_postamble(options):
    """returns the postamble text"""

    tracer = smtrace.ScopedTracer()
    with tracer:
        tracer.config.SetFullyTraceSupplementalProxies(True)

        # flush out some of the header since its not applicable here.
        smtrace.get_current_trace_output_and_reset()

        trace = smtrace.TraceOutput()
        trace.append_separated([ \
            "# " + "-" * 78,
            '# Catalyst options',
            "from paraview import catalyst"])
        accessor = smtrace.ProxyAccessor("options", options)
        trace.append(accessor.trace_ctor("catalyst.Options",
                                         smtrace.ProxyFilter()))
        del accessor

        trace.append_separated([ \
            "# " + "-" * 78,
            "if __name__ == '__main__':",
            "    from paraview.simple import SaveExtractsUsingCatalystOptions",
            "    # Code for non in-situ environments; if executing in post-processing",
            "    # i.e. non-Catalyst mode, let's generate extracts using Catalyst options",
            "    SaveExtractsUsingCatalystOptions(options)"])
        return str(trace)

    return ""


def save_catalyst_state(fname, options, location=vtkPVSession.CLIENT):
    options = servermanager._getPyProxy(options)
    state = _get_catalyst_state(options)
    if not state:
        from .. import print_error
        print_error('No state generated')
        return

    pxm = servermanager.ProxyManager()
    state += '\n'
    pxm.SaveString(state, fname, location)
