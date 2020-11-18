r"""
Internal functions not intended for use by users. This may change without
notice.
"""
from .. import logger

from ..modules.vtkPVInSitu import vtkInSituInitializationHelper, vtkInSituPipelinePython

from ..modules.vtkPVPythonCatalyst import vtkCPPythonScriptV2Helper

def _get_active_helper():
    return vtkCPPythonScriptV2Helper.GetActiveInstance()


def _get_active_data_description():
    if not IsInsitu():
        return None
    helper = _get_active_helper()
    return helper.GetDataDescription()


def IsInsitu():
    """Returns True if executing in an insitu environment, else false"""
    helper = _get_active_helper()
    return (helper is not None)


def IsLegacyCatalystAdaptor():
    """Returns True if the active execution environment is from within a legacy
    Catalyst adaptor implementation that uses vtkCPProcessor etc. manually,
    instead of the Conduit-based in situ API"""
    return _get_active_data_description() is not None

def IsCatalystInSituAPI():
    """Returns True if the active execution environment is from within
    an implementation of the Conduit-based Catalyst In Situ API."""
    return IsInsitu() and not IsLegacyCatalystAdaptor() and vtkInSituInitializationHelper.IsInitialized()

def IsInsituInput(name):
    if not name or not IsInsitu():
        return False
    dataDesc = _get_active_data_description()
    if dataDesc:
        # Legacy Catalyst
        if dataDesc.GetInputDescriptionByName(name) is not None:
            return True
    elif IsCatalystInSituAPI() and (vtkInSituInitializationHelper.GetProducer(name) is not None):
        # Catalyst 2.0
        return True
    return False


def RegisterExtractor(extractor):
    """Keeps track of extractors created inside a specific Catalyst
    script.  This is useful to ensure we only update the extractors for that
    current script when multiple scripts are being executed in the same run.
    """
    assert IsInsitu()
    _get_active_helper().RegisterExtractor(extractor.SMProxy)


def RegisterView(view):
    """Keeps track of views created inside a specific Catalyst
    script.  This is useful to ensure we only update the views for the
    current script when multiple scripts are being executed in the same run.
    """
    assert IsInsitu()
    _get_active_helper().RegisterView(view.SMProxy)

    if IsCatalystInSituAPI():
        view.ViewTime = vtkInSituInitializationHelper.GetTime()
    else:
        view.ViewTime = _get_active_data_description().GetTime()


def CreateProducer(name):
    assert IsInsituInput(name)
    from . import log_level
    from .. import log
    log(log_level(), "creating producer for simulation input named '%s'")
    if IsCatalystInSituAPI():
        # Catalyst 2.0
        from paraview import servermanager
        producer = servermanager._getPyProxy(vtkInSituInitializationHelper.GetProducer(name))

        # since state file may have arbitrary properties being specified
        # on the original source, we ensure we ignore them
        producer.IgnoreUnknownSetRequests = True
        return producer

    # Legacy Catalyst
    from paraview import servermanager
    helper = _get_active_helper()
    producer = helper.GetTrivialProducer(name)
    producer = servermanager._getPyProxy(producer)

    # since state file may have arbitrary properties being specified
    # on the original source, we ensure we ignore them
    producer.IgnoreUnknownSetRequests = True
    return producer


def InitializePythonEnvironment():
    """
    This is called in vtkCPPythonPipeline to initialize the Python
    environment to be more suitable for Catalyst in situ execution.
    """
    from . import log_level
    from .. import log

    log(log_level(), "initializing Python environment for in situ")

    # disable writing bytecode, we don't want to generate pyc files
    # when running in situ.
    log(log_level(), "disable byte-code generation")
    import sys
    sys.dont_write_bytecode = True

    # ensure we can import ParaView servermanager and necessary modules
    log(log_level(), "import paraview.servermanager")
    from paraview import servermanager

    # import Python wrapping for vtkPVCatalyst module
    log(log_level(), "import paraview.modules.vtkPVCatalyst")
    from paraview.modules import vtkPVCatalyst

def RegisterPackageFromZip(zipfilename, packagename=None):
    from . import importers
    zipfilename = _mpi_exchange_if_needed(zipfilename)
    return importers.add_file(zipfilename, packagename)


def RegisterPackageFromDir(path):
    import os.path
    from . import importers
    packagename = os.path.basename(path)
    init_py = os.path.join(path, "__init__.py")
    return importers.add_file(init_py, packagename)


def RegisterModuleFromFile(filename):
    from . import importers
    return importers.add_file(filename)


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
