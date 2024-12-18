from paraview import servermanager
from paraview.util import proxy as proxy_util

from paraview.simple.session import GetActiveView, GetActiveSource
from paraview.simple.rendering import GetDisplayProperties
from paraview.simple.view import CreateView, GetViews
from paraview.simple.layout import AssignViewToLayout


def RenameProxy(proxy, group, newName):
    """Renames the given proxy. This is the name used by :func:`FindSource` and
    is displayed in the Pipeline Browser.

    :param proxy: The proxy to be renamed
    :type proxy: Proxy object
    :param group: The group in which the proxy lives. Can be retrieved with
        `proxy.GetXMLGroup()`
    :type group: str
    :param newName: The new name of the proxy.
    :type newName: str"""
    proxy_util.rename(proxy, group, newName)


def RenameSource(newName, proxy=None):
    """Renames the given source proxy. If the given proxy is not registered
    in the 'sources' group this function will have no effect.

    :param newName: The new name of the source proxy
    :type newName: str
    :param proxy: The source proxy to rename. It must be a member
        of the 'sources' group. Optional, defaults to renaming the active source.
    :type proxy: Source proxy"""
    if not proxy:
        proxy = GetActiveSource()
    RenameProxy(proxy, "sources", newName)


def RenameView(newName, proxy=None):
    """Renames the given view. If the given proxy is not registered
    in the 'views' group this method will have no effect.

    :param newName: The new name of the view proxy
    :type newName: str
    :param proxy: The view proxy to rename. It must be a member
        of the 'views' group. Optional, defaults to renaming the active view.
    :type proxy: View proxy"""
    if not proxy:
        proxy = GetActiveView()
    RenameProxy(proxy, "views", newName)


# -----------------------------------------------------------------------------


def FindSource(name):
    """Return a pipeline source based on the name that was used to register it
    with ParaView.

    Example usage::

       Cone(guiName='MySuperCone')
       Show()
       Render()
       myCone = FindSource('MySuperCone')

    :param name: The name that the pipeline source was registered with during
        creation or after renaming.
    :type name: str
    :return: The pipeline source if found.
    :rtype: Pipeline source proxy."""
    return servermanager.ProxyManager().GetProxy("sources", name)


def FindView(name):
    """Return a view proxy based on the name that was used to register it with
    ParaView.

    Example usage::

       CreateRenderView(guiName='RenderView1')
       myView = FindSource('RenderView1')

    :param name: The name that the view was registered with during creation or
        after renaming.
    :type name: str
    :return: The view if found.
    :rtype: View proxy."""
    return servermanager.ProxyManager().GetProxy("views", name)


def GetActiveViewOrCreate(viewtype):
    """
    Returns the active view if the active view is of the given type,
    otherwise creates a new view of the requested type. Note, if a new view is
    created, it will be assigned to a layout by calling :func:`AssignViewToLayout`.

    :param viewtype: The type of view to access if it is the active view or create if it doesn't exist.
    :type viewtype: str

    :return: The active view if it is of type `viewtype`.
    :rtype: View proxy.

    """
    view = GetActiveView()
    if view is None or view.GetXMLName() != viewtype:
        view = CreateView(viewtype)
        if view:
            # if a new view is created, we assign it to a layout.
            # Since this method gets used when tracing existing views, it makes
            # sense to assign it to a layout during playback.
            AssignViewToLayout(view)
    if not view:
        raise RuntimeError("Failed to create/locate the specified view")
    return view


def FindViewOrCreate(name, viewtype):
    """Returns the view if a view with the given name exists and is of the
    the given `viewtype`, otherwise creates a new view of the requested type.
    Note, if a new view is created, it will be assigned to a layout
    by calling `AssignViewToLayout`.

    :param name: Name of the view to find.
    :type name: str
    :param viewtype: Type of the view to create.
    :type viewtype: str"""
    view = FindView(name)
    if view is None or view.GetXMLName() != viewtype:
        view = CreateView(viewtype)
        if view:
            # if a new view is created, we assign it to a layout.
            # Since this method gets used when tracing existing views, it makes
            # sense to assign it to a layout during playback.
            AssignViewToLayout(view)
    if not view:
        raise RuntimeError("Failed to create/locate the specified view")
    return view


def LocateView(displayProperties=None):
    """Returns the view associated with the given `displayProperties`/representation
    object if it exists.

    :param displayProperties: a representation proxy returned by
        :func:`GetRepresentation()`, :func:`GetDisplayProperties()`, or :func:`Show()`
        functions. Optional, defaults to the active representation.
    :type displayProperties: representation proxy
    :return: The view associated with the representation if it exists, otherwise `None`
    :rtype: View proxy or `None`"""
    if displayProperties is None:
        displayProperties = GetDisplayProperties()
    if displayProperties is None:
        raise ValueError("'displayProperties' must be set")
    for view in GetViews():
        try:
            if displayProperties in view.Representations:
                return view
        except AttributeError:
            pass
    return None


# -----------------------------------------------------------------------------


def GetSources():
    """Get all available pipeline sources.

    :return: dictionary of pipeline sources. Keys are tuples consisting of the registration
             name and integer ID of the proxy, and values are the pipeline sources themselves.
    :rtype: dictionary

    """
    return servermanager.ProxyManager().GetProxiesInGroup("sources")


# -----------------------------------------------------------------------------


def GetRepresentations():
    """Returns all available representation proxies (display properties) in all views.

    :return: dictionary of representations. Keys are tuples consisting of the
             registration name and integer ID of the representation proxy, and values
             are the representation proxies themselves.
    :rtype: dict

    """
    return servermanager.ProxyManager().GetProxiesInGroup("representations")


# -----------------------------------------------------------------------------


def UpdatePipeline(time=None, proxy=None):
    """Updates (executes) the given pipeline object for the given time as
    necessary (i.e., if it did not already execute).

    :param time: The time at which to update the pipeline. Optional, defaults
        to updating the currently loaded timestep.
    :type time: float
    :param proxy: Source proxy to update. Optional, defaults to updating the
        active source.
    :type proxy: Source proxy."""
    if not proxy:
        proxy = GetActiveSource()
    if time:
        proxy.UpdatePipeline(time)
    else:
        proxy.UpdatePipeline()


# -----------------------------------------------------------------------------


def Delete(proxy=None):
    """Deletes the given pipeline source or the active source if no argument
    is specified.

    :param proxy: the proxy to remove
    :type proxy: Source proxy. Optional, defaults to deleting the active source."""
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError("Could not locate proxy to 'Delete'")

    proxy_util.unregister(proxy)


# -----------------------------------------------------------------------------


def ResetProperty(propertyName, proxy=None, restoreFromSettings=True):
    """Resets a proxy property to its default value.

    :param propertyName: Name of the property to reset.
    :type propertyName: str
    :param proxy: The proxy whose property should be reset. Optional, defaults to the
        active source.
    :param restoreFromSettings: If `True`, the property will be reset to the default
        value stored in the settings if available, otherwise it will be reset to
        ParaView's application default value. Optional, defaults to `True`.
    :type restoreFromSettings: bool"""
    if proxy == None:
        proxy = GetActiveSource()

    propertyToReset = proxy.SMProxy.GetProperty(propertyName)

    if propertyToReset != None:
        propertyToReset.ResetToDefault()

        if restoreFromSettings:
            settings = servermanager.vtkSMSettings.GetInstance()
            settings.GetPropertySetting(propertyToReset)

        proxy.SMProxy.UpdateVTKObjects()
