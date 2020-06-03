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
