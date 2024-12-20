from paraview import servermanager

from paraview.simple.session import active_objects, GetActiveSource
from paraview.simple.rendering import GetRepresentation


# ==============================================================================
# Lookup Table / Scalarbar methods
# ==============================================================================
# -----------------------------------------------------------------------------
def HideUnusedScalarBars(view=None):
    """Hides all unused scalar bars from the view. A scalar bar is used if some
    data is shown in that view that is coloring using the transfer function
    shown by the scalar bar.

    :param view: View in which unused scalar bars should be hidden. Optional,
        defaults to the active view.
    :type view: View proxy"""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError("'view' argument cannot be None with no active is present.")
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.UpdateScalarBars(view.SMProxy, tfmgr.HIDE_UNUSED_SCALAR_BARS)


def HideScalarBarIfNotNeeded(lut, view=None):
    """Hides the given scalar bar if it is not used by any of the displayed data.

    :param lut: The lookup table (lut) or scalar bar proxy
    :type lut: Scalar bar proxy
    :param view: The view in which the scalar bar should be hidden. Optional,
        defaults to hiding scalar bars in the active view."""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError("'view' argument cannot be None with no active present.")
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.HideScalarBarIfNotNeeded(lut.SMProxy, view.SMProxy)


def UpdateScalarBars(view=None):
    """Hides all unused scalar bars and shows used scalar bars. A scalar bar is used
    if some data is shown in that view that is coloring using the transfer function
    shown by the scalar bar.

    :param view: The view in which scalar bar visibility should be changed. Optional,
        defaults to using the active view.
    :type view: View proxy"""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError("'view' argument cannot be None with no active is present.")
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.UpdateScalarBars(
        view.SMProxy, tfmgr.HIDE_UNUSED_SCALAR_BARS | tfmgr.SHOW_USED_SCALAR_BARS
    )


def UpdateScalarBarsComponentTitle(ctf, representation=None):
    """Update the component portion of the title in all scalar bars using the provided
    lookup table. The representation is used to recover the array from which the
    component title was obtained.

    :param ctf: The lookup table that the scalar bar represents. Optional, defaults to
        the representation of the active source in the active view.
    :type ctf: Transfer function proxy.
    :param representation: If provided, it is the representation to use to recover
        the array that provides the component title portion. Optional, defaults to the
        active representation.
    :type representation: Representation proxy
    :return: `True` if operation succeeded, `False` otherwise.
    :rtype: bool"""
    if not representation:
        view = active_objects.view
        proxy = active_objects.source
        if not view:
            raise ValueError(
                "'representation' argument cannot be None with no active view."
            )
        if not proxy:
            raise ValueError(
                "'representation' argument cannot be None with no active source."
            )
        representation = GetRepresentation(view, proxy)
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.UpdateScalarBarsComponentTitle(ctf.SMProxy, representation.SMProxy)


def GetScalarBar(ctf, view=None):
    """Returns the scalar bar for the given color transfer function in the given view.

    :param ctf: The color transfer function proxy whose scalar bar representation should
        be returned.
    :type ctf: Color transfer function proxy.
    :param view: View from which the scalar bar proxy should be retrieved.
        Optional, defaults to the active view, if possible.
    :type view: View proxy.
    :return: The scalar bar proxy for the color transfer function if found. his will
        either return an existing scalar bar or create a new one.
    :rtype: Scalar bar proxy"""
    view = view if view else active_objects.view
    if not view:
        raise ValueError(
            "'view' argument cannot be None when no active view is present"
        )
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    sb = servermanager._getPyProxy(
        tfmgr.GetScalarBarRepresentation(ctf.SMProxy, view.SMProxy)
    )
    return sb


# ==============================================================================
# Miscellaneous functions.
# ==============================================================================
def ShowInteractiveWidgets(proxy=None):
    """If possible in the current environment, this function will request the
    application to show the interactive widget(s) for the given proxy.

    :param proxy: The proxy whose associated interactive widgets should be shown.
        Optional, if not provided the active source's widgets are shown.
    :type proxy: Source proxy."""
    proxy = proxy if proxy else GetActiveSource()
    if not proxy:
        raise ValueError("No 'proxy' was provided and no active source was found.")
    _InvokeWidgetUserEvent(proxy, "ShowWidget")


def HideInteractiveWidgets(proxy=None):
    """If possible in the current environment, this function will
    request the application to hide the interactive widget(s) for the given proxy.

    :param proxy: The proxy whose associated interactive widgets should be hidden.
        Optional, if not provided the active source's widgets are hidden.
    :type proxy: Source proxy."""
    proxy = proxy if proxy else GetActiveSource()
    if not proxy:
        raise ValueError("No 'proxy' was provided and no active source was found.")
    _InvokeWidgetUserEvent(proxy, "HideWidget")


def _InvokeWidgetUserEvent(proxy, event):
    """Internal method used by ShowInteractiveWidgets/HideInteractiveWidgets"""
    if proxy:
        proxy.InvokeEvent("UserEvent", event)
        # Since in 5.0 and earlier, ShowInteractiveWidgets/HideInteractiveWidgets
        # was called with the proxy being the filter proxy (eg. Clip) and not the
        # proxy that has the widget i.e. (Clip.ClipType), we explicitly handle it
        # by iterating of proxy list properties and then invoking the event on
        # their value proxies too.
        for smproperty in proxy:
            if smproperty.FindDomain("vtkSMProxyListDomain"):
                _InvokeWidgetUserEvent(smproperty.GetData(), event)
