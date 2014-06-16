import paraview.servermanager as sm
import paraview.simple as simple
from paraview.vtk import vtkTimeStamp

class trace_output:
  """Class used to collect the trace output. Everytime anything is pushed into
  this using the append API, we ensure that the trace is updated. Trace
  doesn't put commands to the trace-output as soon as modifications are noticed
  to try to consolidate the state changes."""
  def __init__(self, data=None):
    self.__data = []
    self.append(data) if data else None

  def append(self, data):
    if isinstance(data, list):
      self.__data += data
      print "\n".join(data),"\n"
    elif isinstance(data, str):
      self.__data.append(data)
      print data,"\n"

  def append_separator(self):
    try:
      self.__data.append("") if self.__data[-1] != "" else None
    except IndexError:
      pass

  def append_separated(self, data):
      self.append_separator()
      self.append(data)

  def clear(self):
    self.__data = []

  def raw_data(self):
    return self.__data[:]

  def __str__(self):
    return '\n'.join(self.__data)


class Trace(object):
    __REGISTERED_ACCESSORS = {}

    @classmethod
    def get_registered_name(cls, proxy, reggroup):
        return proxy.SMProxy.GetSessionProxyManager().GetProxyName(reggroup, proxy.SMProxy)

    @classmethod
    def get_varname(cls, name):
        """returns an unique variable name given a suggested variable name."""
        name = sm._make_name_valid(name)
        name = name[0].lower() + name[1:]
        original_name = name
        suffix = 1
        while cls.__REGISTERED_ACCESSORS.has_key(name):
            name = "%s_%d" % (original_name, suffix)
            suffix += 1
        return name

    @classmethod
    def register_accessor(cls, accessor):
        cls.__REGISTERED_ACCESSORS[accessor.Object] = accessor

    @classmethod
    def unregister_accessor(cls, accessor):
        del cls.__REGISTERED_ACCESSORS[accessor.Object]

    @classmethod
    def reset(cls):
        cls.__REGISTERED_ACCESSORS.clear()

    @classmethod
    def get_accessor(cls, obj):
        try:
            return cls.__REGISTERED_ACCESSORS[obj]
        except KeyError:
            # Create accessor if possible else raise
            # "untraceable" exception.
            if cls._create_accessor(obj):
                return cls.__REGISTERED_ACCESSORS[obj]
            #return "<unknown>"
            raise Untraceable(
                    "%s is not 'known' at this point. Hence, we cannot trace "\
                    "it. Skipping this action." % repr(obj))

    @classmethod
    def has_accessor(cls, obj):
        return cls.__REGISTERED_ACCESSORS.has_key(obj)

    @classmethod
    def _create_accessor(cls, obj):
        global global_trace_output
        if isinstance(obj, sm.SourceProxy):
            # handle pipeline source/filter proxy.
            pname = obj.SMProxy.GetSessionProxyManager().GetProxyName("sources", obj.SMProxy)
            if pname:
                if obj == simple.GetActiveSource():
                    accessor = ProxyAccessor(cls.get_varname(pname), obj)
                    global_trace_output.append_separated([\
                        "# get active source.",
                        "%s = GetActiveSource()" % accessor])
                else:
                    accessor = ProxyAccessor(cls.get_varname(pname), obj)
                    global_trace_output.append_separated([\
                        "# find source",
                        "%s = FindSource('%s')" % (accessor, pname)])
                return True
        if obj.SMProxy.IsA("vtkSMViewProxy"):
            # handle view proxy.
            pname = obj.SMProxy.GetSessionProxyManager().GetProxyName("views", obj.SMProxy)
            if pname:
                if obj == simple.GetActiveView():
                    accessor = ProxyAccessor(cls.get_varname(pname), obj)
                    global_trace_output.append_separated([\
                        "# get active view.",
                        "%s = GetActiveView()" % accessor])
                else:
                    accessor = ProxyAccessor(cls.get_varname(pname), obj)
                    global_trace_output.append_separated([\
                        "# find view",
                        "%s = FindView('%s')" % (accessor, pname)])
                return True
        if obj.SMProxy.IsA("vtkSMRepresentationProxy"):
            # handle representations.
            if hasattr(obj, "Input"):
                inputAccsr = cls.get_accessor(obj.Input)
                # FIXME: trace view.
                #viewAccsr = cls.get_accessor(
                pname = obj.SMProxy.GetSessionProxyManager().GetProxyName("representations", obj.SMProxy)
                if pname:
                    varname = "%sDisplay" % inputAccsr
                    accessor = ProxyAccessor(cls.get_varname(varname), obj)
                    global_trace_output.append_separated([\
                        "# get display properties",
                        "%s = GetDisplayProperties(%s)" % (accessor, str(inputAccsr))])
                    return True
        if cls.get_registered_name(obj, "lookup_tables"):
            pname = cls.get_registered_name(obj, "lookup_tables")
            if cls._create_accessor_for_tf(obj, pname):
                return True
        if cls.get_registered_name(obj, "piecewise_functions"):
            pname = cls.get_registered_name(obj, "piecewise_functions")
            if cls._create_accessor_for_tf(obj, pname):
                return True
        return False

    @classmethod
    def _create_accessor_for_tf(cls, proxy, regname):
        global global_trace_output
        import re
        m = re.match("^[0-9.]*(.+)\\.%s$" % proxy.GetXMLName(), regname)
        if m:
            arrayName = m.group(1)
            if proxy.GetXMLGroup() == "lookup_tables":
                varsuffix = "LUT"
                comment = "color transfer function/color map"
                method = "GetColorTransferFunction"
            else:
                varsuffix = "PWF"
                comment = "opacity transfer function/opacity map"
                method = "GetOpacityTransferFunction"
            varname = cls.get_varname("%s%s" % (arrayName, varsuffix))
            accessor = ProxyAccessor(varname, proxy)
            global_trace_output.append_separated([\
                "# get %s for '%s'" % (comment, arrayName),
                "%s = %s('%s')" % (accessor, method, arrayName)])
            # FIXME: we should optionally log the current state for the transfer
            # function.
            return True
        return False

class Untraceable(Exception):
    def __init__(self, logmessage="<unspecified>"):
        self.LogMessage = logmessage

    def __str__(self):
        return repr(self.LogMessage)

class Accessor(object):
    def __init__(self, varname, obj):
        self.Varname = varname
        self.Object = obj
        Trace.register_accessor(self)

    def finalize(self):
        Trace.unregister_accessor(self)

    def __str__(self):
        return self.Varname

class ProxyAccessor(Accessor):
    def __init__(self, varname, proxy):
        Accessor.__init__(self, varname, proxy)

        self.OrderedProperties = []

        # Create accessors for properties on this proxy.
        oiter = sm.vtkSMOrderedPropertyIterator()
        oiter.SetProxy(proxy.SMProxy)
        while not oiter.IsAtEnd():
            prop_name = oiter.GetKey()
            prop_label = oiter.GetProperty().GetXMLLabel()
            sanitized_label = sm._make_name_valid(prop_label)

            prop = proxy.GetProperty(prop_name)
            if not type(prop) == sm.Property:
                # Note: when PropertyAccessor for a property with ProxyListDomain is
                # created, it creates accessors for all proxies in the domain as well.
                prop_accessor = PropertyAccessor(sanitized_label, prop, self)
                self.OrderedProperties.append(prop_accessor)
            oiter.Next()
        del oiter

    def get_properties(self):
        return self.OrderedProperties[:]

    def get_ctor_properties(self):
        """Returns a list of property accessors that should be specified
           in the constructor."""
        return [x for x in self.OrderedProperties if self.is_ctor_property(x)]

    def is_ctor_property(self, prop):
        return prop.Object.IsA("vtkSMInputProperty") or \
                prop.Object.FindDomain("vtkSMFileListDomain") != None

    def trace_properties(self, props, in_ctor):
        joiner = "\n    " if in_ctor else "\n"
        return joiner.join([x.trace_property(in_ctor) for x in props])

class PropertyAccessor(Accessor):
    def __init__(self, propkey, prop, proxyAccessor):
        Accessor.__init__(self, "%s.%s" % (proxyAccessor, propkey), prop)
        self.PropertyKey = propkey
        self.ProxyAccessor = proxyAccessor
        pld_domain = prop.FindDomain("vtkSMProxyListDomain")
        self.HasProxyListDomain = isinstance(prop, sm.ProxyProperty) and pld_domain != None

        if self.HasProxyListDomain:
            # register accessors for proxies in the proxy list domain.
            # This is cheating. Since there's no accessor for a proxy in the domain
            # unless the proxy is "active" in the property. However, since ParaView
            # UI never modifies the other properties, we cheat
            for i in xrange(pld_domain.GetNumberOfProxies()):
                domain_proxy = pld_domain.GetProxy(i)
                ProxyAccessor(self.varname(), sm._getPyProxy(domain_proxy))

    def trace_property(self, in_ctor):
        """return trace-text for the property."""
        return "%s = %s" % (self.varname(in_ctor), self.value())

    def varname(self, not_fully_scoped=False):
        return self.PropertyKey if not_fully_scoped else self.Varname

    def value(self):
        # FIXME: need to ensure "object" is accessible.
        if isinstance(self.Object, sm.ProxyProperty):
            data = self.Object[:]
            if self.has_proxy_list_domain():
                data = ["'%s'" % x.GetXMLLabel() for x in self.Object[:]]
            else:
                data = [str(Trace.get_accessor(x)) for x in self.Object[:]]
            try:
                data = data if len(data) > 1 else data[0]
            except IndexError:
                data = None
            return str(data)
        else:
            return str(self.Object)

    def get_trace_value(self, val):
        """Returns the value to trace. For proxies, this refers to the variable
        used to access the proxy"""
        return "'%s'" % str(val) if not isinstance(val, sm.Proxy) else \
            str(Trace.get_accessor(val))

    def has_proxy_list_domain(self):
        """Returns True if this property has a ProxyListDomain, else False."""
        return self.HasProxyListDomain

    def get_property_name(self):
        return self.PropertyKey

    def get_property_value(self):
        return self.ProxyAccessor.Object.GetPropertyValue(self.PropertyKey)

class TraceItem(object):
    def __init__(self):
        pass
    def finalize(self):
        pass

    def should_trace_in_create(self, prop):
        setting = sm.vtkSMTrace.GetActiveTracer().GetPropertiesToTraceOnCreate()
        if setting == sm.vtkSMTrace.RECORD_USER_MODIFIED_PROPERTIES:
            # In ParaView, user never changes properties in Create. It's only
            # afterwords, so skip all properties.
            return False
        trace_props_with_default_values = True \
            if setting == sm.vtkSMTrace.RECORD_ALL_PROPERTIES else False
        return not prop.Object.GetIsInternal() and not prop.Object.GetInformationOnly() \
                and (trace_props_with_default_values or not prop.Object.IsValueDefault())

class RegisterPipelineProxy(TraceItem):
    """This traces the creation of a Pipeline Proxy such as
    sources/filters/readers etc."""

    def __init__(self, proxy):
        TraceItem.__init__(self)
        self.Proxy = sm._getPyProxy(proxy)

    def finalize(self):
        global global_trace_output

        pname = Trace.get_registered_name(self.Proxy, "sources")
        varname = Trace.get_varname(pname)
        accessor = ProxyAccessor(varname, self.Proxy)

        ctor = sm._make_name_valid(self.Proxy.GetXMLLabel())
        ctor_props = accessor.get_ctor_properties()
        other_props = [x for x in accessor.get_properties() if \
                x not in ctor_props and self.should_trace_in_create(x)]

        global_trace_output.append_separated([\
                "# Create a new pipeline object",
                "%s = %s(%s)" % (accessor, ctor, accessor.trace_properties(ctor_props, in_ctor=True)),
                accessor.trace_properties(other_props, in_ctor=False)])

        # Trace any Proxies that are values on properties with ProxyListDomains.
        pld_props = [x for x in accessor.get_properties() if x.has_proxy_list_domain()]
        for prop in pld_props:
            paccessor = Trace.get_accessor(prop.get_property_value())
            pproperties = [x for x in paccessor.get_properties() if  self.should_trace_in_create(x)]
            if pproperties:
                global_trace_output.append_separated([\
                    "# Init the %s selected for '%s'" % \
                    (prop.value(), prop.get_property_name()),
                    paccessor.trace_properties(pproperties, in_ctor=False)])
        TraceItem.finalize(self)
        global_trace_output.append_separator()

class UnRegisterPipelineProxy(TraceItem):
    """This traces the deletion of a Pipeline proxy"""
    def __init__(self, proxy):
        global global_trace_output

        TraceItem.__init__(self)
        proxy = sm._getPyProxy(proxy)

        global_trace_output.append_separated([\
                "# delete a pipeline object",
                "Delete(%s)" % (Trace.get_accessor(proxy))])

class PropertiesModified(TraceItem):
    """Traces properties modified on a specific proxy."""
    def __init__(self, proxy):
        TraceItem.__init__(self)

        proxy = sm._getPyProxy(proxy)
        self.ProxyAccessor = Trace.get_accessor(proxy)
        self.MTime = vtkTimeStamp()
        self.MTime.Modified()

    def finalize(self):
        global global_trace_output
        props = self.ProxyAccessor.get_properties()
        props_to_trace = [k for k in props if self.MTime.GetMTime() < k.Object.GetMTime()]
        if props_to_trace:
            global_trace_output.append_separated([
                "# Properties modified on %s" % str(self.ProxyAccessor),
                self.ProxyAccessor.trace_properties(props_to_trace, in_ctor=False)])

        # also handle properties on values for properties with ProxyListDomain.
        for prop in [k for k in props if k.has_proxy_list_domain()]:
            val = prop.get_property_value()
            if val:
                valaccessor = Trace.get_accessor(val)
                props = valaccessor.get_properties()
                props_to_trace = [k for k in props if self.MTime.GetMTime() < k.Object.GetMTime()]
                if props_to_trace:
                  global_trace_output.append_separated([
                      "# Properties modified on %s" % valaccessor,
                      valaccessor.trace_properties(props_to_trace, in_ctor=False)])
        TraceItem.finalize(self)

class Show(TraceItem):
    """Traces Show"""
    def __init__(self, producer, port, view, display):
        global global_trace_output

        TraceItem.__init__(self)

        producer = sm._getPyProxy(producer)
        view = sm._getPyProxy(view)
        display = sm._getPyProxy(display)

        self.ProducerAccessor = Trace.get_accessor(producer)
        self.ViewAccessor = Trace.get_accessor(view)
        self.OutputPort = port
        self.Display = display

    def finalize(self):
        display = self.Display
        if not Trace.has_accessor(display):
            pname = "%sDisplay" % self.ProducerAccessor
            self.Accessor = ProxyAccessor(Trace.get_varname(pname), display)
            trace_defaults = True
        else:
            self.Accessor = Trace.get_accessor(display)
            trace_defaults = False
        port = self.OutputPort

        output = []
        if port > 0:
            txt = "%s = Show(OutputPort(%s, %d), %s)" % \
                (str(self.Accessor), str(self.ProducerAccessor), port, str(self.ViewAccessor))
        else:
            txt = "%s = Show(%s, %s)" % \
                (str(self.Accessor), str(self.ProducerAccessor), str(self.ViewAccessor))

        output.append("# show data in view")
        output.append(txt)
        if trace_defaults:
            # Now trace default values.
            properties = [x for x in self.Accessor.get_properties() \
                if  self.should_trace_in_create(x)]

            if properties:
                output.append("# Trace defaults for the display properties")
                output.append(self.Accessor.trace_properties(properties, in_ctor=False))

        global_trace_output.append_separated(output)
        TraceItem.finalize(self)

    def should_trace_in_create(self, prop):
        # avoid tracing "Input" property for representations.
        return False if not TraceItem.should_trace_in_create(self, prop) else \
            not prop.PropertyKey in ["Input",\
                "SelectionCellFieldDataArrayName",\
                "SelectionPointFieldDataArrayName"]

class Hide(TraceItem):
    """Traces Hide"""
    def __init__(self, producer, port, view):
        global global_trace_output
        TraceItem.__init__(self)

        producer = sm._getPyProxy(producer)
        view = sm._getPyProxy(view)
        producerAccessor = Trace.get_accessor(producer)
        viewAccessor = Trace.get_accessor(view)

        global_trace_output.append_separated([\
          "# hide data in view",
          "Hide(%s, %s)" % (str(producerAccessor), str(viewAccessor)) if port == 0 else \
              "Hide(OutputPort(%s, %d), %s)" % (str(producerAccessor), port, str(viewAccessor))])

class SetScalarColoring(TraceItem):
    """Trace vtkSMPVRepresentationProxy.SetScalarColoring"""
    def __init__(self, display, arrayname, attribute_type):
        TraceItem.__init__(self)

        self.Display = sm._getPyProxy(display)
        self.ArrayName = arrayname
        self.AttributeType = attribute_type

    def finalize(self):
        global global_trace_output

        TraceItem.finalize(self)

        if self.ArrayName:
            global_trace_output.append_separated([\
                "# set scalar coloring",
                "ColorBy(%s, ['%s', '%s'])" % (\
                    str(Trace.get_accessor(self.Display)),
                    self.get_association_as_str(self.AttributeType),
                    self.ArrayName)])
        else:
            global_trace_output.append_separated([\
                "# turn off scalar coloring",
                "ColorBy(%s, None)" % str(Trace.get_accessor(self.Display))])

    def get_association_as_str(self, ass):
        for key, val in sm.ASSOCIATIONS.items():
          if val == ass:
              return key
        raise Untraceable("Cannot detemine association '%d'" % ass)

class CallMethod(TraceItem):
    def __init__(self, proxy, methodname, *args, **kwargs):
        global global_trace_output
        TraceItem.__init__(self)

        to_trace = []
        try:
            to_trace.append("# " + kwargs["comment"])
        except KeyError:
            pass
        accessor = Trace.get_accessor(sm._getPyProxy(proxy))
        args = [str(self.marshall(x)) for x in args]
        to_trace.append("%s.%s(%s)" % (accessor, methodname, ", ".join(args)))
        global_trace_output.append_separated(to_trace)

    def marshall(self, x):
        try:
            if x.IsA("vtkSMProxy"):
                return Trace.get_accessor(sm._getPyProxy(x))
            if type(x) == str:
                return "'%s'" % x
        except AttributeError:
            return x

def createTraceItem(key, args=None, kwargs=None):
    g = globals()
    if g.has_key(key) and callable(g[key]):
        args = args if args else []
        kwargs = kwargs if kwargs else {}
        return g[key](*args, **kwargs)
    raise Untraceable("Unknown trace item type %s" % key)
    #print "Hello again", key, args
    #return A(key)

def startTrace():
    """Starts tracing"""
    global global_trace_output
    Trace.reset()
    global_trace_output = trace_output()
    return True

def stopTrace():
    """Stops trace"""
    global global_trace_output

    trace = str(global_trace_output)
    Trace.reset()
    global_trace_output = None
    return trace

global_trace_output = None

if __name__ == "__main__":
    print "Running test"
    from paraview import simple
    sm.vtkSMTrace.StartTrace()

    s = simple.Sphere()
    c = simple.Clip()

    print "***** TRACE RESULT *****"
    print sm.vtkSMTrace.StopTrace()
