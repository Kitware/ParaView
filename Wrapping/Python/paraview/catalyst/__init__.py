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
