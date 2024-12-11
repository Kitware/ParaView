import paraview
from paraview import servermanager
from paraview.util import proxy as proxy_util

from paraview.simple.session import GetActiveView


# ==============================================================================
# Views and Layout methods
# ==============================================================================
def CreateView(view_xml_name, **params):
    """Creates and returns the specified proxy view based on its name/label.
    Also set params keywords arguments as view properties.

    :param view_xml_name: Name of the requested View as it appears in the proxy
        definition XML. Examples include RenderView, ImageChartView,
        SpreadSheetView, etc.
    :param params: Dictionary created from named arguments. If one of these
        arguments is named 'registrationName', the string value associated
        with it will be used as the name of the proxy. That name will appear
        in the Pipeline Browser of the ParaView GUI, and it is the name to
        be passed to the FindView() function to find this view. Any additional
        named arguments will be passed as properties to initialize the view.
    :return: The created view Python object.
    :rtype:
    """
    view = servermanager._create_view(view_xml_name)
    if not view:
        raise RuntimeError("Failed to create requested view", view_xml_name)

    try:
        registrationName = params["registrationName"]
        del params["registrationName"]
    except KeyError:
        try:
            registrationName = params["guiName"]
            del params["guiName"]
        except KeyError:
            registrationName = None

    controller = servermanager.ParaViewPipelineController()
    controller.PreInitializeProxy(view)
    proxy_util.set(view, **params)
    controller.PostInitializeProxy(view)
    controller.RegisterViewProxy(view, registrationName)

    if paraview.compatibility.GetVersion() <= (5, 6):
        # older versions automatically assigned view to a
        # layout.
        controller.AssignViewToLayout(view)

    if paraview.compatibility.GetVersion() <= (5, 9):
        if hasattr(view, "UseColorPaletteForBackground"):
            view.UseColorPaletteForBackground = 0

    # setup an interactor if current process support interaction if an
    # interactor hasn't already been set. This overcomes the problem where VTK
    # segfaults if the interactor is created after the window was created.
    view.MakeRenderWindowInteractor(True)

    from paraview.catalyst.detail import IsInsitu, RegisterView

    if IsInsitu():
        # tag the view to know which pipeline this came from.
        RegisterView(view)
    return view


# -----------------------------------------------------------------------------


def CreateRenderView(**params):
    """Create standard 3D render view.
    See :func:`CreateView` for argument documentation"""
    return CreateView("RenderView", **params)


# -----------------------------------------------------------------------------


def CreateXYPlotView(**params):
    """Create XY plot Chart view.
    See :func:`CreateView` for argument documentation"""
    return CreateView("XYChartView", **params)


# -----------------------------------------------------------------------------


def CreateXYPointPlotView(**params):
    """Create XY plot point Chart view.
    See :func:`CreateView` for argument documentation"""
    return CreateView("XYPointChartView", **params)


# -----------------------------------------------------------------------------


def CreateBarChartView(**params):
    """Create Bar Chart view.
    See :func:`CreateView` for argument documentation"""
    return CreateView("XYBarChartView", **params)


# -----------------------------------------------------------------------------


def CreateComparativeRenderView(**params):
    """Create Comparative view.
    See :func:`CreateView` for argument documentation"""
    return CreateView("ComparativeRenderView", **params)


# -----------------------------------------------------------------------------


def CreateComparativeXYPlotView(**params):
    """Create comparative XY plot Chart view.
    See :func:`CreateView` for argument documentation"""
    return CreateView("ComparativeXYPlotView", **params)


# -----------------------------------------------------------------------------


def CreateComparativeBarChartView(**params):
    """Create comparative Bar Chart view.
    See :func:`CreateView` for argument documentation"""
    return CreateView("ComparativeBarChartView", **params)


# -----------------------------------------------------------------------------


def CreateParallelCoordinatesChartView(**params):
    """Create Parallel Coordinates Chart view.
    See :func:`CreateView` for argument documentation"""
    return CreateView("ParallelCoordinatesChartView", **params)


# -----------------------------------------------------------------------------


def Create2DRenderView(**params):
    """Create the standard 3D render view with the 2D interaction mode turned on.
    See :func:`CreateView` for argument documentation"""
    return CreateView("2DRenderView", **params)


# -----------------------------------------------------------------------------


def GetRenderView():
    """Returns the active view if there is one. Else creates and returns a new view.
    :return: the active view"""
    view = GetActiveView()
    if not view:
        # it's possible that there's no active view, but a render view exists.
        # If so, locate that and return it (before trying to create a new one).
        view = servermanager.GetRenderView()
    if not view:
        view = CreateRenderView()
    return view


# -----------------------------------------------------------------------------


def GetRenderViews():
    """Get all render views in a list.
    :return: all render views in a list."""
    return servermanager.GetRenderViews()


# -----------------------------------------------------------------------------


def GetViews(viewtype=None):
    """Returns all views in a list.

    :param viewtype: If provided, only the views of the
        specified type are returned. Optional, defaults to `None`.
    :type viewtype: str
    :return: list of views of the given viewtype, or all views if viewtype is `None`.
    :rtype: list of view proxies"""
    val = []
    for aProxy in servermanager.ProxyManager().GetProxiesInGroup("views").values():
        if aProxy.IsA("vtkSMViewProxy") and (
            viewtype is None or aProxy.GetXMLName() == viewtype
        ):
            val.append(aProxy)
    return val


# -----------------------------------------------------------------------------


def SetViewProperties(view=None, **params):
    """Sets one or more properties of the given view. If an argument
    is not provided, the active view is used. Pass in arguments of the form
    `property_name=value` to this function to set property values. For example::

        SetProperties(Background=[1, 0, 0], UseImmediateMode=0)

    :param view: The view whose properties are to be set. If not provided, the
        active view is used.
    :type view: View proxy.
    :param params: A variadic list of `key=value` pairs giving values of
        specific named properties in the view. For a list of available properties,
        call `help(view)`.
    """
    if not view:
        view = GetActiveView()
    proxy_util.set(view, **params)


def ExportView(filename, view=None, **params):
    """Export a view to the specified output file. Based on the view and file
    extension, an exporter of the right type will be chosen to export the view.

    :param filename: Name of the exported file.
    :type filename: str
    :param view: The view proxy to export. Optional, defaults to the active view.
    :type view: View proxy.
    :param params: A variadic list of `key=value` pairs giving values of
        specific named properties of the exporter."""
    view = view if view else GetActiveView()
    if not view:
        raise ValueError("No 'view' was provided and no active view was found.")
    if not filename:
        raise ValueError("No filename specified")

    # ensure that the view is up-to-date.
    view.StillRender()
    helper = servermanager.vtkSMViewExportHelper()
    proxy = helper.CreateExporter(filename, view.SMProxy)
    if not proxy:
        raise RuntimeError("Failed to create exporter for ", filename)
    proxy.UnRegister(None)
    proxy = servermanager._getPyProxy(proxy)
    proxy_util.set(proxy, **params)
    proxy.Write()
    del proxy
    del helper


def ImportView(filename, view=None, **params):
    """Import a view from a specified input scene file.

    :param filename: The name of the file to import.
    :type filename: str
    :param view: View proxy into which the scene should be imported. Optional, defaults
        to the active view.
    :type view: View proxy.
    :param params: A variadic list of `key=value` pairs giving values of
        specific named properties of the importer."""
    view = view if view else GetActiveView()
    if not view:
        raise ValueError("No 'view' was provided and no active view was found.")
    if not filename:
        raise ValueError("No filename specified")
    session = servermanager.ActiveConnection.Session
    proxy = servermanager.vtkSMImporterFactory.CreateImporter(filename, session)
    if not proxy:
        raise RuntimeError("Failed to create importer for ", filename)
    proxy.UnRegister(None)
    proxy = servermanager._getPyProxy(proxy)
    proxy_util.set(proxy, **params)
    proxy.UpdatePipelineInformation()
    proxy.Import(view)
    view.StillRender()
    del proxy
