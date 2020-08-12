r"""
INTERNAL MODULE, NOT FOR PUBLIC CONSUMPTION.

Used by vtkCPPythonScriptV2Pipeline and may change without notice.

"""
from . import log_level
from .. import log, print_warning
from .detail import UpdateProducers, HasProducers, CreateProducer, \
                    SetActiveDataDescription, IsAnyTriggerActivated, Extract

def load_package_from_zip(zipfilename, packagename=None):
    """Loads a zip file and imports a top-level package from it with the name
    `packagename`. If packagename is None, then the basename for the zipfile is
    used as the package name.

    If import fails, this will throw appropriate Python import errors.

    :returns: the module object for the package on success else None
    """
    from .detail import LoadPackageFromZip
    module = LoadPackageFromZip(zipfilename, packagename)
    # todo: validate package now, that way we can raise failure here itself.
    # rather than waiting till request_data_description or co_process
    _validate_and_initialize(module, is_zip=True)
    return module

def load_package_from_dir(path):
    from .detail import LoadPackageFromDir
    module = LoadPackageFromDir(path)
    _validate_and_initialize(module, is_zip=False)
    return module

def load_module_from_file(fname):
    from .detail import LoadModuleFromFile
    module = LoadModuleFromFile(fname)
    _validate_and_initialize(module, is_zip=False)
    return module

def _activate(func):
    def callback(dataDescription, module, *args, **kwargs):
        log(log_level(), "setting data description %r, %r", dataDescription, module)
        SetActiveDataDescription(dataDescription, module)
        r = func(dataDescription, module, *args, **kwargs)
        log(log_level(), "resetting data description to None")
        SetActiveDataDescription(None, None)
        return r
    return callback

@_activate
def request_data_description(dataDescription, module):
    log(log_level(), "called request_data_description (ts=%d, time=%f)",
        dataDescription.GetTimeStep(), dataDescription.GetTime())

    cntr = _get_extracts_controller(dataDescription, module)

    options = module.options
    if not _is_activated(cntr, options, module):
        return

    params = module.catalyst_params

    # call 'catalyst_request_data_description' on module, if exists
    _call_customized_callback(module, "catalyst_request_data_description", dataDescription,
            do_submodules_first=False)

    # here, we'll inspect each pipeline and based on that determine which arrays
    # etc. are to be requested. Currently, we don't have support for that, so we
    # just request everything.
    for cc in range(dataDescription.GetNumberOfInputDescriptions()):
        inputDD = dataDescription.GetInputDescription(cc)
        inputDD.AllFieldsOn()
        inputDD.GenerateMeshOn()

@_activate
def co_process(dataDescription, module):
    log(log_level(), "called co_process (ts=%d, time=%f)",
        dataDescription.GetTimeStep(), dataDescription.GetTime())
    options = module.options
    cntr = _get_extracts_controller(dataDescription, module)
    params = module.catalyst_params

    # first, load all analysis scripts, if we haven't already.
    # this will set params["analysis_modules"].
    _load_analysis_scripts(module)

    # check if any trigger has been activated, this may not yield true
    # in case none of the extracts (or live) triggers were actually activated.
    if not _is_activated(cntr, options, module):
        return

    if not params["custom_initialization_done"]:
        # if module / submodules have "catalyst_initialize", call it
        _call_customized_callback(module, "catalyst_initialize", dataDescription,
                do_submodules_first=False)

        # ensures that 'catalyst_initialize' gets called only once.
        params["custom_initialization_done"] = True

    # Pass updated data to all producers.
    UpdateProducers()

    # call 'catalyst_coprocess' on module, if exists
    _call_customized_callback(module, "catalyst_coprocess", dataDescription,
            do_submodules_first=False)

    # now call standard co_process that uses the extract
    # generators known to the system
    Extract(cntr)

    # now handle Live request
    if _is_live_trigger_activated(cntr, options):
        _do_live(dataDescription, module)


def finalize(module):
    """
    Called when Catalyst is being finalized. It's unclear which cases this would
    be useful, but we definitely need this for testing to ensure the analysis
    executed as expected.
    """
    log(log_level(), "called finalize (%s)", module)
    params = module.catalyst_params

    if module.options.GenerateCinemaSpecification:
        log(log_level(), "saving cinema specification")
        cntr = params["extracts_controller"]
        cntr.SaveSummaryTable("data.csv", module.options.GetSessionProxyManager())

    # note: the order here is inverse of all other customization callbacks.
    # we first call 'catalyst_finalize' on all analysis pipeline and then the parent
    # module.

    _call_customized_callback(module, "catalyst_finalize", do_submodules_first=True)


def _get_extracts_controller(dataDescription, module):
    """Returns a vtkSMExtractsController instance initialized based on the
    dataDescription and module
    """
    cntr = module.catalyst_params["extracts_controller"]
    cntr.SetTimeStep(dataDescription.GetTimeStep())
    cntr.SetTime(dataDescription.GetTime())
    return cntr

def _create_extracts_controller(options):
    """Returns a new vtkSMExtractsController instance initialized based on the
    options

    :param options: Catalyst options
    :type options: :class:`paraview.servermanager.Proxy`
    for '(misc, CatalystOptions)'
    """
    from paraview.modules.vtkRemotingServerManager import vtkSMExtractsController
    import re, os

    cntr = vtkSMExtractsController()

    # override options if passed in environment options.
    if 'PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY' in os.environ:
        extracts_output_dir = os.environ["PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY"]
    else:
        extracts_output_dir = options.ExtractsOutputDirectory

    # if the paths have ${...}, replace that with environment variables.
    from ..util import ReplaceDollarVariablesWithEnvironment
    extracts_output_dir = ReplaceDollarVariablesWithEnvironment(extracts_output_dir)
    cntr.SetExtractsOutputDirectory(extracts_output_dir)
    return cntr

def _is_live_trigger_activated(cntr, options):
    return options.EnableCatalystLive and options.CatalystLiveTrigger.IsActivated(cntr)

def _has_customized_execution(module):
    return hasattr(module, "catalyst_coprocess") or \
        hasattr(module, "catalyst_request_data_description") or \
        hasattr(module, "request_data_description") or \
        hasattr(module, "catalyst_initialize")

def _call_customized_callback(module, name, *args, do_submodules_first=False, **kwargs):
    def do_module(m):
        if hasattr(m, name):
            log(log_level(), "calling '%s.%s'", m.__name__, name)
            getattr(m, name)(*args, *kwargs)

    def do_submodules(m):
        params = m.catalyst_params
        if params["analysis_modules"] is not None:
            for s_module in params["analysis_modules"]:
                do_module(s_module)

    if do_submodules_first:
        do_submodules(module)
    do_module(module)
    if not do_submodules_first:
        do_submodules(module)


def _validate_and_initialize(module, is_zip):
    """Validates a module to confirm that it is a `module` that we can treat as
    a Catalyst script. Additionally, adds some private meta-data that this
    module will use during exection."""
    module.catalyst_params = {}
    module.catalyst_params["analysis_modules"] = None
    module.catalyst_params["is_zip"] = is_zip
    module.catalyst_params["customized_execution"] = _has_customized_execution(module)
    module.catalyst_params["custom_initialization_done"] = False
    if not hasattr(module, "options"):
        print_warning("Module '%s' missing Catalyst 'options', will use a default options object", module.__name__)
        from . import Options
        module.options = Options()
    # setup extracts controller to use.
    # we need to use the same controller throughout the entire run to ensure
    # that we can generate a summary table, if requested correctly.
    module.catalyst_params["extracts_controller"] = _create_extracts_controller(module.options)

def _load_analysis_scripts(module):
    import importlib, importlib.util, zipimport, os.path
    params  = module.catalyst_params
    if params["analysis_modules"] is not None:
        # already loaded, nothing to do.
        return
    if not hasattr(module, "scripts"):
        # module has no scripts
        params["analysis_modules"] = []
        return

    log(log_level(), "loading analsis modules '%s'", str(module.scripts))
    analysis_modules = []
    for script in module.scripts:
        log(log_level(), "importing '%s'", script)
        if params["is_zip"]:
            z = zipimport.zipimporter(module.__path__[0])
            s_module = z.load_module(script)
        else:
            spec = importlib.util.spec_from_file_location(script,
                    os.path.join(module.__path__[0], "%s.py" % script))
            s_module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(s_module)
        analysis_modules.append(s_module)
    # check if any of the analysis_modules customized execution.
    for m in analysis_modules:
        if _has_customized_execution(m):
            params["customized_execution"] = True
            break
    params["analysis_modules"] = analysis_modules


def _is_activated(cntr, options, module):
    """Check if any triggers are activated for the current cycle.
    Note, this only check existing triggers know to the system. Before the first
    call to 'co_process', this does not include triggers setup in each of the
    analysis scripts."""
    if not options.GlobalTrigger.IsActivated(cntr):
        # skipping since current frame it filtered out by the global trigger.
        log(log_level(), "GlobalTrigger not activated")
        return False

    # if module or analysis_modules have 'function' then we treat the pipeline
    # as activated since triggers may not be sufficient.
    if module.catalyst_params["customized_execution"]:
        log(log_level(), "Treating as activated due to presence of custom callbacks"),
        return True

    if not _is_live_trigger_activated(cntr, options) and not IsAnyTriggerActivated(cntr):
        log(log_level(), "Live or Extracts trigger not activated")
        return False

    log(log_level(), "trigger(s) activated!")
    return True

def _parseurl(url):
    parts = url.split(':')
    if len(parts) == 1:
        hostname = parts[0]
        port = 22222
    else:
        hostname = parts[0]
        port = int(parts[1])
    if not hostname:
        hostname = "localhost"
    if not port:
        port = 22222
    return (hostname, port)

def _do_live(dataDescription, module):
    from .. import servermanager, simple
    log(log_level(), "attempting Live")

    if not HasProducers():
        # when Live is enabled and we have no producers (which happens when
        # there no non-trivial analysis script), we create producers for all
        # inputs
        log(log_level(), "live: create producers since none created in analysis")
        for cc in range(dataDescription.GetNumberOfInputDescriptions()):
            CreateProducer(dataDescription.GetInputDescriptionName(cc))

    options = module.options
    params = module.catalyst_params
    live_link = params.get("live_link", None)
    if live_link is None:
        live_link = servermanager.vtkLiveInsituLink()
        params["live_link"] = live_link

        # update host/port
        hostname, port = _parseurl(options.CatalystLiveURL)
        live_link.SetHostname(hostname)
        live_link.SetInsituPort(port)

    # init the link
    if not live_link.Initialize(servermanager.ProxyManager().SMProxyManager):
        log(log_level(), "live-link init failed, skipping")
        return

    while True:
        # note: the code here is simply copied from an earlier implementation;
        # it may we worth revising this in near future.
        live_link.InsituUpdate(dataDescription.GetTime(), dataDescription.GetTimeStep())

        # sources need to be updated by insitu
        # code. vtkLiveInsituLink never updates the pipeline, it
        # simply uses the data available at the end of the
        # pipeline, if any.
        for source in simple.GetSources().values():
            source.UpdatePipeline(dataDescription.GetTime())

        # push extracts to the visualization process.
        live_link.InsituPostProcess(dataDescription.GetTime(), dataDescription.GetTimeStep())

        # handle pause/break points
        if not live_link.GetSimulationPaused():
            break
        elif live_link.WaitForLiveChange():
            break

_temp_directory = None
def _mpi_exchange_if_needed(filename):
    global _temp_directory

    from ..modules.vtkRemotingCore import vtkProcessModule
    pm = vtkProcessModule.GetProcessModule()
    if pm.GetNumberOfLocalPartitions() <= 1:
        return filename

    from mpi4py import MPI
    from vtkmodules.vtkParallelMPI4Py import vtkMPI4PyCommunicator
    comm = vtkMPI4PyCommunicator.ConvertToPython(pm.GetGlobalController().GetCommunicator())
    if comm.Get_rank() == 0:
        with open(filename, 'rb') as f:
            data = f.read()
    else:
        data = None
    data = comm.bcast(data, root=0)
    if comm.Get_rank() == 0:
        return filename
    else:
        if not _temp_directory:
            import tempfile, os.path
            # we hook the temp-dir to the module so it lasts for the livetime of
            # the interpreter and gets cleaned up on exit.
            _temp_directory = tempfile.TemporaryDirectory()
        filename = os.path.join(_temp_directory.name, os.path.basename(filename))
        with open(filename, "wb") as f:
            f.write(data)
        return filename
