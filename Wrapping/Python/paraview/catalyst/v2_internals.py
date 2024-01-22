r"""
INTERNAL MODULE, NOT FOR PUBLIC CONSUMPTION.

Used by vtkCPPythonScriptV2Pipeline and may change without notice.

"""
from . import log_level
from .. import log, print_warning
from ..modules.vtkPVPythonCatalyst import vtkCPPythonScriptV2Helper


def register_module(path):
    """register a file/directory as an importable module.
    Returns the name to use to import the module, on success, otherwise `None`.
    """
    from .detail import RegisterPackageFromZip
    from .detail import RegisterPackageFromDir
    from .detail import RegisterModuleFromFile
    from paraview.vtk.vtkParallelCore import vtkPSystemTools

    from . import importers
    # plug into Python import machinery to help import pipeline modules
    # seamlessly.
    importers.install_pathfinder()

    if vtkPSystemTools.FileIsDirectory(path):
        return RegisterPackageFromDir(path)
    elif path.lower().endswith(".zip"):
        return RegisterPackageFromZip(path)
    else:
        return RegisterModuleFromFile(path)


class CatalystV1Information:
    """
    Provides information to the current `catalyst_execute` call.
    """

    def __init__(self, dataDescription):
        self._dataDescription = dataDescription

    @property
    def time(self):
        """returns the current simulation time"""
        return self._dataDescription.GetTime()

    @property
    def timestep(self):
        """returns the current simulation cycle or timestep index"""
        return self._dataDescription.GetTimeStep()

    @property
    def cycle(self):
        """returns the current simulation cycle or timestep index"""
        return self._dataDescription.GetTimeStep()

    @property
    def dataDescription(self):
        """avoid using this unless absolutely sure what you're doing"""
        return self._dataDescription


class CatalystV2Information:
    def __init__(self):
        from paraview.modules.vtkPVInSitu import vtkInSituInitializationHelper
        from paraview.modules.vtkPVInSitu import vtkInSituPythonConduitHelper
        self.helper = vtkInSituInitializationHelper
        self.helperConduitNode = vtkInSituPythonConduitHelper

    @property
    def time(self):
        """returns the current simulation time"""
        return self.helper.GetTime()

    @property
    def timestep(self):
        """returns the current simulation cycle or timestep index"""
        return self.helper.GetTimeStep()

    @property
    def cycle(self):
        """returns the current simulation cycle or timestep index"""
        return self.helper.GetTimeStep()

    @property
    def catalyst_params(self):
        """ returns a reference to the arguments (as a conduit node) passed in
        `catalyst_execute` for this timestep.  This reference should be treated
        as a READ-ONLY since any change will affect the data passed by the
        simulation.

        Note: This call will import the catalyst_conduit python package tha
        ships with catalyst. So make sure that your PYTHONPATH includes it.
        The path is available in the cmake variable CATALYST_PYTHONPATH during
        configuration.
        """
        return self.helperConduitNode.GetCatalystParameters()


def import_and_validate(modulename):
    import importlib

    m = importlib.import_module(modulename)
    _validate_and_initialize(m)
    return m


def do_request_data_description(module, dataDescription):
    log(log_level(), "called do_request_data_description %r", module)

    # call 'RequestDataDescription' on module, if exists
    if hasattr(module, "RequestDataDescription"):
        log(log_level(), "calling '%s.%s'", module.__name__, "RequestDataDescription")
        module.RequestDataDescription(dataDescription)
        return True  # indicates RequestDataDescription was overridden in the script.
    return False


def do_catalyst_initialize(module):
    log(log_level(), "called do_catalyst_initialize %r", module)

    # call 'catalyst_initialize' on module if exits
    if hasattr(module, "catalyst_initialize"):
        log(log_level(), "calling '%s.%s'", module.__name__, "catalyst_initialize")
        module.catalyst_initialize()


def do_catalyst_finalize(module):
    log(log_level(), "called do_catalyst_finalize %r", module)

    # call 'catalyst_finalize' on module if exits
    if hasattr(module, "catalyst_finalize"):
        log(log_level(), "calling '%s.%s'", module.__name__, "catalyst_finalize")
        module.catalyst_finalize()


def do_catalyst_results(module):
    log(log_level(), "called do_catalyst_results %r", module)

    # call 'catalyst_results' on module if exits
    if hasattr(module, "catalyst_results"):
        log(log_level(), "calling '%s.%s'", module.__name__, "catalyst_results")
        info = CatalystV2Information()
        module.catalyst_results(info)


def do_catalyst_execute(module):
    log(log_level(), "called do_catalyst_execute %r", module)

    # call 'catalyst_execute' on module, if exists
    if hasattr(module, "catalyst_execute"):
        log(log_level(), "calling '%s.%s'", module.__name__, "catalyst_execute")

        dataDescription = _get_active_data_description()
        if dataDescription:
            info = CatalystV1Information(dataDescription)
        else:
            info = CatalystV2Information()
        module.catalyst_execute(info)
        return True

    return False


def _get_active_data_description():
    helper = vtkCPPythonScriptV2Helper.GetActiveInstance()
    return helper.GetDataDescription()


def _get_active_arguments():
    args = []
    helper = vtkCPPythonScriptV2Helper.GetActiveInstance()
    if not helper:
        # happens when script is executed in pvbatch, and not catalyst
        return args
    slist = helper.GetArgumentsAsStringList()
    for cc in range(slist.GetLength()):
        args.append(slist.GetString(cc))
    return args


def _get_execute_parameters():
    params = []
    helper = vtkCPPythonScriptV2Helper.GetActiveInstance()
    if not helper:
        # happens when script is executed in pvbatch, and not catalyst
        return params
    slist = helper.GetParametersAsStringList()
    for cc in range(slist.GetLength()):
        params.append(slist.GetString(cc))
    return params


def _get_script_filename():
    helper = vtkCPPythonScriptV2Helper.GetActiveInstance()
    if not helper:
        return None
    return helper.GetScriptFileName()


def has_customized_execution(module):
    return hasattr(module, "catalyst_execute") or \
        hasattr(module, "RequestDataDescription") or \
        hasattr(module, "catalyst_initialize")


def _validate_and_initialize(module):
    """Validates a module to confirm that it is a `module` that we can treat as
    a Catalyst script. Additionally, adds some private meta-data that this
    module will use during exection."""
    if not hasattr(module, "options"):
        print_warning("Module '%s' missing Catalyst 'options', will use a default options object", module.__name__)
        from . import Options
        module.options = Options()
    # provide the options to vtkCPPythonScriptV2Helper.
    helper = vtkCPPythonScriptV2Helper.GetActiveInstance()
    helper.SetOptions(module.options.SMProxy)
