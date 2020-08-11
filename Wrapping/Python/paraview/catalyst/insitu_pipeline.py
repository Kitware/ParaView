r"""
INTERNAL MODULE, NOT FOR PUBLIC CONSUMPTION.

Used by vtkInSituPipelinePython and may change without notice.
"""

from . import log_level
from .. import log, print_warning

from paraview.modules.vtkPVInSitu import vtkInSituPipelinePython, vtkInSituInitializationHelper

def load_module(path):
    from paraview.vtk.vtkParallelCore import vtkPSystemTools
    if vtkPSystemTools.FileIsDirectory(path):
        return load_package_from_dir(path)
    elif path.lower().endswith(".zip"):
        return load_package_from_zip(path)
    elif vtkPSystemTools.FileExists(path):
        return load_module_from_file(path)
    else:
        return None

def load_package_from_zip(zipfilename, packagename=None):
    """Loads a zip file and imports a top-level package from it with the name
    `packagename`. If packagename is None, then the basename for the zipfile is
    used as the package name.

    If import fails, this will throw appropriate Python import errors.

    :returns: the module object for the package on success else None
    """
    from .detail import LoadPackageFromZip
    module = LoadPackageFromZip(zipfilename, packagename)
    _validate_and_initialize(module, is_zip=True)
    return module

def load_package_from_dir(path):
    from .detail import LoadPackageFromDir
    module = LoadPackageFromDir(path)
    _validate_and_initialize(module, is_zip=True)
    return module

def load_module_from_file(fname):
    from .detail import LoadModuleFromFile
    module = LoadModuleFromFile(fname)
    _validate_and_initialize(module, is_zip=True)
    return module


def do_initialize(vtk_pipeline, module):
    log(log_level(), "in do_initialize(%s)", str(module))
    # Pass the Catalyst Options proxy back to vtkInSituPipelinePython instance.
    # vtkInSituPipelinePython uses that to determine locations for output directories
    # etc.
    vtk_pipeline.SetOptions(module.options.SMProxy)

    # if module has function named `catalyst_initialize`, call it.
    if hasattr(module, "catalyst_initialize"):
        module.catalyst_initialize()

    # load analysis scripts, if any.
    _load_analysis_scripts(module)

    for submodule in module.catalyst_params["analysis_modules"]:
        if hasattr(submodule, "catalyst_initialize"):
            submodule.catalyst_initialize()

def do_execute(module):
    log(log_level(), "in do_execute(%s)", str(module))

    # create an object that provides metadata about the current
    # execute call. While totally redundant since that information can be
    # glean from elsewhere, it makes catalyst scripts cleaner and is extensible.
    class _information:
        @property
        def time(self):
            """returns the current simulation time"""
            return vtkInSituInitializationHelper.GetTime()
        @property
        def timestep(self):
            """returns the current simulation cycle or timestep index"""
            return vtkInSituInitializationHelper.GetTimeStep()
        @property
        def cycle(self):
            """returns the current simulation cycle or timestep index"""
            return vtkInSituInitializationHelper.GetTimeStep()

    info = _information()

    # if module has function named `catalyst_execute`, call it.
    if hasattr(module, "catalyst_execute"):
        module.catalyst_execute(info)

    for submodule in module.catalyst_params["analysis_modules"]:
        if hasattr(submodule, "catalyst_execute"):
            submodule.catalyst_execute(info)


def do_finalize(module):
    log(log_level(), "in do_finalize(%s)", str(module))

    for submodule in module.catalyst_params["analysis_modules"]:
        if hasattr(submodule, "catalyst_finalize"):
            submodule.catalyst_finalize()

    # if module has function named `catalyst_finalize`, call it.
    if hasattr(module, "catalyst_finalize"):
        module.catalyst_finalize()


def _validate_and_initialize(module, is_zip):
    """Validates a module to confirm that it is a `module` that we can treat as
    a Catalyst script. Additionally, adds some private meta-data that this
    module will use during exection."""
    module.catalyst_params = {}
    module.catalyst_params["analysis_modules"] = None
    module.catalyst_params["is_zip"] = is_zip
    if not hasattr(module, "options"):
        print_warning("Module '%s' missing Catalyst 'options', will use a default options object", module.__name__)
        from . import Options
        module.options = Options()


def _load_analysis_scripts(module):
    from .detail import LoadSubmodule
    params  = module.catalyst_params
    if params["analysis_modules"] is not None:
        # already loaded, nothing to do.
        return
    if not hasattr(module, "scripts"):
        # module has no scripts
        params["analysis_modules"] = []
        return

    log(log_level(), "loading analysis modules '%s'", str(module.scripts))
    analysis_modules = []
    for script in module.scripts:
        s_module = LoadSubmodule(script, module)
        analysis_modules.append(s_module)
    # check if any of the analysis_modules customized execution.
    for m in analysis_modules:
        if _has_customized_execution(m):
            params["customized_execution"] = True
            break
    params["analysis_modules"] = analysis_modules
