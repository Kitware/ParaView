from paraview import servermanager


def set(proxy, **props):
    """
    Sets one or more properties of the given proxy.
    Pass in arguments of the form `property_name=value` to this function to
    set property values.

    For example::

        set(
            proxy, # CAN NOT be None
            Center=[1, 2, 3],
            Radius=3.5,
        )

    :param proxy: The pipeline source whose properties should be set.
    :type proxy: Source proxy
    :param params: A variadic list of `key=value` pairs giving values of
        specific named properties in the pipeline source. For a list of available
        properties, call `help(proxy)`.
    """
    if proxy is None:
        raise RuntimeError("Proxy can not be None")

    proxy = servermanager._getPyProxy(proxy)
    for k, v in props.items():
        proxy.__setattr__(k, v)


def rename(proxy, group=None, name=None):
    """
    Renames the given proxy. This is the name used by :func:`FindSource` and
    is displayed in the Pipeline Browser.

    :param proxy: The proxy to be renamed
    :type proxy: Proxy object
    :param group: The group in which the proxy lives. Can be retrieved with
        `proxy.GetXMLGroup()` but not always (layouts vs misc).
    :type group: str
    :param newName: The new name of the proxy.
    :type newName: str
    """
    pxm = servermanager.ProxyManager()

    if group is None:
        group = proxy.GetXMLGroup()

    old_name = pxm.GetProxyName(group, proxy)
    if old_name and name != old_name:
        pxm.RegisterProxy(group, name, proxy)
        pxm.UnRegisterProxy(group, old_name, proxy)


def unregister(proxy):
    """
    Unregister proxy from ParaView Pipeline Controller
    """
    controller = servermanager.ParaViewPipelineController()
    controller.UnRegisterProxy(proxy)
