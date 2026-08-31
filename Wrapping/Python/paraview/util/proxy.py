from paraview import servermanager
import builtins

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


class supported_proxies(object):
    """filter object used to hide proxies that are currently not supported by
    the state saving mechanism or those that are generally skipped in state.
    There is none currently but keeping the method for backward compatibility
    and future proofing."""

    def __call__(self, proxy):
        return proxy



def get_consumers(proxy, filter, consumer_set, recursive=True):
    """Returns the consumers for a proxy iteratively. If filter is non-None,
    filter is used to cull consumers."""
    for i in range(proxy.GetNumberOfConsumers()):
        consumer = proxy.GetConsumerProxy(i)
        consumer = consumer.GetTrueParentProxy() if consumer else None
        consumer = servermanager._getPyProxy(consumer)
        if not consumer or consumer.IsPrototype() or consumer in consumer_set:
            continue
        if filter(consumer):
            consumer_set.add(consumer)
            if recursive: get_consumers(consumer, filter, consumer_set)



def get_producers(proxy, filter, producer_set):
    """Returns the producers for a proxy iteratively. If filter is non-None,
    filter is used to cull producers."""
    for i in range(proxy.GetNumberOfProducers()):
        producer = proxy.GetProducerProxy(i)
        producer = producer.GetTrueParentProxy() if producer else None
        producer = servermanager._getPyProxy(producer)
        if not producer or producer.IsPrototype() or producer in producer_set:
            continue
        if filter(producer):
            producer_set.add(producer)
            get_producers(producer, filter, producer_set)
    # FIXME: LookupTable is missed :/, darn subproxies!
    try:
        if proxy.LookupTable and filter(proxy.LookupTable):
            producer_set.add(proxy.LookupTable)
            get_producers(proxy.LookupTable, filter, producer_set)
    except AttributeError:
        pass
    try:
        if proxy.ScalarOpacityFunction and filter(proxy.ScalarOpacityFunction):
            producer_set.add(proxy.ScalarOpacityFunction)
            get_producers(proxy.ScalarOpacityFunction, filter, producer_set)
    except AttributeError:
        pass



def toposort(input_set):
    """implementation of Tarjan topological sort to sort proxies using consumer
    dependencies as graph edges."""
    result = []
    # "builtins" prefix needed because of paraview.util.proxy.set()
    marked_set = builtins.set()
    while marked_set != input_set:
        unmarked_node = (input_set - marked_set).pop()
        __toposort_visit(result, unmarked_node, input_set, marked_set)
    result.reverse()
    return result



def __toposort_visit(result, proxy, input_set, marked_set, t_marked_set=None):
    if t_marked_set is None:
        # "builtins" prefix needed because of paraview.util.proxy.set()
        temporarily_marked_set = builtins.set()
    else:
        temporarily_marked_set = t_marked_set
    if proxy in temporarily_marked_set:
        raise RuntimeError("Cycle detected in pipeline! %r" % proxy)
    if not proxy in marked_set:
        temporarily_marked_set.add(proxy)
        # "builtins" prefix needed because of paraview.util.proxy.set()
        consumers = builtins.set()
        get_consumers(proxy, lambda x: x in input_set, consumer_set=consumers, recursive=False)
        for x in consumers:
            __toposort_visit(result, x, input_set, marked_set, temporarily_marked_set)
        marked_set.add(proxy)
        temporarily_marked_set.discard(proxy)
        result.append(proxy)
