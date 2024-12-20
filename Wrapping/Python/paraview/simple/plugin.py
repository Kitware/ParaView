from paraview import servermanager


# ==============================================================================
# Plugin Management
# ==============================================================================


def LoadXML(xmlstring, ns=None):
    """Given a server manager XML as a string, parse and process it.
    If you loaded the simple module with ``from paraview.simple import *``,
    make sure to pass ``globals()`` as the second arguments::

        LoadXML(xmlstring, globals())

    Otherwise, the new functions will not appear in the global namespace.

    :param xmlstring: XML string containing server manager definitions.
    :type xmlstring: str
    :param ns: Namespace in which new functions will be defined. Optional,
               defaults to `None`.
    :type ns: dict

    """
    from paraview.simple import _extend_simple

    if not ns:
        ns = globals()
    servermanager.LoadXML(xmlstring)
    _extend_simple(ns)


# -----------------------------------------------------------------------------


def LoadPlugin(filename, remote=True, ns=None):
    """Loads a ParaView plugin and updates this module with new constructors
    if any. The remote argument (default to ``True``) is to specify whether
    the plugin will be loaded on the client (``remote=False``) or on the
    server (``remote=True``).

    If you loaded the simple module with ``from paraview.simple import *``,
    make sure to pass ``globals()`` as an argument::

        LoadPlugin("myplugin", False, globals()) # to load on client
        LoadPlugin("myplugin", True, globals())  # to load on server
        LoadPlugin("myplugin", ns=globals())     # to load on server

    Otherwise, the new functions will not appear in the global namespace.

    :param filename: Path to the plugin to load.
    :type filename: str
    :param remote: If `True`, loads the plugin on the server unless the
        connection is not remote. If `False`, loads the plugin on the client.
        Optional, defaults to `True`.
    :type remote: bool
    :param ns: Namespace in which new functions will be loaded. Optional,
        defaults to `None`.
    :type ns: dict
    :return: None
    """
    return LoadPlugins(filename, remote=remote, ns=ns)


# -----------------------------------------------------------------------------


def LoadPlugins(*args, **kwargs):
    """Loads ParaView plugins and updates this module with new constructors
    if any. The remote keyword argument (default to ``True``) is to specify
    whether the plugin will be loaded on client (``remote=False``) or on server
    (``remote=True``). Proxy definition updates are deferred until all plugins
    have been read, which can be more computationally efficient when multiple
    plugins are loaded in sequence.

    If you loaded the simple module with ``from paraview.simple import *``,
    make sure to pass ``globals()`` as a keyword argument::

        LoadPlugins("myplugin1", "myplugin2", remote=False, ns=globals()) # to load on client
        LoadPlugins("myplugin", "myplugin2", remote=True, ns=globals())  # to load on server
        LoadPlugins("myplugin", "myplugin2", ns=globals())     # to load on server

    Otherwise, the new functions will not appear in the global namespace.

    Note, `remote=True` has no effect when the connection is not remote.
    """
    from paraview.simple import _extend_simple

    remote = True
    if "remote" in kwargs:
        remote = kwargs["remote"]

    ns = globals()
    if "ns" in kwargs and kwargs["ns"]:
        ns = kwargs["ns"]

    servermanager.vtkSMProxyManager.GetProxyManager().SetBlockProxyDefinitionUpdates(
        True
    )
    for arg in args:
        servermanager.LoadPlugin(arg, remote)
    servermanager.vtkSMProxyManager.GetProxyManager().SetBlockProxyDefinitionUpdates(
        False
    )
    servermanager.vtkSMProxyManager.GetProxyManager().UpdateProxyDefinitions()

    _extend_simple(ns)


# -----------------------------------------------------------------------------


def LoadDistributedPlugin(pluginname, remote=True, ns=None):
    """Loads a plugin that's distributed with the ParaView executable. This uses the
    information known about plugins distributed with ParaView to locate the
    shared library for the plugin to load.

    :param pluginname: Short name of the plugin (not a file path)
    :type pluginname: str
    :param remote: If `True`, loads the plugin on the server unless the
        connection is not remote. If `False`, loads the plugin on the client.
        Optional, defaults to `True`.
    :type remote: bool
    :param ns: Namespace in which new functions will be loaded. Optional,
        defaults to `None`.
    :type ns: dict
    :raises RuntimeError: If the plugin was not found.
    """
    if not servermanager.ActiveConnection:
        raise RuntimeError("Cannot load a plugin without a session.")

    conn = servermanager.ActiveConnection
    do_remote = remote and conn.IsRemote()

    plm = servermanager.vtkSMProxyManager.GetProxyManager().GetPluginManager()
    if do_remote:
        session = servermanager.ActiveConnection.Session
        info = plm.GetRemoteInformation(session)
    else:
        info = plm.GetLocalInformation()
    for cc in range(0, info.GetNumberOfPlugins()):
        if info.GetPluginName(cc) == pluginname:
            return LoadPlugin(info.GetPluginFileName(cc), remote=do_remote, ns=ns)
    raise RuntimeError("Plugin '%s' not found" % pluginname)


# ==============================================================================
# Custom Filters Management
# ==============================================================================
def LoadCustomFilters(filename, ns=None):
    """Loads a custom filter XML file and updates this module with new
    constructors if any. If you loaded the simple module with
    ``from paraview.simple import *``, make sure to pass ``globals()`` as an
    argument.

    :param filename: Path to XML file with custom filter definitions.
    :type filename: str
    :param ns: Namespace in which new functions will be loaded. Optional,
        defaults to `None`.
    :type ns: dict
    """
    from paraview.simple import _extend_simple

    servermanager.ProxyManager().SMProxyManager.LoadCustomProxyDefinitions(filename)
    if not ns:
        ns = globals()

    _extend_simple(ns)
