
import servermanager


class trace_globals(): pass
def reset_trace_globals():
  trace_globals.trace_started = False
  trace_globals.capture_all_properties = False
  trace_globals.verbose = False
  trace_globals.last_registered_proxies = []
  trace_globals.registered_proxies = []
  trace_globals.trace_output = []
  trace_globals.traced_proxy_groups = ["sources", "representations", "views", "lookup_tables", "scalar_bars"]
  trace_globals.ignored_view_properties = ["ViewSize", "GUISize", "Representations"]
  trace_globals.ignored_representation_properties = ["Input"]
  trace_globals.observer = servermanager.vtkSMPythonTraceObserver()
reset_trace_globals()



class proxy_trace_info():
  def __init__(self, proxy, proxyGroup, proxyName):
    self.Proxy = proxy
    self.Group = proxyGroup
    self.ProxyName = proxyName
    self.PyVariable = servermanager._make_name_valid(proxyName).replace(".", "_")
    self.Props = dict()
    self.CurrentProps = dict()
    self.ModifiedProps = dict()

class prop_trace_info():
  def __init__(self, proxyTraceInfo, prop):

    self.Prop = prop
    # Determine python variable name
    self.PyVariable = prop.GetXMLLabel()
    # For non-self properties, use the xml name instead of xml label:
    if prop.GetParent() != proxyTraceInfo.Proxy:
      self.PyVariable = proxyTraceInfo.Proxy.GetPropertyName(prop)
    self.PyVariable = servermanager._make_name_valid(self.PyVariable)


def trace_observer():
  return trace_globals.observer

def ignoredViewProperties():
  return trace_globals.ignored_view_properties

def ignoredRepresentationProperties():
  return trace_globals.ignored_representation_properties

def propIsIgnored(info, propName):
  if info.Group == "views" and propName in ignoredViewProperties(): return True
  if info.Group == "representations" \
    and propName in ignoredRepresentationProperties(): return True
  return False

def get_proxy_info(p):
  for info in trace_globals.last_registered_proxies:
    if info.Proxy == p: return info
  for info in trace_globals.registered_proxies:
    if info.Proxy == p: return info
  return None

def make_comma_separated_string(values):
  ret = str()
  for v in values:
    if len(ret): ret += ", "
    ret += str(v)
  if len(ret): ret = " %s " % ret
  return ret

def vector_smproperty_tostring(proxyInfo, propInfo):
  proxy = proxyInfo.Proxy
  prop = propInfo.Prop
  pythonProp = servermanager._wrap_property(proxy, prop)
  return str(pythonProp)

def input_smproperty_tostring(proxyInfo, propInfo):
  proxy = proxyInfo.Proxy
  prop = propInfo.Prop
  # Create a list of the python variables used for each input proxy
  nameList = []
  for i in xrange(prop.GetNumberOfProxies()):
    inputProxy = prop.GetProxy(i)
    info = get_proxy_info(inputProxy)
    if info != None and info.PyVariable != None: nameList.append(info.PyVariable)

  if len(nameList) == 0: return "[]"
  if len(nameList) == 1: return nameList[0]
  if len(nameList) > 1:
    nameListStr = make_comma_separated_string(nameList)
    return "[%s]" % nameListStr

def proxy_smproperty_tostring(proxyInfo, propInfo):
  proxy = proxyInfo.Proxy
  prop = propInfo.Prop
  pythonProp = servermanager._wrap_property(proxy, prop)
  if len(pythonProp.Available) and prop.GetNumberOfProxies() == 1:
    proxyPropertyValue = prop.GetProxy(0)
    listdomain = prop.GetDomain('proxy_list')
    if listdomain:
      for i in xrange(listdomain.GetNumberOfProxies()):
        if listdomain.GetProxy(i) == proxyPropertyValue:

          info = proxy_trace_info(proxyPropertyValue, "helpers", pythonProp.Available[i])
          info.PyVariable = "%s.%s" % (proxyInfo.PyVariable, propInfo.PyVariable)
          trace_globals.registered_proxies.append(info)

          return "\"%s\"" % pythonProp.Available[i]


  # Create a list of the python variables used for each proxy
  nameList = []
  for i in xrange(prop.GetNumberOfProxies()):
    inputProxy = prop.GetProxy(i)
    info = get_proxy_info(inputProxy)
    if info != None and info.PyVariable != None: nameList.append(info.PyVariable)

  if len(nameList) == 0: return "[]"
  if len(nameList) == 1: return nameList[0]
  if len(nameList) > 1:
    nameListStr = make_comma_separated_string(nameList)
    return "[%s]" % nameListStr

def trace_proxy_registered(proxy, proxyGroup, proxyName):
  if trace_globals.verbose:
    print "Proxy '%s' registered in group '%s'" % (proxyName, proxyGroup)
  if not proxyGroup in trace_globals.traced_proxy_groups:
    return
  info = proxy_trace_info(proxy, proxyGroup, proxyName)
  trace_globals.last_registered_proxies.append(info)
  if trace_globals.capture_all_properties:
    itr = servermanager.PropertyIterator(proxy)
    for prop in itr:
      if prop.GetInformationOnly() or prop.GetIsInternal(): continue
      trace_property_modified(info, prop)

def trace_property_modified(info, prop):
  if prop.GetInformationOnly() or prop.GetIsInternal(): return
  propInfo = prop_trace_info(info, prop)
  if trace_globals.verbose:
    print "Property '%s' modifed on proxy '%s'" % (propInfo.PyVariable, info.ProxyName)
  propValue = None
  if (prop.IsA("vtkSMVectorProperty")):
    propValue = vector_smproperty_tostring(info, propInfo)
  elif (prop.IsA("vtkSMInputProperty")):
    propValue = input_smproperty_tostring(info, propInfo)
  elif (prop.IsA("vtkSMProxyProperty")):
    propValue = proxy_smproperty_tostring(info, propInfo)
  if propValue != None:
    info.Props[prop] = propValue
    info.ModifiedProps[propInfo.PyVariable] = propValue
    info.CurrentProps[propInfo.PyVariable] = propValue


def sort_proxy_info_by_group(infoList):
  views = []
  sources = []
  representations = []
  other = []
  for i in infoList:
    if i.Group == "views": views.append(i)
    elif i.Group == "sources": sources.append(i)
    elif i.Group == "representations": representations.append(i)
    else: other.append(i)
  return views + sources + other + representations

def append_trace():

  # Get the list of last registered proxies in sorted order
  modified_proxies = sort_proxy_info_by_group(trace_globals.last_registered_proxies)

  # Now append the existing proxies to the list
  for p in trace_globals.registered_proxies: modified_proxies.append(p)

  for info in modified_proxies:
    traceOutput = ""

    # Generate list of tuples : (propName, propValue)
    propNameValues = []
    for propName, propValue in info.ModifiedProps.iteritems():
      if propIsIgnored(info, propName): continue
      propNameValues.append( (propName,propValue) )

    # Clear the prop list
    info.ModifiedProps.clear()

    if info in trace_globals.last_registered_proxies:

      # Determine the function call to construct the proxy
      setPropertiesInCtor = True
      ctorArgs = []
      extraCtorCommands = ""
      ctorMethod = servermanager._make_name_valid(info.Proxy.GetXMLLabel())
      if info.Group == "representations":

        ctorMethod = "GetRepresentation"
        setPropertiesInCtor = False

        # First lookup input proxy:
        inputProxy = None
        if "Input" in info.CurrentProps:
          inputProxy = info.CurrentProps["Input"]
        if not inputProxy:
          print "smtrace: error looking up input proxy for representation '%s'" % info.ProxyName

        # Next look up the view:
        viewForRep = None
        for p in trace_globals.registered_proxies + trace_globals.last_registered_proxies:
          if p.Group == "views" and "Representations" in p.CurrentProps:
            if p.CurrentProps["Representations"].find(info.PyVariable) >= 0:
              viewForRep = p.PyVariable
        if not viewForRep:
          print "smtrace: error looking up view for representation '%s'" % info.ProxyName

        if inputProxy and viewForRep:
          ctorArgs += [inputProxy, viewForRep]

      if info.Group == "scalar_bars":
        ctorMethod = "CreateScalarBar"

        # Lookup the view for the scalar bar widget
        viewForRep = None
        for p in trace_globals.registered_proxies + trace_globals.last_registered_proxies:
          if p.Group == "views" and "Representations" in p.CurrentProps:
            if p.CurrentProps["Representations"].find(info.PyVariable) >= 0:
              viewForRep = p.PyVariable
        if not viewForRep:
          print "smtrace: error looking up view for representation '%s'" % info.ProxyName

        # If a view was found, use extraCtorCommands to add the scalar bar to the view
        if viewForRep:
          extraCtorCommands = "%s.Representations.append(%s)\n" % (viewForRep, info.PyVariable)

      if info.Group == "views":
        ctorMethod = "CreateRenderView"
        setPropertiesInCtor = False
      if info.Group == "lookup_tables":
        ctorMethod = "CreateLookupTable"


      if setPropertiesInCtor:
        for propName, propValue in propNameValues:
          ctorArgs.append("%s=%s"%(propName, propValue))
        propNameValues = []

      ctorArgString = make_comma_separated_string(ctorArgs)
      traceOutput = "%s = %s(%s)\n%s" % (info.PyVariable, ctorMethod, ctorArgString, extraCtorCommands)

    # Set properties on the proxy
    for propName, propValue in propNameValues:
      traceOutput += "%s.%s = %s\n" % (info.PyVariable, propName, propValue)

    if (len(traceOutput)):
      trace_globals.trace_output.append(traceOutput)
  for p in trace_globals.last_registered_proxies:
    trace_globals.registered_proxies.append(p)
  while (len(trace_globals.last_registered_proxies)):
    trace_globals.last_registered_proxies.pop()


def save_trace(fileName):
  append_trace()
  outFile = open(fileName, 'w')
  for line in trace_globals.trace_output:
    outFile.write(line)
    outFile.write("\n")
  outFile.close()

def print_trace():
  append_trace()
  for line in trace_globals.trace_output:
    print line

def on_proxy_registered(o, e):
  '''Called when a proxy is registered with the proxy manager'''
  p = o.GetLastProxyRegistered()
  pGroup = o.GetLastProxyRegisteredGroup()
  pName = o.GetLastProxyRegisteredName()
  if p and pGroup and pName:
    trace_proxy_registered(p, pGroup, pName)

def on_property_modified(o, e):
  '''Called when a property of a registered proxy is modified'''
  propName = o.GetLastPropertyModifiedName()
  proxy = o.GetLastPropertyModifiedProxy()
  if propName and proxy:
    prop = proxy.GetProperty(propName)
    info = get_proxy_info(proxy)
    if info and prop:
      trace_property_modified(info, prop)

def on_update_information(o, e):
  '''Called after an update information event'''
  append_trace()
  if trace_globals.verbose:
    print "----------- Current trace -----------------------"
    print_trace()
    print "-------------------------------------------------"
  

def add_observers():
  '''Add callback observers to the instance of vtkSMPythonTraceObserver'''
  o = trace_observer()
  o.AddObserver("RegisterEvent", on_proxy_registered)
  o.AddObserver("PropertyModifiedEvent", on_property_modified)
  o.AddObserver("UpdateInformationEvent", on_update_information)


def clear_trace():
  if not trace_globals.trace_started: return
  reset_trace_globals()

# clear trace globals and initialize trace observer
def start_trace(CaptureAllProperties=False, Verbose=False):
  clear_trace()
  add_observers()
  trace_globals.capture_all_properties = CaptureAllProperties
  trace_globals.verbose = Verbose 
  trace_globals.trace_started = True

