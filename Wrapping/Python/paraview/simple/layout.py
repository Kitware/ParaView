import paraview
from paraview import servermanager

from paraview.util import proxy as proxy_util

from paraview.simple.session import GetActiveView
from paraview.simple.view import GetViews, GetRenderViews

# -----------------------------------------------------------------------------


def RenameLayout(newName, proxy=None):
    """Renames the given layout. If the given proxy is not registered
    in the 'layouts' group this method will have no effect.

    :param newName: The new name of the layout proxy
    :type newName: str
    :param layout: The layout proxy to rename. It must be a member
        of the 'layouts' groupd. Optional, defaults to renaming the active layout."""
    if not proxy:
        proxy = GetLayout()
    proxy_util.rename(proxy, "layouts", newName)


def CreateLayout(name=None):
    """Create a new layout with no active view.

    :param name: The name of the layout. Optional, defaults to an automatically
        created name."""
    layout = servermanager.misc.ViewLayout(registrationGroup="layouts")
    if name:
        RenameLayout(name, layout)
    return layout


# -----------------------------------------------------------------------------


def RemoveLayout(proxy=None):
    """Remove the provided layout. If none is provided, remove the layout
    containing the active view. If it is the only layout it will create a new
    one with the same name as the removed one.

    :param proxy: The layout proxy to remove. Optional, defaults to the
        layout containing the active view.
    :type proxy: Layout proxy or `None`."""
    pxm = servermanager.ProxyManager()
    if not proxy:
        proxy = GetLayout()
    name = pxm.GetProxyName("layouts", proxy)
    pxm.UnRegisterProxy("layouts", name, proxy)
    if len(GetLayouts()) == 0:
        CreateLayout(name)


# -----------------------------------------------------------------------------


def GetLayouts():
    """Returns all the layout proxies.

    :return: A list of all the layouts.
    :rtype: list of layout proxies"""
    return servermanager.ProxyManager().GetProxiesInGroup("layouts")


# -----------------------------------------------------------------------------


def GetLayout(view=None):
    """Return the layout containing the given view, if any.

    :param view: A view in the layout to be removed. Optional, defaults to the
        active view.
    :return: The layout containing the view
    :rtype: :class:`paraview.servermanager.ViewLayout`"""
    if not view:
        view = GetActiveView()
    if not view:
        raise RuntimeError("No active view was found.")
    lproxy = servermanager.vtkSMViewLayoutProxy.FindLayout(view.SMProxy)
    return servermanager._getPyProxy(lproxy)


def GetLayoutByName(name):
    """Return the first layout with the given name, if any.

    :param name: Name of the layout
    :type name: str
    :return: The named layout if it exists
    :rtype: Layout proxy or `None`"""
    layouts = GetLayouts()
    for key in layouts.keys():
        if key[0] == name:
            return layouts.get(key)
    return None


def GetViewsInLayout(layout=None):
    """Returns a list of views in the given layout.

    :param layout: Layout whose views should be returned. Optional, defaults to the
        layout for the active view, if possible.
    :type layout: Layout proxy.
    :return: List of views in the layout.
    :rtype: list"""
    layout = layout if layout else GetLayout()
    if not layout:
        raise RuntimeError(
            "Layout couldn't be determined. Please specify a valid layout."
        )
    views = GetViews()
    return [x for x in views if layout.GetViewLocation(x) != -1]


def AssignViewToLayout(view=None, layout=None, hint=0):
    """Assigns the view provided to the layout provided. If no layout exists,
    then a new layout will be created.

    It is an error to assign the same view to multiple layouts.

    :param view: The view to assign to the layout. Optional, defaults to the active view.
    :type view: View proxy.
    :param layout: If layout is `None`, then either the active layout or an
                   existing layout on the same server will be used.
    :type layout: Layout proxy.
    :return: Returns `True` on success.
    :rtype: bool

    """
    view = view if view else GetActiveView()
    if not view:
        raise RuntimeError("No active view was found.")

    layout = layout if layout else GetLayout()
    controller = servermanager.ParaViewPipelineController()
    return controller.AssignViewToLayout(view, layout, hint)


# -----------------------------------------------------------------------------


def RemoveViewsAndLayouts():
    """
    Removes all existing views and layouts.

    """
    pxm = servermanager.ProxyManager()
    layouts = pxm.GetProxiesInGroup("layouts")

    for view in GetRenderViews():
        proxy_util.unregister(view)

    # Can not use regular delete for layouts
    for name, id in layouts:
        proxy = layouts[(name, id)]
        pxm.UnRegisterProxy("layouts", name, proxy)


def EqualizeViewsHorizontally(layout=None):
    """Equalizes horizontal view sizes in the provided layout.

    :param layout: Layout the layout contain the views to equalize. Optional, defaults
                   to the active layout.
    :type layout: Layout proxy.
    """
    layout = layout if layout else GetLayout()
    layout.SMProxy.EqualizeViews(
        paraview.modules.vtkRemotingViews.vtkSMViewLayoutProxy.HORIZONTAL
    )


def EqualizeViewsVertically(layout=None):
    """Equalizes vertical view sizes in the provided layout.

    :param layout: Layout the layout contain the views to equalize. Optional, defaults
                   to the active layout.
    :type layout: Layout proxy.
    """
    layout = layout if layout else GetLayout()
    layout.SMProxy.EqualizeViews(
        paraview.modules.vtkRemotingViews.vtkSMViewLayoutProxy.VERTICAL
    )


def EqualizeViewsBoth(layout=None):
    """Equalizes the vertical and horizontal view sizes in the provided layout.

    :param layout: Layout the layout contain the views to equalize. Optional, defaults
                   to the active layout.
    :type layout: Layout proxy.
    """
    layout = layout if layout else GetLayout()
    layout.SMProxy.EqualizeViews()
