r"""
Module intended to provide functions used in Catalyst scripts. This is public
API and must preserve backwards compatibility so that older versions of Catalyst
scripts continue to work as ParaView code keeps evolving. Hence, we limit the
API exposed here to bare minimum.
"""


def Options():
    """Creates and returns an options object which is used to configure Catalyst
    specific options such as output directories, live support, etc."""
    # we import simple to ensure that the active connection is created
    from paraview import servermanager, simple
    pxm = servermanager.ProxyManager()
    proxy = pxm.NewProxy("coprocessing", "CatalystOptions")
    return servermanager._getPyProxy(proxy)


def log_level():
    """Returns the Python logging level to use to log informative messages
    from this package"""
    from paraview.modules.vtkPVVTKExtensionsCore import vtkPVLogger
    from paraview.detail.loghandler import get_level
    return get_level(vtkPVLogger.GetCatalystVerbosity())


def get_args():
    """For the active pipeline script, returns "args", if any, specified
    when the script initialized in the Catalyst adaptor.

    This is currently only supported for adaptors that use Catalyst 2.0 Adaptor
    API. For legacy adaptors, this will simply return an empty list.

    Return value is a list of strings.
    """
    from . import v2_internals
    return v2_internals._get_active_arguments()


def get_execute_params():
    """For the active pipeline script, returns "parameters", if any, specified
    during the execute phase in the Catalyst adaptor.

    This is currently only supported for adaptors that use Catalyst 2.0 Adaptor
    API. For legacy adaptors, this will simply return an empty list.

    Return value is a list of strings.
    """
    from . import v2_internals
    return v2_internals._get_execute_parameters()


def get_script_filename():
    """For the active pipeline script, returns its filename. This is provided in
    the same form as it was passed to
    `vtkCPPythonScriptV2Helper::PrepareFromScript`.
    """
    from . import v2_internals
    return v2_internals._get_script_filename()
