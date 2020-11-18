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

    modulename = None
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
        self.helper = vtkInSituInitializationHelper

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

def import_and_validate(modulename):
    import importlib

    # to support multiple pipelines, we create a container
    # that keeps all proxies (extractors, and producers) needed by this pipeline
    # in this helper container.
    class Container:
        pass

    m = importlib.import_module(modulename)
    _validate_and_initialize(m)
    return m

def do_request_data_description(module, dataDescription):
    log(log_level(), "called do_request_data_description %r", module)

    # call 'RequestDataDescription' on module, if exists
    if hasattr(module, "RequestDataDescription"):
        log(log_level(), "calling '%s.%s'", module.__name__, "RequestDataDescription")
        module.RequestDataDescription(dataDescription)
        return True # indicates RequestDataDescription was overridden in the script.
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
