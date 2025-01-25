import paraview
import paraview._backwardscompatibilityhelper

from paraview import servermanager
from paraview.util import proxy as proxy_util

# ==============================================================================
# Internal helpers
# ==============================================================================


def _create_func(key, module, skipRegistration=False):
    """Internal function."""

    from paraview.simple.session import active_objects

    def CreateObject(*input, **params):
        """
        This function creates a new proxy. For pipeline objects that accept inputs,
        all non-keyword arguments are assumed to be inputs. All keyword arguments are
        assumed to be property-value pairs and are passed to the new proxy.

        """

        registrationName = None
        for nameParam in ["registrationName", "guiName"]:
            if nameParam in params:
                registrationName = params[nameParam]
                del params[nameParam]

        # Create a controller instance.
        controller = servermanager.ParaViewPipelineController()

        from paraview.catalyst.detail import IsInsituInput, CreateProducer

        if IsInsituInput(registrationName):
            # This is a catalyst input, replace with a trivial producer
            px = CreateProducer(registrationName)
        else:
            # Instantiate the actual object from the given module.
            px = paraview._backwardscompatibilityhelper.GetProxy(
                module, key, no_update=True
            )

        # preinitialize the proxy.
        controller.PreInitializeProxy(px)

        # Make sure non-keyword arguments are valid
        for inp in input:
            if inp != None and not isinstance(inp, servermanager.Proxy):
                if px.GetProperty("Input") != None:
                    raise RuntimeError("Expecting a proxy as input.")
                else:
                    raise RuntimeError(
                        "This function does not accept non-keyword arguments."
                    )

        # Assign inputs
        inputName = servermanager.vtkSMCoreUtilities.GetInputPropertyName(px.SMProxy, 0)

        if px.GetProperty(inputName) != None:
            if len(input) > 0:
                px.SetPropertyWithName(inputName, input)
            else:
                # If no input is specified, try the active pipeline object
                if (
                    px.GetProperty(inputName).GetRepeatable()
                    and active_objects.get_selected_sources()
                ):
                    px.SetPropertyWithName(
                        inputName, active_objects.get_selected_sources()
                    )
                elif active_objects.source:
                    px.SetPropertyWithName(inputName, active_objects.source)
        else:
            if len(input) > 0:
                raise RuntimeError("This function does not expect an input.")

        # Pass all the named arguments as property,value pairs
        proxy_util.set(px, **params)

        # post initialize
        controller.PostInitializeProxy(px)

        if isinstance(px, servermanager.MultiplexerSourceProxy):
            px.UpdateDynamicProperties()

        if not skipRegistration:
            # Register the proxy with the proxy manager (assuming we are only using
            # these functions for pipeline proxies or animation proxies.
            if isinstance(px, servermanager.SourceProxy):
                controller.RegisterPipelineProxy(px, registrationName)
            elif px.GetXMLGroup() == "animation":
                controller.RegisterAnimationProxy(px)
        return px

    # add special tag to detect these functions in _remove_functions
    CreateObject.__paraview_create_object_tag = True
    CreateObject.__paraview_create_object_key = key
    CreateObject.__paraview_create_object_module = module
    return CreateObject


# -----------------------------------------------------------------------------


def _listProperties(create_func):
    """Internal function that, given a proxy creation function, e.g.,
    paraview.simple.Sphere, returns the list of named properties that can
    be set during construction."""
    key = create_func.__paraview_create_object_key
    module = create_func.__paraview_create_object_module
    prototype_func = _create_func(key, module, skipRegistration=True)

    # Find the prototype by XML label name
    xml_definition = module._findProxy(name=key)
    xml_group = xml_definition["group"]
    xml_name = xml_definition["key"]
    prototype = servermanager.ProxyManager().GetPrototypeProxy(xml_group, xml_name)

    # Iterate over properties and add them to the list
    property_iter = prototype.NewPropertyIterator()
    property_iter.UnRegister(None)
    property_names = []
    while not property_iter.IsAtEnd():
        label = property_iter.GetPropertyLabel()
        if label is None:
            label = property_iter.GetKey()
        property_names.append(paraview.make_name_valid(label))
        property_iter.Next()

    return property_names


# -----------------------------------------------------------------------------


def _get_proxymodules_to_import(connection):
    """
    used in _add_functions, _get_generated_proxies, and _remove_functions to get
    modules to import proxies from.
    """
    if connection and connection.ProxiesNS:
        modules = connection.ProxiesNS
        return [modules.filters, modules.sources, modules.writers, modules.animation]
    else:
        return []


# -----------------------------------------------------------------------------


def _get_generated_proxies():
    proxies = []
    for m in _get_proxymodules_to_import(servermanager.ActiveConnection):
        for key in dir(m):
            proxies.append(key)
    return proxies


# -----------------------------------------------------------------------------


def _find_writer(filename):
    "Internal function."
    extension = None
    parts = filename.split(".")
    if len(parts) > 1:
        extension = parts[-1]
    else:
        raise RuntimeError("Filename has no extension, please specify a write")

    if extension == "png":
        return "vtkPNGWriter"
    elif extension == "bmp":
        return "vtkBMPWriter"
    elif extension == "ppm":
        return "vtkPNMWriter"
    elif extension == "tif" or extension == "tiff":
        return "vtkTIFFWriter"
    elif extension == "jpg" or extension == "jpeg":
        return "vtkJPEGWriter"
    else:
        raise RuntimeError("Cannot infer filetype from extension:", extension)


# -----------------------------------------------------------------------------


class _funcs_internals:
    "Internal class."
    first_render = True


# -----------------------------------------------------------------------------


def ListProperties(proxyOrCreateFunction):
    """Given a proxy or a proxy creation function, e.g. paraview.simple.Sphere,
    returns the list of properties for the proxy that would be created.

    :param proxyOrCreateFunction: Proxy or proxy creation function whose property
        names are desired.
    :type proxyOrCreateFunction: Proxy or proxy creation function
    :return: List of property names
    :rtype: List of str"""

    try:
        return _listProperties(proxyOrCreateFunction)
    except:
        pass

    try:
        return proxyOrCreateFunction.ListProperties()
    except:
        pass

    return None


# -----------------------------------------------------------------------------


def _DisableFirstRenderCameraReset():
    """Normally a ResetCamera is called automatically when Render is called for
    the first time after importing the `paraview.simple` module. Calling this
    function disables this first render camera reset."""
    _funcs_internals.first_render = False


# -----------------------------------------------------------------------------


def _create_doc(new, old):
    "Internal function."
    res = new + "\n"
    ts = []
    strpd = old.split("\n")
    for s in strpd:
        ts.append(s.lstrip())
    res += " ".join(ts)
    res += "\n"
    return res


# -----------------------------------------------------------------------------


def _func_name_valid(name):
    "Internal function."
    for c in name:
        if c == "(" or c == ")":
            return False

    return True


# -----------------------------------------------------------------------------


def _get_function_arguments(function):
    """The a list of named arguments for a function.
    Used for autocomplete in PythonShell in ParaView GUI and in pvpython"""
    import inspect

    if not inspect.isfunction(function) and not inspect.ismethod(function):
        raise TypeError(f"input argument: {function} is not a function")
    result = []
    if hasattr(function, "__paraview_create_object_tag") and getattr(
        function, "__paraview_create_object_tag"
    ):
        result = ListProperties(function)
    else:
        for _, parameter in inspect.signature(function).parameters.items():
            # skip *args and **kwargs
            if (
                parameter.kind != inspect.Parameter.VAR_POSITIONAL
                and parameter.kind != inspect.Parameter.VAR_KEYWORD
            ):
                result.append(parameter.name)
    return result


def _add_functions(g):
    if not servermanager.ActiveConnection:
        return []

    added_entries = set()
    activeModule = servermanager.ActiveConnection.ProxiesNS

    # add deprecated proxies first, so we create the new function and documentation.
    modules = paraview._backwardscompatibilityhelper.get_deprecated_proxies(
        activeModule
    )
    for proxyModule in modules:
        for deprecatedProxyPair in modules[proxyModule]:
            oldProxyLabel = deprecatedProxyPair[0]
            newProxyLabel = deprecatedProxyPair[1]
            if not newProxyLabel in g and _func_name_valid(newProxyLabel):
                f = _create_func(deprecatedProxyPair, proxyModule)
                f.__doc__ = "{} is a deprecated proxy. It will be automatically replaced by {}".format(
                    oldProxyLabel, newProxyLabel
                )
                f.__qualname__ = oldProxyLabel
                f.__name__ = oldProxyLabel
                g[oldProxyLabel] = f
                added_entries.add(oldProxyLabel)

    for m in _get_proxymodules_to_import(servermanager.ActiveConnection):
        # Skip registering proxies in certain modules (currently only writers)
        skipRegistration = m is activeModule.writers
        for key in dir(m):
            if key not in g and _func_name_valid(key):
                # print "add %s function" % key
                f = _create_func(key, m, skipRegistration)
                f.__qualname__ = key
                f.__name__ = key
                f.__doc__ = _create_doc(m.getDocumentation(key), f.__doc__)
                g[key] = f
                added_entries.add(key)

    return list(added_entries)


def _remove_functions(g):
    to_remove = [
        item[0]
        for item in g.items()
        if hasattr(item[1], "__paraview_create_object_tag")
    ]
    for key in to_remove:
        del g[key]
        # paraview.print_info("remove %s", key)


def _switchToActiveConnectionCallback(caller, event):
    """Callback called when the active session/connection changes in the
    ServerManager. We update the Python state to reflect the change."""
    if servermanager:
        session = servermanager.vtkSMProxyManager.GetProxyManager().GetActiveSession()
        connection = servermanager.GetConnectionFromSession(session)
        SetActiveConnection(connection)


def _initializeSession(connection):
    """Internal method used to initialize a session. Users don't need to
    call this directly. Whenever a new session is created this method is called
    by API in this module."""
    if not connection:
        raise RuntimeError("'connection' cannot be empty.")
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeSession(connection.Session)


class _active_session_observer:
    def __init__(self):
        pxm = servermanager.vtkSMProxyManager.GetProxyManager()
        self.ObserverTag = pxm.AddObserver(
            pxm.ActiveSessionChanged, _switchToActiveConnectionCallback
        )

    def __del__(self):
        if servermanager:
            if servermanager.vtkSMProxyManager:
                servermanager.vtkSMProxyManager.GetProxyManager().RemoveObserver(
                    self.ObserverTag
                )


# -----------------------------------------------------------------------------


class _active_objects(object):
    """This class manages the active objects (source and view). The active
    objects are shared between Python and the user interface. This class
    is for internal use. Use the :ref:`SetActiveSource`,
    :ref:`GetActiveSource`, :ref:`SetActiveView`, and :ref:`GetActiveView`
    methods for setting and getting active objects."""

    def __get_selection_model(self, name, session=None):
        "Internal method."
        if session and session != servermanager.ActiveConnection.Session:
            raise RuntimeError(
                "Try to set an active object with invalid active connection."
            )
        pxm = servermanager.ProxyManager(session)
        model = pxm.GetSelectionModel(name)
        if not model:
            model = servermanager.vtkSMProxySelectionModel()
            pxm.RegisterSelectionModel(name, model)
        return model

    def set_view(self, view):
        "Sets the active view."
        active_view_model = self.__get_selection_model("ActiveView")
        if view:
            active_view_model = self.__get_selection_model(
                "ActiveView", view.GetSession()
            )
            active_view_model.SetCurrentProxy(
                view.SMProxy, active_view_model.CLEAR_AND_SELECT
            )
        else:
            active_view_model = self.__get_selection_model("ActiveView")
            active_view_model.SetCurrentProxy(None, active_view_model.CLEAR_AND_SELECT)

    def get_view(self):
        "Returns the active view."
        return servermanager._getPyProxy(
            self.__get_selection_model("ActiveView").GetCurrentProxy()
        )

    def set_source(self, source):
        "Sets the active source."
        active_sources_model = self.__get_selection_model("ActiveSources")
        if source:
            # 3 == CLEAR_AND_SELECT
            active_sources_model = self.__get_selection_model(
                "ActiveSources", source.GetSession()
            )
            active_sources_model.SetCurrentProxy(
                source.SMProxy, active_sources_model.CLEAR_AND_SELECT
            )
        else:
            active_sources_model = self.__get_selection_model("ActiveSources")
            active_sources_model.SetCurrentProxy(
                None, active_sources_model.CLEAR_AND_SELECT
            )

    def __convert_proxy(self, px):
        "Internal method."
        if not px:
            return None
        elif px.IsA("vtkSMOutputPort"):
            return servermanager.OutputPort(
                servermanager._getPyProxy(px.GetSourceProxy()), px.GetPortIndex()
            )
        else:
            return servermanager._getPyProxy(px)

    def get_source(self):
        "Returns the active source."
        return self.__convert_proxy(
            self.__get_selection_model("ActiveSources").GetCurrentProxy()
        )

    def get_selected_sources(self):
        "Returns the set of sources selected in the pipeline browser."
        model = self.__get_selection_model("ActiveSources")
        proxies = []
        for i in range(model.GetNumberOfSelectedProxies()):
            proxies.append(self.__convert_proxy(model.GetSelectedProxy(i)))
        return proxies

    view = property(get_view, set_view)
    source = property(get_source, set_source)


# ==============================================================================
# Active Source / View / Camera / AnimationScene
# ==============================================================================


def GetActiveView():
    """Returns the active view.

    :return: Active view.
    :rtype: View proxy."""
    return active_objects.view


# -----------------------------------------------------------------------------


def SetActiveView(view):
    """Sets the active view.

    :param view: The view to make active.
    :type view: View proxy."""
    active_objects.view = view


# -----------------------------------------------------------------------------


def GetActiveSource():
    """Returns the active source.

    :return: Active pipeline source.
    :rtype: Source proxy"""
    return active_objects.source


# -----------------------------------------------------------------------------


def SetActiveSource(source):
    """Sets the active source.

    :param source: The source
    :type source: Source proxy"""
    active_objects.source = source


# -----------------------------------------------------------------------------


def GetActiveCamera():
    """Returns the active camera for the active view.

    :return: The active camera
    :rtype: `vtkCamera`"""
    return GetActiveView().GetActiveCamera()


# ==============================================================================
# Client/Server Connection methods
# ==============================================================================


def Disconnect(ns=None, force=True):
    """Disconnect from the currently connected server and free the active
    session. Does not shut down the client application where the call is executed.

    :param ns: Namespace in which ParaView functions were created. Optional, defaults
        to the namespace returned by `globals()`
    :type ns: Dict or `None`
    :param force: Force disconnection in a simultaneous connection. Optional, defaults
        to forcing disconnection.
    :type force: bool
    """
    if not ns:
        ns = globals()

    supports_simutaneous_connections = (
        servermanager.vtkProcessModule.GetProcessModule().GetMultipleSessionsSupport()
    )
    if not force and supports_simutaneous_connections:
        # This is an internal Disconnect request that doesn't need to happen in
        # multi-server setup. Ignore it.
        return
    if servermanager.ActiveConnection:
        _remove_functions(ns)
        servermanager.Disconnect()
        import gc

        gc.collect()


# -----------------------------------------------------------------------------


def Connect(
    ds_host=None, ds_port=11111, rs_host=None, rs_port=11111, timeout=60, ns=None
):
    """Creates a connection to a server.

    :param ds_host: Data server hostname. Needed to set up a client/data server/render server connection.
    :type ds_host: string
    :param ds_port: Data server port to listen on.
    :type ds_port: int
    :param rs_host: Render server hostname. Needed to set up a client/data server/render server connection.
    :type rs_host: string
    :param rs_port: Render server port to listen on.
    :type rs_port: int
    :param timeout: Time in seconds at which the connection is abandoned if not made.
    :type timeout: int

    Example usage connecting to a host named "amber"::

        Connect("amber") # Connect to a single server at default port
        Connect("amber", 12345) # Connect to a single server at port 12345
        Connect("amber", 11111, "vis_cluster", 11111) # connect to data server, render server pair
        Connect("amber", timeout=30) # Connect to a single server at default port with a 30s timeout instead of default 60s
        Connect("amber", timeout=-1) # Connect to a single server at default port with no timeout instead of default 60s
        Connect("amber", timeout=0)  # Connect to a single server at default port without retrying instead of retrying for
        the default 60s

    """
    from paraview.simple import _extend_simple

    if ns is None:
        ns = globals()
    Disconnect(ns, False)
    connection = servermanager.Connect(ds_host, ds_port, rs_host, rs_port, timeout)
    if not (connection is None):
        _initializeSession(connection)
        _extend_simple(ns)
    return connection


# -----------------------------------------------------------------------------


def ReverseConnect(port=11111, ns=None):
    """Create a reverse connection to a server.  First, disconnects from any servers,
    then listens on port and waits for an incoming connection from the server.

    :param port: The port to listen on for incoming connections.
    :type port: int
    :return: Connection object
    :rtype:
    """
    from paraview.simple import _extend_simple

    if ns is None:
        ns = globals()
    Disconnect(ns, False)
    connection = servermanager.ReverseConnect(port)
    _initializeSession(connection)
    _extend_simple(ns)
    return connection


# -----------------------------------------------------------------------------


def ResetSession():
    """Reset the session to its initial state. All pipeline readers, sources,
    filters extractors, and representations are removed."""
    connection = servermanager.ResetSession()
    _initializeSession(connection)
    return connection


# ==============================================================================
# Multi-servers
# ==============================================================================


def SetActiveConnection(connection=None, ns=None):
    """Set the active connection. If the process was run without multi-server
    enabled and this method is called with a non-None argument while an
    ActiveConnection is present, it will raise a RuntimeError.

    :param connection: If provided, changes the current connection.
    :type connection: Connection object.
    :param ns: Namespace in which functions from the old Connection are removed
        and functions in the new Connection are added.
    :raises RuntimeError: If called when ParaView is not running in multi-server
        mode, a `RuntimeError` will be raised.
    """
    from paraview.simple import _extend_simple

    if not ns:
        ns = globals()
    if servermanager.ActiveConnection != connection:
        _remove_functions(ns)
        servermanager.SetActiveConnection(connection)
        _extend_simple(ns)


# -----------------------------------------------------------------------------


def IsFirstRender():
    return _funcs_internals.first_render


# -----------------------------------------------------------------------------


def ResetFirstRender():
    _funcs_internals.first_render = False


# -----------------------------------------------------------------------------
# Initialize module
# -----------------------------------------------------------------------------

if not paraview.options.satellite:
    active_session_observer = _active_session_observer()
    active_objects = _active_objects()
