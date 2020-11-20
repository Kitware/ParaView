r"""
This module can be used in Python-based apps (or simulations) to simplify the
code that such apps need to incorporate into their code to use Catalyst.
"""

# note, here we deliberately don't import paraview.servermanager and the ilk
# since we don't want to initialize ParaView yet.
import paraview
from paraview.modules.vtkPVCatalyst import vtkCPProcessor, vtkCPDataDescription
from paraview.modules.vtkPVPythonCatalyst import vtkCPPythonScriptV2Pipeline, vtkCPPythonScriptPipeline, vtkCPPythonPipeline
from paraview.modules.vtkRemotingCore import vtkProcessModule


def _sanity_check():
    pm = vtkProcessModule.GetProcessModule()
    if pm and pm.GetPartitionId() == 0:
        paraview.print_warning(\
            "Warning: ParaView has been initialized before `initialize` is called")

coprocessor = None

def initialize():
    # if ParaView is already initialized, some of the options we set here don't
    # get used, hence it's a good thing to warn about it. It's not an error,
    # but something that users (and developers) should realize.

    # Note, this happens when pvpython/pvbatch is used to execute the Python scripts.
    # Once we unity ParaView initialization in these executables and standard
    # Python interpreter, this will not be an issue.
    _sanity_check()

    # initialize ParaView's ServerManager. To do that, we set some
    # variables that control how the ParaView will get initialized
    # and then simply `import paraview.servermanager`
    paraview.options.batch = True
    paraview.options.symmetric = True

    global coprocessor
    coprocessor = vtkCPProcessor()
    if not coprocessor.Initialize():
        raise RuntimeError("Failed to initialize Catalyst")

def add_pipeline_legacy(scriptname):
    global coprocessor
    assert coprocessor is not None

    pipeline = vtkCPPythonScriptPipeline()
    if not pipeline.Initialize(scriptname):
        raise RuntimeError("Initialization failed!")

    coprocessor.AddPipeline(pipeline)

def add_pipeline_v2(path):
    import os.path

    global coprocessor
    assert coprocessor is not None

    pipeline = vtkCPPythonScriptV2Pipeline()
    if not pipeline.Initialize(path):
        raise RuntimeError("Initialization failed!")
    coprocessor.AddPipeline(pipeline)

def add_pipeline(filename, version=0):
    import os.path

    # if version is specified, use that.
    if int(version) == 1:
        add_pipeline_legacy(filename)

    elif int(version) == 2:
        add_pipeline_v2(filename)

    elif int(version) == 0:
        # if not, try to determine version using the filename.
        version = vtkCPPythonPipeline.DetectScriptVersion(filename)
        if version == 0:
            # default to version 1.0
            version = 1
        return add_pipeline(filename, version)

    else:
        raise RuntimeError("Invalid version '%d'" % version)

def coprocess(time, timestep, dataset, name="input", wholeExtent=None):
    global coprocessor

    dataDesc = vtkCPDataDescription()
    dataDesc.AddInput(name)
    dataDesc.SetTimeData(time, timestep)

    idd = dataDesc.GetInputDescriptionByName(name)
    if wholeExtent is not None:
        idd.SetWholeExtent(wholeExtent)
    if not coprocessor.RequestDataDescription(dataDesc):
        # nothing to do.
        return False
    idd.SetGrid(dataset)
    coprocessor.CoProcess(dataDesc)

def finalize():
    global coprocessor
    coprocessor.Finalize()
