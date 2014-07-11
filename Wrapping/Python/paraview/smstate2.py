r"""Module for generating a Python state for ParaView.
This module uses paraview.smtracer to generate a trace for a selected set of
proxies my mimicking the creating of various pipeline components in sequence.
Typical usage of this module is as follows::

    from paraview import smstate2
    state = smstate2.get_state()
    print state

Note, this cannot be called when Python tracing is active.
"""

from paraview import servermanager as sm
from paraview import smtracer
from paraview import simple

class supported_proxies(object):
    """filter object used to hide proxies that are currently not supported by
    the state saving mechanism or those that are generally skipped in state e.g.
    animation proxies and time keeper."""
    def __call__(self, proxy):
        return proxy and \
            not proxy.GetXMLGroup() == "animation" and \
            not proxy.GetXMLName() == "TimeKeeper"

class visible_representations(object):
    """filter object to skip hidden representations from being saved in state file"""
    def __call__(self, proxy):
        if not supported_proxies()(proxy): return False
        try:
            return proxy.Visibility
        except AttributeError:
            pass
        return True

def __toposort(input_set):
    """implementation of Tarjan topological sort to sort proxies using consumer
    dependencies as graph edges."""
    result = []
    marked_set = set()
    while marked_set != input_set:
        unmarked_node = (input_set - marked_set).pop()
        __toposort_visit(result, unmarked_node, input_set, marked_set)
    result.reverse()
    return result

def __toposort_visit(result, proxy, input_set, marked_set, t_marked_set=None):
    if t_marked_set is None:
        temporarily_marked_set = set()
    else:
        temporarily_marked_set = t_marked_set
    if proxy in temporarily_marked_set:
        raise RuntimeError, "Cycle detected in pipeline! %r" % proxy
    if not proxy in marked_set:
        temporarily_marked_set.add(proxy)
        consumers = set()
        get_consumers(proxy, lambda x: x in input_set, consumer_set=consumers, recursive=False)
        for x in consumers:
            __toposort_visit(result, x, input_set, marked_set, temporarily_marked_set)
        marked_set.add(proxy)
        temporarily_marked_set.discard(proxy)
        result.append(proxy)

def get_consumers(proxy, filter, consumer_set, recursive=True):
    """Returns the consumers for a proxy iteratively. If filter is non-None,
    filter is used to cull consumers."""
    for i in xrange(proxy.GetNumberOfConsumers()):
        consumer = proxy.GetConsumerProxy(i)
        consumer = consumer.GetTrueParentProxy() if consumer else None
        consumer = sm._getPyProxy(consumer)
        if not consumer or consumer.IsPrototype() or consumer in consumer_set:
            continue
        if filter(consumer):
            consumer_set.add(consumer)
            if recursive: get_consumers(consumer, filter, consumer_set)

def get_producers(proxy, filter, producer_set):
    """Returns the producers for a proxy iteratively. If filter is non-None,
    filter is used to cull producers."""
    for i in xrange(proxy.GetNumberOfProducers()):
        producer = proxy.GetProducerProxy(i)
        producer = producer.GetTrueParentProxy() if producer else None
        producer = sm._getPyProxy(producer)
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
    except AttributeError: pass
    try:
        if proxy.ScalarOpacityFunction and filter(proxy.ScalarOpacityFunction):
            producer_set.add(proxy.ScalarOpacityFunction)
            get_producers(proxy.ScalarOpacityFunction, filter, producer_set)
    except AttributeError: pass

def get_state(source_set=[], filter=None):
    """Returns the state string"""
    if sm.vtkSMTrace.GetActiveTracer():
        raise RuntimeError, "Cannot generate Python state when tracing is active."

    if filter is None:
      filter = visible_representations()

    # build a set of proxies of interest
    start_set = source_set if source_set else simple.GetSources().values()
    start_set = [x for x in start_set if filter(x)]

    # now, locate dependencies for the start_set, pruning irrelevant branches
    consumers = set(start_set)
    for proxy in start_set:
        get_consumers(proxy, filter, consumers)

    producers = set()
    for proxy in consumers:
        get_producers(proxy, filter, producers)

    # proxies_of_interest is set of all proxies that we should trace.
    proxies_of_interest = producers.union(consumers)
    #print "proxies_of_interest", proxies_of_interest

    trace_config = smtracer.start_trace()
    smtracer.reset_trace_output()

    trace = smtracer.TraceOutput()

    #--------------------------------------------------------------------------
    # First, we trace the views and layouts, if any.
    # TODO: add support for layouts.
    views = [x for x in proxies_of_interest if smtracer.Trace.get_registered_name(x, "views")]
    if views:
        trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# setup views used in the visualization",
            "# ----------------------------------------------------------------"])
        for view in views:
            # FIXME: save view camera positions and size.
            traceitem = smtracer.RegisterViewProxy(view)
            traceitem.finalize()
            del traceitem
        trace.append_separated(smtracer.get_current_trace_output_and_reset(raw=True))

    #--------------------------------------------------------------------------
    # Next, trace data processing pipelines.
    sorted_proxies_of_interest = __toposort(proxies_of_interest)
    sorted_sources = [x for x in sorted_proxies_of_interest \
        if smtracer.Trace.get_registered_name(x, "sources")]
    if sorted_sources:
        trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# setup the data processing pipelines",
            "# ----------------------------------------------------------------"])
        for source in sorted_sources:
            traceitem = smtracer.RegisterPipelineProxy(source)
            traceitem.finalize()
            del traceitem
        trace.append_separated(smtracer.get_current_trace_output_and_reset(raw=True))
        trace_config.SetTracePropertiesOnExistingProxies(False)

    #--------------------------------------------------------------------------
    # Now, trace the transfer functions (color maps and opacity maps) used.
    ctfs = set([x for x in proxies_of_interest \
        if smtracer.Trace.get_registered_name(x, "lookup_tables")])
    if ctfs:
        # for transfer functions, we want the trace to save all properties.
        # FIXME: provide a cleaner way for doing this.
        trace_config.SetTracePropertiesOnExistingProxies(True)
        trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# setup color maps and opacity mapes used in the visualization",
            "# note: the Get..() functions create a new object, if needed",
            "# ----------------------------------------------------------------"])
        for ctf in ctfs:
            smtracer.Trace.get_accessor(ctf)
            if ctf.ScalarOpacityFunction in proxies_of_interest:
                smtracer.Trace.get_accessor(ctf.ScalarOpacityFunction)
        trace.append_separated(smtracer.get_current_trace_output_and_reset(raw=True))
        trace_config.SetTracePropertiesOnExistingProxies(False)

    #--------------------------------------------------------------------------
    # Can't decide if the representations should be saved with the pipeline
    # objects or afterwords, opting for afterwords for now since the topological
    # sort doesn't guarantee that the representations will follow their sources
    # anyways.
    sorted_representations = [x for x in sorted_proxies_of_interest \
        if smtracer.Trace.get_registered_name(x, "representations")]
    scalarbar_representations = [x for x in sorted_proxies_of_interest\
        if smtracer.Trace.get_registered_name(x, "scalar_bars")]
    # print "sorted_representations", sorted_representations
    # print "scalarbar_representations", scalarbar_representations
    if sorted_representations or scalarbar_representations:
        for view in views:
            view_representations = [x for x in view.Representations if x in sorted_representations]
            view_scalarbars = [x for x in view.Representations if x in scalarbar_representations]
            if view_representations or view_scalarbars:
                trace.append_separated([\
                    "# ----------------------------------------------------------------",
                    "# setup the visualization in view '%s'" % smtracer.Trace.get_accessor(view),
                    "# ----------------------------------------------------------------"])
            for rep in view_representations:
                try:
                    producer = rep.Input
                    port = rep.Input.Port
                    traceitem = smtracer.Show(producer, port, view, rep,
                        comment="show data from %s" % smtracer.Trace.get_accessor(producer))
                    traceitem.finalize()
                    del traceitem
                    trace.append_separated(smtracer.get_current_trace_output_and_reset(raw=True))

                    if rep.IsScalarBarVisible(view):
                        # FIXME: this will save this multiple times, right now,
                        # if two representations use the same LUT.
                        trace.append_separated([\
                            "# show color legend",
                            "%s.SetScalarBarVisibility(%s, True)" % (\
                                smtracer.Trace.get_accessor(rep),
                                smtracer.Trace.get_accessor(view))])
                except AttributeError: pass
            # save the scalar bar properties themselves.
            if view_scalarbars:
                trace.append_separated("# setup the color legend parameters for each legend in this view")
                trace_config.SetTracePropertiesOnExistingProxies(True)
                for rep in view_scalarbars:
                    smtracer.Trace.get_accessor(rep)
                trace_config.SetTracePropertiesOnExistingProxies(False)
            trace.append_separated(smtracer.get_current_trace_output_and_reset(raw=True))
    del trace_config
    smtracer.stop_trace()
    return str(trace)

if __name__ == "__main__":
    print  "Running test"
    simple.Mandelbrot()
    simple.Show()
    simple.Hide()
    simple.Shrink().ShrinkFactor  = 0.4
    simple.UpdatePipeline()
    simple.Clip().ClipType.Normal[1] = 1

    rep = simple.Show()
    view = simple.Render()
    rep.SetScalarBarVisibility(view, True)
    simple.Render()
#    rep.SetScalarBarVisibility(view, False)

    print "===================================================================="
    print get_state()
