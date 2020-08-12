r"""
Internal functions not intended for use by users. This may change without
notice.
"""
from .. import logger

ActiveDataDescription = None
ActivePythonPipelineModule = None

def SetActiveDataDescription(dataDesc, module):
    global ActiveDataDescription, ActivePythonPipelineModule
    ActiveDataDescription = dataDesc
    ActivePythonPipelineModule = module

def IsInsitu():
    """Returns True if executing in an insitu environment, else false"""
    global ActiveDataDescription
    return ActiveDataDescription is not None

def IsInsituInput(name):
    global ActiveDataDescription
    if not name or not IsInsitu():
        return False
    if ActiveDataDescription.GetInputDescriptionByName(name) is not None:
        return True
    return False

def RegisterExtractGenerator(generator):
    """Keeps track of extract generators created inside a specific Catalyst
    script.  This is useful to ensure we only update the generators for that
    current script when multiple scripts are being executed in the same run.
    """
    global ActivePythonPipelineModule
    assert IsInsitu()

    module = ActivePythonPipelineModule
    if not hasattr(module, "_extract_generators"):
        from vtkmodules.vtkCommonCore import vtkCollection
        module._extract_generators = vtkCollection()
    module._extract_generators.AddItem(generator.SMProxy)

def CreateProducer(name):
    global ActiveDataDescription, ActivePythonPipelineModule
    assert IsInsituInput(name)

    module = ActivePythonPipelineModule
    if not hasattr(module, "_producer_map"):
        module._producer_map = {}
    if name in module._producer_map:
        return module[name]

    from paraview import servermanager
    dataDesc = ActiveDataDescription
    ipdesc = dataDesc.GetInputDescriptionByName(name)

    # eventually, we want the Catalyst C++ code to give use the vtkAlgorithm to
    # use; e.g.
    # servermanager._getPyProxy(ipdesc.GetProducer())

    pxm = servermanager.ProxyManager()
    producer = servermanager._getPyProxy(pxm.NewProxy("sources", "PVTrivialProducer2"))
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeProxy(producer)
    controller.RegisterPipelineProxy(producer, name)

    # since state file may have arbitrary properties being specified
    # on the original source, we ensure we ignore them
    producer.IgnoreUnknownSetRequests = True

    vtkobject = producer.GetClientSideObject()
    assert vtkobject
    vtkobject.SetWholeExtent(ipdesc.GetWholeExtent())
    vtkobject.SetOutput(ipdesc.GetGrid())
    module._producer_map[name] = producer
    return producer


def UpdateProducers():
    global ActiveDataDescription, ActivePythonPipelineModule
    assert IsInsitu()
    dataDesc = ActiveDataDescription
    module = ActivePythonPipelineModule
    if not hasattr(module, "_producer_map"):
        # implies that this state file has no producer!
        # it is possibly, but most likely an error; let's warn.
        logger.warning("script may not depend on simulation data; is that expected?")
        return False

    for name, producer in module._producer_map.items():
        ipdesc = dataDesc.GetInputDescriptionByName(name)
        assert ipdesc

        vtkobject = producer.GetClientSideObject()
        assert vtkobject
        vtkobject.SetOutput(ipdesc.GetGrid(), dataDesc.GetTime())
        vtkobject.SetWholeExtent(ipdesc.GetWholeExtent())
        producer.MarkModified(producer.SMProxy)
    return True


def HasProducers():
    global ActiveDataDescription, ActivePythonPipelineModule
    assert IsInsitu()
    dataDesc = ActiveDataDescription
    module = ActivePythonPipelineModule
    return hasattr(module, "_producer_map")

def IsAnyTriggerActivated(cntr):
    global ActivePythonPipelineModule
    assert IsInsitu()

    module = ActivePythonPipelineModule
    if hasattr(module, "_extract_generators"):
        return cntr.IsAnyTriggerActivated(module._extract_generators)

    # if there are no extract generators in this module, what should we do?
    # I am leaning towards saying treat it as if they are activated since
    # the user may have custom extract generation code.
    from . import log_level
    from .. import log
    log(log_level(), "module has no extract generators, treating as activated.")
    return True

def Extract(cntr):
    global ActivePythonPipelineModule
    assert IsInsitu()

    module = ActivePythonPipelineModule
    if hasattr(module, "_extract_generators"):
        return cntr.Extract(module._extract_generators)
    return False

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


def LoadPackageFromZip(zipfilename, packagename=None):
    """Loads a zip file and imports a top-level package from it with the name
    `packagename`. If packagename is None, then the basename for the zipfile is
    used as the package name.

    If import fails, this will throw appropriate Python import errors.

    :returns: the module object for the package on success else None
    """
    import zipimport, os.path

    zipfilename = _mpi_exchange_if_needed(zipfilename)
    if packagename:
        package = packagename
    else:
        basename = os.path.basename(zipfilename)
        package = os.path.splitext(basename)[0]

    z = zipimport.zipimporter(zipfilename)
    assert z.is_package(package)
    return z.load_module(package)


def LoadPackageFromDir(path):
    import importlib.util, os.path
    packagename = os.path.basename(path)
    init_py = os.path.join(path, "__init__.py")
    spec = importlib.util.spec_from_file_location(packagename, init_py)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def LoadModuleFromFile(fname):
    import importlib.util, os.path
    fname = _mpi_exchange_if_needed(fname)
    modulename = os.path.basename(fname)
    spec = importlib.util.spec_from_file_location(modulename, fname)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

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
