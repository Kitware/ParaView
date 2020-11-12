r"""Module for generating a Python state for ParaView.
This module uses paraview.smtrace to generate a trace for a selected set of
proxies by mimicking the creation of various pipeline components in sequence.
Typical usage of this module is as follows::

    from paraview import smstate
    state = smstate.get_state()
    print (state)

Note, this cannot be called when Python tracing is active.
"""

from paraview import servermanager as sm
from paraview import smtrace
from paraview import simple
import sys

if sys.version_info >= (3,):
    xrange = range

RECORD_MODIFIED_PROPERTIES = sm.vtkSMTrace.RECORD_MODIFIED_PROPERTIES
RECORD_ALL_PROPERTIES = sm.vtkSMTrace.RECORD_ALL_PROPERTIES

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
        raise RuntimeError ("Cycle detected in pipeline! %r" % proxy)
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

def get_state(options=None, source_set=[], filter=None, raw=False,
        preamble=None, postamble=None):
    """Returns the state string"""
    if options:
        options = sm._getPyProxy(options)
        propertiesToTraceOnCreate = options.PropertiesToTraceOnCreate
        skipHiddenRepresentations = options.SkipHiddenDisplayProperties
        skipRenderingComponents = options.SkipRenderingComponents
    else:
        propertiesToTraceOnCreate = RECORD_MODIFIED_PROPERTIES
        skipHiddenRepresentations = True
        skipRenderingComponents = False

    # essential to ensure any obsolete accessors don't linger - can cause havoc
    # when saving state following a Python trace session
    # (paraview/paraview#18994)
    import gc
    gc.collect()

    if sm.vtkSMTrace.GetActiveTracer():
        raise RuntimeError ("Cannot generate Python state when tracing is active.")

    if filter is None:
        filter = visible_representations() if skipHiddenRepresentations else supported_proxies()

    # build a set of proxies of interest
    if source_set:
        start_set = source_set
    else:
        # if nothing is specified, we save all views and sources.
        start_set = [x for x in simple.GetSources().values()] + simple.GetViews()
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
    #print ("proxies_of_interest", proxies_of_interest)

    trace_config = smtrace.start_trace(preamble="")
    # this ensures that lookup tables/scalar bars etc. are fully traced.
    trace_config.SetFullyTraceSupplementalProxies(True)
    trace_config.SetSkipRenderingComponents(skipRenderingComponents)

    trace = smtrace.TraceOutput()
    if preamble is None:
        trace.append("# state file generated using %s" % simple.GetParaViewSourceVersion())
    elif preamble:
        trace.append(preamble)
    trace.append_separated(smtrace.get_current_trace_output_and_reset(raw=True))

    #--------------------------------------------------------------------------
    # We trace the views and layouts, if any.
    if skipRenderingComponents:
        views = []
    else:
        views = [x for x in proxies_of_interest if smtrace.Trace.get_registered_name(x, "views")]

    if views:
        # sort views by their names, so the state has some structure to it.
        views = sorted(views, key=lambda x:\
                smtrace.Trace.get_registered_name(x, "views"))
        trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# setup views used in the visualization",
            "# ----------------------------------------------------------------"])
        for view in views:
            # FIXME: save view camera positions and size.
            traceitem = smtrace.RegisterViewProxy(view)
            traceitem.finalize()
            del traceitem
        trace.append_separated(smtrace.get_current_trace_output_and_reset(raw=True))
        trace.append_separated(["SetActiveView(None)"])

    # from views,  build the list of layouts of interest.
    layouts = set()
    for aview in views:
        l = simple.GetLayout(aview)
        if l:
            layouts.add(simple.GetLayout(aview))

    # trace create of layouts
    if layouts:
        layouts = sorted(layouts, key=lambda x:\
                  smtrace.Trace.get_registered_name(x, "layouts"))
        trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# setup view layouts",
            "# ----------------------------------------------------------------"])
        for layout in layouts:
            traceitem = smtrace.RegisterLayoutProxy(layout)
            traceitem.finalize(filter=lambda x: x in views)
            del traceitem
        trace.append_separated(smtrace.get_current_trace_output_and_reset(raw=True))

    if views:
        # restore the active view after the layouts have been created.
        trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# restore active view",
            "SetActiveView(%s)" % smtrace.Trace.get_accessor(simple.GetActiveView()),
            "# ----------------------------------------------------------------"])

    #--------------------------------------------------------------------------
    # Next, trace data processing pipelines.
    sorted_proxies_of_interest = __toposort(proxies_of_interest)
    sorted_sources = [x for x in sorted_proxies_of_interest \
        if smtrace.Trace.get_registered_name(x, "sources")]
    if sorted_sources:
        trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# setup the data processing pipelines",
            "# ----------------------------------------------------------------"])
        for source in sorted_sources:
            traceitem = smtrace.RegisterPipelineProxy(source)
            traceitem.finalize()
            del traceitem
        trace.append_separated(smtrace.get_current_trace_output_and_reset(raw=True))

    #--------------------------------------------------------------------------
    # Can't decide if the representations should be saved with the pipeline
    # objects or afterwards, opting for afterwards for now since the topological
    # sort doesn't guarantee that the representations will follow their sources
    # anyways.
    sorted_representations = [x for x in sorted_proxies_of_interest \
        if smtrace.Trace.get_registered_name(x, "representations")]
    scalarbar_representations = [x for x in sorted_proxies_of_interest\
        if smtrace.Trace.get_registered_name(x, "scalar_bars")]
    # print ("sorted_representations", sorted_representations)
    # print ("scalarbar_representations", scalarbar_representations)
    if not skipRenderingComponents and (sorted_representations or scalarbar_representations):
        for view in views:
            view_representations = [x for x in view.Representations if x in sorted_representations]
            view_scalarbars = [x for x in view.Representations if x in scalarbar_representations]
            if view_representations or view_scalarbars:
                trace.append_separated([\
                    "# ----------------------------------------------------------------",
                    "# setup the visualization in view '%s'" % smtrace.Trace.get_accessor(view),
                    "# ----------------------------------------------------------------"])
            for rep in view_representations:
                try:
                    producer = rep.Input
                    port = rep.Input.Port
                    traceitem = smtrace.Show(producer, port, view, rep,
                        comment="show data from %s" % smtrace.Trace.get_accessor(producer))
                    traceitem.finalize()
                    del traceitem
                    trace.append_separated(smtrace.get_current_trace_output_and_reset(raw=True))

                    if rep.UseSeparateColorMap:
                        trace.append_separated([\
                            "# set separate color map",
                            "%s.UseSeparateColorMap = True" % (\
                                smtrace.Trace.get_accessor(rep))])

                except AttributeError: pass
            # save the scalar bar properties themselves.
            if view_scalarbars:
                trace.append_separated("# setup the color legend parameters for each legend in this view")
                for rep in view_scalarbars:
                    smtrace.Trace.get_accessor(rep)
                    trace.append_separated(smtrace.get_current_trace_output_and_reset(raw=True))
                    trace.append_separated([\
                      "# set color bar visibility", "%s.Visibility = %s" % (\
                    smtrace.Trace.get_accessor(rep), rep.Visibility)])


            for rep in view_representations:
                try:
                    producer = rep.Input
                    port = rep.Input.Port

                    if rep.IsScalarBarVisible(view):
                        # FIXME: this will save this multiple times, right now,
                        # if two representations use the same LUT.
                        trace.append_separated([\
                            "# show color legend",
                            "%s.SetScalarBarVisibility(%s, True)" % (\
                                smtrace.Trace.get_accessor(rep),
                                smtrace.Trace.get_accessor(view))])

                    if not rep.Visibility:
                      traceitem = smtrace.Hide(producer, port, view)
                      traceitem.finalize()
                      del traceitem
                      trace.append_separated(smtrace.get_current_trace_output_and_reset(raw=True))

                except AttributeError: pass

    #--------------------------------------------------------------------------
    # Now, trace the transfer functions (color maps and opacity maps) used.
    ctfs = set([x for x in proxies_of_interest \
        if smtrace.Trace.get_registered_name(x, "lookup_tables")])
    if not skipRenderingComponents and ctfs:
        trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# setup color maps and opacity mapes used in the visualization",
            "# note: the Get..() functions create a new object, if needed",
            "# ----------------------------------------------------------------"])
        for ctf in ctfs:
            smtrace.Trace.get_accessor(ctf)
            if ctf.ScalarOpacityFunction in proxies_of_interest:
                smtrace.Trace.get_accessor(ctf.ScalarOpacityFunction)
        trace.append_separated(smtrace.get_current_trace_output_and_reset(raw=True))

    # Trace extractors.
    exgens = set([x for x in proxies_of_interest \
            if smtrace.Trace.get_registered_name(x, "extractors")])
    if exgens:
        trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# setup extractors",
            "# ----------------------------------------------------------------"])
        for exgen in exgens:
            # FIXME: this currently doesn't handle multiple output ports
            # correctly.
            traceitem = smtrace.CreateExtractor(\
                    xmlname=exgen.Writer.GetXMLName(),
                    producer=exgen.Producer,
                    extractor=exgen,
                    registrationName=smtrace.Trace.get_registered_name(exgen, "extractors"))
            traceitem.finalize()
            del traceitem
        trace.append_separated(smtrace.get_current_trace_output_and_reset(raw=True))

    # restore the active source since the order in which the pipeline is created
    # in the state file can end up changing the active source to be different
    # than what it was when the state is being saved.
    trace.append_separated([\
            "# ----------------------------------------------------------------",
            "# restore active source",
            "SetActiveSource(%s)" % smtrace.Trace.get_accessor(simple.GetActiveSource()),
            "# ----------------------------------------------------------------"])

    if postamble is None:
        if options:
            # add coda about extracts generation.
            trace.append_separated(["",
                "if __name__ == '__main__':",
                "    # generate extracts",
                "    SaveExtracts(ExtractsOutputDirectory='%s')" % options.ExtractsOutputDirectory])
    elif postamble:
        trace.append_separated(postamble)

    del trace_config
    smtrace.stop_trace()
    #print (trace)
    return str(trace) if not raw else trace.raw_data()

if __name__ == "__main__":
    print ( "Running test")
    simple.Mandelbrot()
    simple.Show()
    simple.Hide()
    simple.Shrink().ShrinkFactor  = 0.4
    simple.UpdatePipeline()
    simple.Clip().ClipType.Normal[1] = 1

    rep = simple.Show()
    view = simple.Render()
    view.ViewSize=[500, 500]
    rep.SetScalarBarVisibility(view, True)
    simple.Render()
#    rep.SetScalarBarVisibility(view, False)

    print ("====================================================================")
    print (get_state())
