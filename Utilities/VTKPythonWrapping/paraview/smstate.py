import smtrace
import servermanager

_proxy_groups = ["sources", "representations", "views", "scalar_bars", "selection_sources",
                 "piecewise_functions", "implicit_functions", "lookup_tables"]

class proxy_lookup(object):
    """This class implements the interface of vtkSMPythonTraceObserver."""

    def __init__(self, proxy):
        # Store the proxy, name, and group
        if isinstance(proxy, servermanager.Proxy):
            proxy = proxy.SMProxy
        for proxy_group in _proxy_groups:
            proxy_name = servermanager.ProxyManager().IsProxyInGroup(proxy, proxy_group)
            if proxy_name:
                self.proxy_name = proxy_name
                self.proxy_group = proxy_group
                self.proxy = proxy
                return

    # The following methods implement the interface of vtkSMPythonTraceObserver
    # that is used by the smtrace module to trace proxy registration events
    def GetLastProxyRegistered(self):
        return self.proxy

    def GetLastProxyRegisteredName(self):
        return self.proxy_name

    def GetLastProxyRegisteredGroup(self):
        return self.proxy_group


def get_all_inputs_registered(proxy):
    """Given a servermanager.Proxy, check if the proxy's Inputs
    have been registered with smtrace.  Returns True if all inputs
    are registered or proxy does not have an Input property, else
    returns False."""
    itr = servermanager.PropertyIterator(proxy.SMProxy)
    for prop in itr:
        if prop.IsA("vtkSMInputProperty"):
            for i in xrange(prop.GetNumberOfProxies()):
                input_proxy = prop.GetProxy(i)
                info = smtrace.get_proxy_info(input_proxy, search_existing=False)
                if not info: return False
    return True


def register_proxy(proxy):
    """Register a proxy with the smtrace module"""
    lookup = proxy_lookup(proxy)
    smtrace.on_proxy_registered(lookup, None)


def register_proxies_by_dependency(proxy_list):
    """Given a list of proxies, step through the list and register the proxies
    one by one.  Check if the proxy has an Input proxy, if it does then only
    register the proxy after its inputs have been registered."""

    # Make a copy of the input list
    proxies_to_register = list(proxy_list)

    # Step through the list, each time only registering proxies if their
    # inputs are already registered.  We stop when the list is empty or
    # if we iterate through the whole list without making progress.
    progress = True
    while proxies_to_register and progress:
        progress = False
        for proxy in list(proxies_to_register):
            if get_all_inputs_registered(proxy):
                register_proxy(proxy)
                proxies_to_register.remove(proxy)
                progress = True

    # Print a warning if there were proxies that could not be registered
    if proxies_to_register:
        print "WARNING: Missing dependencies, could not register proxies:", proxies_to_register


def get_proxy_lists_ordered_by_group(WithRendering=True):
    """Returns a list of lists.  Each sub list contains all proxies that are
    currently registered under a given group name.  The order of the sub lists
    is important.  The idea is that no proxy in a group list should have
    properties that refer to proxies that appear in later group lists.  For
    example, sources are listed before representations, representations are
    listed before views.  If WithRendering is false, groups that are related
    to rendering are skipped."""

    # Get proxy lists by group.  Order is very important here.  The idea is that
    # we want to register groups of proxies such that when a proxy is registered
    # none of its properties refer to proxies that have not yet been registered.
    #
    # rules:
    #
    # scalar_bars refer to lookup_tables.
    # sources refer to selection_sources
    # representations refer to sources and piecewise_functions
    # views refer to representations and scalar_bars

    if WithRendering:
        proxy_groups = ["implicit_functions", "piecewise_functions", "lookup_tables",
                        "scalar_bars", "selection_sources", "sources",
                        "representations", "views"]
    else:
        proxy_groups = ["implicit_functions", "selection_sources",  "sources"]

    # Collect the proxies using a list comprehension
    get_func = servermanager.ProxyManager().GetProxiesInGroup
    proxy_lists = [get_func(proxy_group).values() for proxy_group in proxy_groups]
    return proxy_lists


def _trace_state():
    """This method using the smtrace module to trace each registered proxy and
    generate a python trace script.  The proxies must be traced in the correct
    order so that no traced proxy refers to a proxy that is yet to be traced."""

    # Start trace
    smtrace.start_trace(CaptureAllProperties=True, UseGuiName=True)

    # Get proxy lists ordered by group
    proxy_lists = get_proxy_lists_ordered_by_group()

    # Now register the proxies with the smtrace module
    for proxy_list in proxy_lists:
        register_proxies_by_dependency(proxy_list)

    # Calling append_trace causes the smtrace module to sort out all the
    # registered proxies and their properties and write them as executable
    # python.
    smtrace.append_trace()

    # Stop trace and print it to the console
    smtrace.stop_trace()
    smtrace.print_trace()


def run():
    """This is the main method to call to save the state.  It calls _trace_state()
    and makes sure that smtrace.stop_trace() is called even if exceptions are
    thrown during execution."""
    try:
        _trace_state()
    finally:
        smtrace.stop_trace()


