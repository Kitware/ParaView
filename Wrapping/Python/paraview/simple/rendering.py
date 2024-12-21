import paraview
from paraview import servermanager
from paraview.util import proxy as proxy_util

from paraview.simple.session import (
    GetActiveView,
    GetActiveSource,
    IsFirstRender,
    ResetFirstRender,
)
from paraview.simple.view import GetViews

# -----------------------------------------------------------------------------


def Render(view=None):
    """Renders the given view if given, otherwise renders the active view. If
    this is the first render and the view is a RenderView, the camera is reset.

    :param view: The view to render. Optional, defaults to rendering the active
        view.
    :type view: View proxy."""
    if not view:
        view = GetActiveView()
    if not view:
        raise AttributeError("view cannot be None")
    # setup an interactor if current process support interaction if an
    # interactor hasn't already been set. This overcomes the problem where VTK
    # segfaults if the interactor is created after the window was created.
    view.MakeRenderWindowInteractor(True)
    view.StillRender()
    if IsFirstRender():
        # Not all views have a ResetCamera method
        try:
            view.ResetCamera()
            view.StillRender()
        except AttributeError:
            pass
        ResetFirstRender()
    return view


# -----------------------------------------------------------------------------
def RenderAllViews():
    """Renders all existing views."""
    for view in GetViews():
        Render(view)


# -----------------------------------------------------------------------------
def Interact(view=None):
    """Call this method to start interacting with a view. This method will
    block until the interaction is done. If the local process cannot support
    interactions, this method will simply return without doing anything.

    :param view: The interaction occurs with this view. Optional, defaults to
        the active view.
    :type view: View proxy."""
    if not view:
        view = GetActiveView()
    if not view:
        raise ValueError("view argument cannot be None")
    if not view.MakeRenderWindowInteractor(False):
        raise RuntimeError("Configuration doesn't support interaction.")
    paraview.print_debug_info("Staring interaction. Use 'q' to quit.")

    # Views like ComparativeRenderView require that Render() is called before
    # the Interaction is begun. Hence we call a Render() before start the
    # interactor loop. This also avoids the case where there are pending updates
    # and thus the interaction will be begun on stale datasets.
    Render(view)
    view.GetInteractor().Start()


# ==============================================================================
# Representation methods
# ==============================================================================


def GetRepresentation(proxy=None, view=None):
    """Given a pipeline proxy and a view, returns the corresponding representation object.
    If proxy and view are not specified, active objects are used.

    :param proxy: Pipeline proxy whose representation in the given view is
        requested. Optional, defaults to the active view.
    :type proxy: Source proxy
    :param view: The view associated with the requested representation. Optional,
        defaults to returning the representation associated with the active view.
    :type view: View proxy
    :return: The representation for the given proxy in the view.
    :rtype: Representation proxy
    """
    if not view:
        view = GetActiveView()
    if not view:
        raise ValueError("view argument cannot be None.")
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise ValueError("proxy argument cannot be None.")
    rep = servermanager.GetRepresentation(proxy, view)
    if not rep:
        controller = servermanager.ParaViewPipelineController()
        return controller.Show(proxy, proxy.Port, view)
    return rep


# -----------------------------------------------------------------------------
def GetDisplayProperties(proxy=None, view=None):
    """DEPRECATED: Should use `GetRepresentation()` instead."""
    return GetRepresentation(proxy, view)


# -----------------------------------------------------------------------------
def Show(proxy=None, view=None, representationType=None, **params):
    """Turns on the visibility of a given pipeline proxy in the given view.
    If pipeline proxy and/or view are not specified, active objects are used.

    :param proxy: The pipeline proxy to show. If not provided, uses the active source.
    :type proxy: Source proxy.
    :param view: The view proxy to show the source proxy in. Optional, defaults
        to the active view.
    :type view: View proxy.
    :param representationType: Name of the representation type to use. Optional,
        defaults to a suitable representation for the source proxy and view.
    :type representationType: str
    :return: The representation proxy for the source proxy in the view.
    :rtype: Representation proxy.:"""
    if proxy == None:
        proxy = GetActiveSource()
    if (
        not hasattr(proxy, "GetNumberOfOutputPorts")
        or proxy.GetNumberOfOutputPorts() == 0
    ):
        raise RuntimeError("Cannot show a sink i.e. algorithm with no output.")
    if proxy == None:
        raise RuntimeError(
            "Show() needs a proxy argument or that an active source is set."
        )
    if not view:
        # If there's no active view, controller.Show() will create a new preferred view.
        # if possible.
        view = GetActiveView()
    controller = servermanager.ParaViewPipelineController()
    rep = controller.Show(proxy, proxy.Port, view, representationType)
    if rep == None:
        raise RuntimeError(
            "Could not create a representation object for proxy %s"
            % proxy.GetXMLLabel()
        )
    for param in params.keys():
        setattr(rep, param, params[param])
    return rep


# -----------------------------------------------------------------------------
def ShowAll(view=None):
    """Show all pipeline sources in the given view.

    :param view: The view in which to show all pipeline sources.
    :type view: View proxy. Optional, defaults to the active view."""
    if not view:
        view = GetActiveView()
    controller = servermanager.ParaViewPipelineController()
    controller.ShowAll(view)


# -----------------------------------------------------------------------------
def Hide(proxy=None, view=None):
    """Turns the visibility of a given pipeline source off in the given view.
    If pipeline object and/or view are not specified, active objects are used.

    :param proxy: The pipeline source. Optional, defaults to the active source.
    :type proxy: Pipeline source proxy to hide.
    :param view: The view in which the pipeline source should be hidden. Optional,
        defaults to the active view.
    :type view: View proxy.
    """
    if not proxy:
        proxy = GetActiveSource()
    if not view:
        view = GetActiveView()
    if not proxy:
        raise ValueError(
            "proxy argument cannot be None when no active source is present."
        )
    controller = servermanager.ParaViewPipelineController()
    controller.Hide(proxy, proxy.Port, view)


# -----------------------------------------------------------------------------
def HideAll(view=None):
    """Hide all pipeline sources in the given view.

    :param view: The view in which to hide all pipeline sources. Optional, defaults
        to the active view.
    :type view: View proxy.
    """
    if not view:
        view = GetActiveView()
    controller = servermanager.ParaViewPipelineController()
    controller.HideAll(view)


# -----------------------------------------------------------------------------
def SetRepresentationProperties(proxy=None, view=None, **params):
    """Sets one or more representation properties of the given pipeline source.

    Pass a list of `property_name=value` pairs to this function to set property values.
    For example::

        SetProperties(Color=[1, 0, 0], LineWidth=2)

    :param proxy: Pipeline source whose representation properties should be set. Optional,
        defaults to the active source.
    :type proxy: Source proxy
    :param view: The view in which to make the representation property changes. Optional,
        defaults to the active view.
    :type view: View proxy
    """
    rep = GetRepresentation(proxy, view)
    proxy_util.set(rep, **params)


# -----------------------------------------------------------------------------
def SetDisplayProperties(proxy=None, view=None, **params):
    """DEPRECATED: Should use `SetRepresentationProperties()` instead"""
    return SetRepresentationProperties(proxy, view, **params)


# -----------------------------------------------------------------------------
def ColorBy(rep=None, value=None, separate=False):
    """Set data array to color a representation by. This will automatically set
    up the color maps and others necessary state for the representations.

    :param rep: Must be a representation proxy i.e. the value returned by
        the :func:`GetRepresentation`. Optional, defaults to the display properties
        for the active source, if possible.
    :type rep: Representation proxy
    :param value: Name of the array to color by.
    :type value: str
    :param separate: Set to `True` to create a color map unique to this
        representation. Optional, defaults to the global color map ParaView uses
        for any object colored by an array of the same name.
    :type separate: bool"""
    rep = rep if rep else GetDisplayProperties()
    if not rep:
        raise ValueError("No display properties can be determined.")

    rep.ColorBy(value, separate)


# -----------------------------------------------------------------------------
def ColorBlocksBy(rep=None, selectors=None, value=None, separate=False):
    """Like :func:`ColorBy`, set data array by which to color selected blocks within a
    representation, but color only selected blocks with the specified properties.
    This will automatically set up the color maps and others necessary state
    for the representations.

    :param rep: Must be a representation proxy i.e. the value returned by
        the :func:`GetRepresentation`. Optional, defaults to the display properties
        for the active source, if possible.
    :type rep: Representation proxy
    :param selectors: List of block selectors that choose which blocks to modify
        with this call.
    :type selectors: list of str
    :param value: Name of the array to color by.
    :type value: str
    :param separate: Set to `True` to create a color map unique to this
        representation. Optional, default is that the color map used will be the global
        color map ParaView uses for any object colored by an array of the same name.
    :type separate: bool"""
    rep = rep if rep else GetDisplayProperties()
    if not rep:
        raise ValueError("No display properties can be determined.")

    rep.ColorBlocksBy(selectors, value, separate)
