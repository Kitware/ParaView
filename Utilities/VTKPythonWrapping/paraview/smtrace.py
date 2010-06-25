
import simple
import servermanager
import re


class trace_globals: pass

def reset_trace_observer():
  trace_globals.observer = servermanager.vtkSMPythonTraceObserver()
  trace_globals.observer_active = False

def reset_trace_globals():
  trace_globals.capture_all_properties = False
  trace_globals.use_gui_name = False
  trace_globals.verbose = False
  trace_globals.active_source_at_start = None
  trace_globals.active_view_at_start = None
  trace_globals.last_active_view = None
  trace_globals.last_active_source = None
  trace_globals.last_registered_proxies = []
  trace_globals.registered_proxies = []
  trace_globals.trace_output = ["try: paraview.simple\n"
                                "except: from paraview.simple import *\n"
                                "paraview.simple._DisableFirstRenderCameraReset()\n"]
  trace_globals.trace_output_endblock = "Render()"
  trace_globals.traced_proxy_groups = ["sources", "representations", "views",
                                       "implicit_functions", "piecewise_functions",
                                       "lookup_tables", "scalar_bars",
                                       "selection_sources", "animation"]
  trace_globals.ignored_properties = {
    "views" : ["ViewSize", "GUISize", "ViewPosition", "Representations"],
    "representations" : ["Input"],
    "animation" : ["Cues"]}
  trace_globals.proxy_ctor_hook = None
  reset_trace_observer()
reset_trace_globals()

def pyvariable_from_proxy_name(proxy_name):
  return servermanager._make_name_valid(proxy_name.replace(".", "_"))

class proxy_trace_info:
  def __init__(self, proxy, proxyGroup, proxyName):
    self.Proxy = proxy
    self.Group = proxyGroup
    self.ProxyName = proxyName
    self.PyVariable = pyvariable_from_proxy_name(proxyName)
    self.Props = dict()
    self.CurrentProps = dict()
    self.ModifiedProps = dict()
    self.ignore_next_unregister = False
    self.ctor_traced = False
    # If this proxy is a helper proxy that belongs to the ProxyDomain of
    # another proxy's ProxyProperty, this variable stores the proxy_trace_info
    # for that other proxy.  For example, the Slice.SliceType
    self.ParentProxyInfo = None

class prop_trace_info:
  def __init__(self, proxyTraceInfo, prop):

    self.Prop = prop
    # Determine python variable name
    self.PyVariable = prop.GetXMLLabel()
    # For non-self properties, use the xml name instead of xml label:
    if prop.GetParent() != proxyTraceInfo.Proxy:
      self.PyVariable = proxyTraceInfo.Proxy.GetPropertyName(prop)
    self.PyVariable = servermanager._make_name_valid(self.PyVariable)

class list_of_tuples (list):
  """This class is used when a list of tuples is needed where each tuple is of
     the form (key, object)"""

  def purge(self, key):
    """This method can be used to properties from the list of tuples.
    Returns the purged list."""
    clone = list_of_tuples()
    for pair in self:
      if pair[0] == key:
        pass
      else:
        clone.append(pair)
    return clone

  def get_value(self, key):
    for pair in self:
      if pair[0] == key:
        return pair[1]
    raise KeyError, "%s does not exist" % str(key)

  def has_key(self, key):
    for pair in self:
      if pair[0] == key:
        return True
    return False


def trace_observer():
  return trace_globals.observer

def propIsIgnored(info, propName):
  if info.Group in trace_globals.ignored_properties:
    return propName in trace_globals.ignored_properties[info.Group]
  return False

def track_existing_sources():
  existing_sources = simple.GetSources()
  for proxy_name, proxy_id in existing_sources:
    proxy = simple.FindSource(proxy_name)
    track_existing_source_proxy(proxy)

def track_existing_source_proxy(proxy, proxy_name):
    proxy_info = proxy_trace_info(proxy, "sources", proxy_name)
    proxy_info.ctor_traced = True
    trace_globals.registered_proxies.append(proxy_info)
    if not trace_globals.last_active_source and proxy == trace_globals.active_source_at_start:
      trace_globals.last_active_source = proxy
      trace_globals.trace_output.append("%s = GetActiveSource()" % proxy_info.PyVariable)
    else:
      trace_globals.trace_output.append("%s = FindSource(\"%s\")" % (proxy_info.PyVariable, proxy_name))
    return proxy_info

def track_existing_view_proxy(proxy, proxy_name):
    proxy_info = proxy_trace_info(proxy, "views", proxy_name)
    proxy_info.ctor_traced = True
    trace_globals.registered_proxies.append(proxy_info)
    if not trace_globals.last_active_view and proxy == trace_globals.active_view_at_start:
      trace_globals.last_active_view = proxy
      trace_globals.trace_output.append("%s = GetRenderView()" % proxy_info.PyVariable)
    else:
      all_views = simple.GetRenderViews()
      if proxy in all_views:
        view_index = all_views.index(proxy)
        trace_globals.trace_output.append("%s = GetRenderViews()[%d]" % (proxy_info.PyVariable, view_index))
    return proxy_info
  
def track_existing_representation_proxy(proxy, proxy_name):
  # Find the input proxy
  input_property = proxy.GetProperty("Input")
  if input_property.GetNumberOfProxies() > 0:
    input_proxy = input_property.GetProxy(0)
    input_proxy_info = get_proxy_info(input_proxy)
    if input_proxy_info:
      proxy_info = proxy_trace_info(proxy, "representations", proxy_name)
      proxy_info.ctor_traced = True
      trace_globals.registered_proxies.append(proxy_info)
      trace_globals.trace_output.append("%s = GetDisplayProperties(%s)" \
        % (proxy_info.PyVariable, input_proxy_info.PyVariable))
      return proxy_info
  return None

def track_existing_lookuptable_proxy(proxy, proxy_name):
    """When recording the state, we ran into a use of an existing lookuptable.
    Now this is tricky. When replaying the state, we need to ensure that it
    works in most general cases. Since LUT creation is totally hidden from the
    user for most part, we cannot expect the user to 'deal with it' when a lut
    that was expected to be already created is missing. So we handle more
    gracefully, creating the LUT is none already exists.
    All this is nicely encapsulate in GetLookupTableForArray() function. So
    really, the work here is no different than that when a LUT is registered by
    the GUI. So we simply pretend as if the LUT was registered."""
    return trace_proxy_registered(proxy, "lookup_tables", proxy_name)

def track_existing_animation_scene_proxy(proxy, proxy_name):
    proxy_info = proxy_trace_info(proxy, "animation", proxy_name)
    proxy_info.ctor_traced = True
    trace_globals.registered_proxies.append(proxy_info)
    trace_globals.trace_output.append("%s = GetAnimationScene()" \
        % (proxy_info.PyVariable))
    return proxy_info

def track_existing_animation_cue_proxy(proxy, proxy_name):
    return trace_proxy_registered(proxy, "animation", proxy_name)

def track_existing_proxy(proxy):
  proxy_name = get_source_proxy_registration_name(proxy)
  if proxy_name:
    return track_existing_source_proxy(proxy, proxy_name)
  proxy_name = get_representation_proxy_registration_name(proxy)
  if proxy_name:
    return track_existing_representation_proxy(proxy, proxy_name)
  proxy_name = get_view_proxy_registration_name(proxy)
  if proxy_name:
    return track_existing_view_proxy(proxy, proxy_name)
  proxy_name = get_lookuptable_proxy_registration_name(proxy)
  if proxy_name:
    return track_existing_lookuptable_proxy(proxy, proxy_name)
  proxy_name = get_animation_scene_proxy_registration_name(proxy)
  if proxy_name:
    return track_existing_animation_scene_proxy(proxy, proxy_name)
  proxy_name = get_animation_cue_proxy_registration_name(proxy)
  if proxy_name:
    return track_existing_animation_cue_proxy(proxy, proxy_name)
  return None

def get_proxy_info(p, search_existing=True):
  """Lookup the proxy_trace_info object for the given proxy or pyvariable.
  If no proxy_trace_info object is found, and search_existing is True
  (the default) then a new proxy_trace_info will be created if the proxy can
  be found in the list of existing servermanager proxies."""
  for info in trace_globals.last_registered_proxies + trace_globals.registered_proxies:
    if info.Proxy == p: return info
    if info.PyVariable == p: return info
  # It must be a proxy that existed before trace started
  if search_existing and not isinstance(p, str):
    return track_existing_proxy(p)
  return None

def ensure_active_source(proxy_info):
    if proxy_info and proxy_info.Proxy != trace_globals.last_active_source:
        trace_globals.trace_output.append("SetActiveSource(%s)" % proxy_info.PyVariable)
        trace_globals.last_active_source = proxy_info.Proxy

def ensure_active_view(proxy_info):
    if proxy_info and proxy_info.Proxy != trace_globals.last_active_view:
        trace_globals.trace_output.append("SetActiveView(%s)" % proxy_info.PyVariable)
        trace_globals.last_active_view = proxy_info.Proxy


def get_input_proxy_info_for_rep(rep_info):
    """Given a proxy_trace_info object for a representation proxy, returns the
    proxy_trace_info for the representation's input proxy.  If one is not found,
    returns None."""
    # The representation info must have 'Input' in its current properties dict:
    if "Input" in rep_info.CurrentProps:
        input_proxy_pyvariable = rep_info.CurrentProps["Input"]
        return get_proxy_info(input_proxy_pyvariable)
    return None

def get_view_proxy_info_for_rep(rep_info):
    """Given a proxy_trace_info object for a representation proxy, returns the
    proxy_trace_info for the view proxy that the representation belongs to.
    If one is not found, returns None."""
    for p in trace_globals.registered_proxies + trace_globals.last_registered_proxies:
        # If the proxy is a view, check for rep_info.Proxy in the
        # view's 'Representation' property
        if p.Group == "views":
          rep_prop = p.Proxy.GetProperty("Representations")
          if rep_prop:
            for i in xrange(rep_prop.GetNumberOfProxies()):
              if rep_info.Proxy == rep_prop.GetProxy(i): return p
    return None

def get_animated_proxy_info(cue_info):
    """Given a proxy_trace_info object for a animation cue proxy, returns the
    proxy_trace_info for the animated proxy.  If one is not found, returns None."""
    # The cue info must have 'AnimatedProxy' in its current properties dict:
    prop = cue_info.Proxy.GetProperty("AnimatedProxy")
    if prop and prop.GetNumberOfProxies():
        proxy = prop.GetProxy(0)
        if proxy.GetXMLName() == "RepresentationAnimationHelper":
            proxy = proxy.GetProperty("Source").GetProxy(0)
        return get_proxy_info(proxy)
    return None

def get_source_proxy_registration_name(proxy):
    """Assuming the given proxy is registered in the group 'sources',
    lookup the proxy's registration name with the servermanager"""
    return servermanager.ProxyManager().GetProxyName("sources", proxy)

def get_view_proxy_registration_name(proxy):
    """Assuming the given proxy is registered in the group 'views',
    lookup the proxy's registration name with the servermanager"""
    return servermanager.ProxyManager().GetProxyName("views", proxy)

def get_representation_proxy_registration_name(proxy):
    """Assuming the given proxy is registered in the group 'representations',
    lookup the proxy's registration name with the servermanager"""
    return servermanager.ProxyManager().GetProxyName("representations", proxy)

def get_lookuptable_proxy_registration_name(proxy):
    """Assuming the give proxy is registered in the group "lookup_tables",
    lookup the proxy's registration name with the servermanager"""
    return servermanager.ProxyManager().GetProxyName("lookup_tables", proxy)

def get_animation_scene_proxy_registration_name(proxy):
    """Assuming the give proxy is registered in the group "animation",
    lookup the proxy's registration name with the servermanager"""
    if proxy.GetXMLName() == "AnimationScene":
        return servermanager.ProxyManager().GetProxyName("animation", proxy)
    return None

def get_animation_cue_proxy_registration_name(proxy):
    """Assuming the give proxy is registered in the group "animation",
    lookup the proxy's registration name with the servermanager"""
    if proxy.GetXMLName() == "KeyFrameAnimationCue" or \
       proxy.GetXMLName() == "CameraAnimationCue" or \
       proxy.GetXMLName() == "TimeAnimationCue":
        return servermanager.ProxyManager().GetProxyName("animation", proxy)
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


def get_property_value_from_list_domain(proxyInfo, propInfo):
  """Given a proxy and one of its proxyproperties (or inputproperty),
  get the value of the property as a string IF the property has a
  proxy list domain, else return None.  For more information see the class
  method servermanager.ProxyProperty.GetAvailable.  As a side effect of
  calling this method, the current property value (which is another proxy)
  may be tracked by adding it to trace_globals.registered_proxies."""
  proxy = proxyInfo.Proxy
  prop = propInfo.Prop
  pythonProp = servermanager._wrap_property(proxy, prop)
  if len(pythonProp.Available) and prop.GetNumberOfProxies() == 1:
    proxyPropertyValue = prop.GetProxy(0)
    listdomain = prop.GetDomain('proxy_list')
    if listdomain:
      for i in xrange(listdomain.GetNumberOfProxies()):
        if listdomain.GetProxy(i) == proxyPropertyValue:
          info = get_proxy_info(proxyPropertyValue)
          if not info:
            info = proxy_trace_info(proxyPropertyValue, "helpers", pythonProp.Available[i])
          info.PyVariable = propInfo.PyVariable
          info.ParentProxyInfo = proxyInfo
          trace_globals.registered_proxies.append(info)

          # If capture_all_properties, record all the properties of this proxy
          if trace_globals.capture_all_properties:
            itr = servermanager.PropertyIterator(proxyPropertyValue)
            for prop in itr:
              if prop.GetInformationOnly() or prop.GetIsInternal(): continue
              trace_property_modified(info, prop)


          return "\"%s\"" % pythonProp.Available[i]
  return None


def proxy_smproperty_tostring(proxyInfo, propInfo):
  """Given a proxy_trace_info and a prop_trace_info for one of the
  proxy's proxy properties, return the property value as a string."""
  strValue = get_property_value_from_list_domain(proxyInfo, propInfo)
  if strValue: return strValue
  proxy = proxyInfo.Proxy
  prop = propInfo.Prop
  # Create a list of the python variables for each proxy in the property vector
  nameList = []
  for i in xrange(prop.GetNumberOfProxies()):
    inputProxy = prop.GetProxy(i)
    info = get_proxy_info(inputProxy)
    if info and info.PyVariable: nameList.append(info.PyVariable)
  if len(nameList) == 0: return "[]"
  if len(nameList) == 1: return nameList[0]
  if len(nameList) > 1:
    nameListStr = make_comma_separated_string(nameList)
    return "[%s]" % nameListStr

def input_smproperty_tostring(proxyInfo, propInfo):
  """Get the value of an servermanager.InputProperty as a string.
  Currently this is handled in the same way as the base class
  servermanager.ProxyProperty.  This method calls
  proxy_smproperty_tostring."""
  return proxy_smproperty_tostring(proxyInfo, propInfo)

def trace_proxy_rename(proxy_info, new_name):
  """ Handle renaming an existing source proxy."""
  if not proxy_info or proxy_info.Group != "sources": return
  old_pyvariable = proxy_info.PyVariable
  proxy_info.PyVariable = pyvariable_from_proxy_name(new_name)
  if proxy_info.PyVariable == old_pyvariable: return
  proxy_info.ignore_next_unregister = True
  name_to_set = new_name.replace("\"", "")
  trace_globals.trace_output.append("RenameSource(\"%s\", %s)" % (name_to_set, old_pyvariable))
  trace_globals.trace_output.append("%s = %s" % (proxy_info.PyVariable, old_pyvariable))
  trace_globals.trace_output.append("del %s" % old_pyvariable)

def trace_proxy_registered(proxy, proxyGroup, proxyName):
  """Creates a new proxy_trace_info object if the proxy type is one that is
  followed for trace (not all proxy types are).  Returns the new object or None."""
  if trace_globals.verbose:
    print "Proxy '%s' registered in group '%s'" % (proxyName, proxyGroup)
  if not proxyGroup in trace_globals.traced_proxy_groups:
    return None
  info = proxy_trace_info(proxy, proxyGroup, proxyName)
  trace_globals.last_registered_proxies.append(info)
  if trace_globals.capture_all_properties or proxyGroup == "selection_sources":
    itr = servermanager.PropertyIterator(proxy)
    for prop in itr:
      if prop.GetInformationOnly() or prop.GetIsInternal(): continue
      trace_property_modified(info, prop)
  return info

def trace_property_modified(info, prop):
  """Creates a new prop_trace_info object for the property modification
  and returns it."""
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
    if not info.ParentProxyInfo:
        info.Props[prop] = propInfo
        info.ModifiedProps[propInfo.PyVariable] = propValue
        info.CurrentProps[propInfo.PyVariable] = propValue
    else:
        parentProxyInfo = info.ParentProxyInfo
        parentProxyInfo.Props[prop] = propInfo
        propPyVariable = "%s.%s" % (info.PyVariable, propInfo.PyVariable)
        parentProxyInfo.ModifiedProps[propPyVariable] = propValue
        parentProxyInfo.CurrentProps[propPyVariable] = propValue
  return propInfo


def trace_save_screenshot(filename, size, allViews):
  """This method is called from the paraview c++ implementation.
  Do not change the arguments without updating the c++ code."""
  if not trace_globals.observer_active: return

  # make sure the trace is up to date
  if len(trace_globals.last_registered_proxies):
    append_trace()

  if not allViews:
    trace_globals.trace_output.append("WriteImage('%s')" % filename)
  else:
    import os
    filename, extension = os.path.splitext(filename)
    filename = filename + "_%02d" + extension
    viewsToSave = \
      [info.PyVariable for info in trace_globals.registered_proxies if info.Group == "views"]
    for i, view in enumerate(viewsToSave):
      saveStr = "WriteImage('%s', view=%s)" % (filename%i, view)
      trace_globals.trace_output.append(saveStr)
  trace_globals.trace_output.append("\n")

def property_references_untraced_proxy(propPyVariable, propValue, propInfoList):
  """Given a property pyvariable, the property value, and a list of prop_trace_info
  objects, this methods looks for the prop_trace_info in the list with the matching
  pyvariable.  If the property if not a proxy property, return False.  If it is proxy
  property then check each proxy referenced by the property to see if its ctor has
  been traced already.  If any proxy referenced has not been traced, return True, else
  return True."""
  for propInfo in propInfoList:
    if propInfo.PyVariable != propPyVariable: continue
    if propInfo.Prop.IsA("vtkSMProxyProperty"):

      # Skip properties with proxy_list domains
      if propInfo.Prop.GetDomain('proxy_list'): continue

      # Maybe instead of using the propValue string we should just ask the
      # proxy property for its current proxy values, but for now use the propValue string.
      # Convert string like '[proxyA, proxyB]]' into list ['proxyA', 'proxyB']
      proxyPyVariables = [name.strip() for name in propValue.replace('[','',).replace(']','').split(',')]

      # For each proxy pyvariable, check if its ctor has been traced.
      # When calling get_proxy_info we set search_existing to false because
      # we already have a pyvariable for the proxy, indicating we are not
      # trying to looking up an undiscovered proxy.
      for proxyPyVariable in proxyPyVariables:
        proxyInfo = get_proxy_info(proxyPyVariable, search_existing=False)
        if not proxyInfo or proxyInfo.ctor_traced: continue
        return True
  return False

def sort_proxy_info_by_group(infoList):
  views = []
  sources = []
  representations = []
  selections = []
  other = []
  for i in infoList:
    if i.Group == "views": views.append(i)
    elif i.Group == "sources": sources.append(i)
    elif i.Group == "selection_sources": selections.append(i)
    elif i.Group == "representations": representations.append(i)
    else: other.append(i)
  return views + selections + sources + other + representations

def append_trace():

  # Get the list of last registered proxies in sorted order
  modified_proxies = sort_proxy_info_by_group(trace_globals.last_registered_proxies)

  # Now append the existing proxies to the list
  for p in trace_globals.registered_proxies: modified_proxies.append(p)

  for info in modified_proxies:
    traceOutput = ""
    deferredProperties = []

    # Generate list of tuples : (propName, propValue)
    propNameValues = list_of_tuples()
    for propName, propValue in info.ModifiedProps.iteritems():
      if propIsIgnored(info, propName): continue

      # Note, the 'Input' property is ignored for representations, so we are
      # only dealing with filter proxies here.  If the 'Input' property is a
      # single value (not a multi-input filter), then ensure the input is
      # the active source and leave the 'Input' property out of the
      # propNameValues list.
      if propName == "Input" and propValue.find("[") == -1:
        inputProxyInfo = get_proxy_info(propValue)
        ensure_active_source(inputProxyInfo)
        continue

      if property_references_untraced_proxy(propName, propValue, info.Props.values()):
        deferredProperties.append((propName, propValue))
      else:
        propNameValues.append((propName, propValue))

    # Clear the modified prop list
    info.ModifiedProps.clear()

    # If info is in the last_registered_proxies list, then we need to add the
    # proxy's constructor call to the trace
    if info in trace_globals.last_registered_proxies and not info.ctor_traced:

      # Determine the function call to construct the proxy
      setPropertiesInCtor = True
      ctorArgs = []
      extraCtorCommands = ""
      ctorMethod = servermanager._make_name_valid(info.Proxy.GetXMLLabel())
      if info.Group == "sources":
        # track it as the last active source now
        trace_globals.last_active_source = info.Proxy
        # maybe append the guiName property
        if trace_globals.use_gui_name:
          ctorArgs.append("guiName=\"%s\"" % info.ProxyName)

      if info.Group == "representations":
        ctorMethod = "Show"
        setPropertiesInCtor = False
        # Ensure the input proxy is the active source:
        input_proxy_info = get_input_proxy_info_for_rep(info)
        if input_proxy_info:
          ensure_active_source(input_proxy_info)
        # Ensure the view is the active view:
        view_proxy_info = get_view_proxy_info_for_rep(info)
        if view_proxy_info:
          ensure_active_view(view_proxy_info)

      if info.Group == "scalar_bars":
        ctorMethod = "CreateScalarBar"
        extraCtorCommands = "GetRenderView().Representations.append(%s)\n" % info.PyVariable
        # Ensure the view is the active view:
        view_proxy_info = get_view_proxy_info_for_rep(info)
        if view_proxy_info:
          ensure_active_view(view_proxy_info)

      if info.Group == "views":
        if info.Proxy.GetXMLLabel() == "XYChartView":
          ctorMethod = "CreateXYPlotView"
        elif info.Proxy.GetXMLLabel() == "XYBarChartView":
          ctorMethod = "CreateBarChartView"
        elif info.Proxy.GetXMLName() == "ComparativeRenderView":
          ctorMethod = "CreateComparativeRenderView"
        elif info.Proxy.GetXMLName() == "ComparativeXYPlotView":
          ctorMethod = "CreateComparativeXYPlotView"
        elif info.Proxy.GetXMLName() == "ComparativeBarChartView":
          ctorMethod = "CreateComparativeBarChartView"
        else:
          ctorMethod = "CreateRenderView"

        # Now track it as the last active view
        trace_globals.last_active_view = info.Proxy
        setPropertiesInCtor = False
      if info.Group == "lookup_tables":
        match = re.match("(\d)\.([^.]+)\.PVLookupTable", info.ProxyName)
        if match:
          ctorMethod = "GetLookupTableForArray"
          ctorArgs.append("\"%s\"" % match.group(2))
          ctorArgs.append(match.group(1))
        else:
          # This is not a standard LUT created by the GUI--how in the world?
          # We'll just handle it as if it's a stray LUT.
          ctorMethod = "CreateLookupTable"

      if info.Group == "piecewise_functions":
        ctorMethod = "CreatePiecewiseFunction"

      if info.Group == "animation":
        if info.Proxy.GetXMLName() == "KeyFrameAnimationCue":
          ctorMethod = "GetAnimationTrack"
          propname = None
          index = None
          if propNameValues.has_key("AnimatedPropertyName"):
            propname = propNameValues.get_value("AnimatedPropertyName")
            propNameValues = propNameValues.purge("AnimatedPropertyName")
          if propNameValues.has_key("AnimatedElement"):
            propname = propNameValues.get_value("AnimatedElement")
            propNameValues = propNameValues.purge("AnimatedElement")
          propNameValues = propNameValues.purge("AnimatedProxy")
          source_proxy_info = get_animated_proxy_info(info)
          if source_proxy_info:
            ensure_active_source(source_proxy_info)
          ctorArgs.append(propname)
          if index != None:
            ctorArgs.append(index)
          setPropertiesInCtor = False
        elif info.Proxy.GetXMLName() == "CameraAnimationCue":
          view_proxy_info = get_animated_proxy_info(info)
          if view_proxy_info:
            ensure_active_view(view_proxy_info)
          ctorMethod = "GetCameraTrack"
          propNameValues = propNameValues.purge("AnimatedProxy")
          propNameValues = propNameValues.purge("AnimatedPropertyName")
          setPropertiesInCtor = False
        elif info.Proxy.GetXMLName() == "TimeAnimationCue":
          ctorMethod = "GetTimeTrack"
          propNameValues = propNameValues.purge("AnimatedProxy")
          propNameValues = propNameValues.purge("AnimatedPropertyName")
          setPropertiesInCtor = False

      if setPropertiesInCtor:
        for propName, propValue in propNameValues:
          if not "." in propName:
              ctorArgs.append("%s=%s" % (propName, propValue))
          else:
              # This line handles properties like:  my_slice.SliceType.Normal = [1, 0, 0]
              extraCtorCommands += "%s.%s = %s\n" % (info.PyVariable, propName, propValue)
        propNameValues = list_of_tuples()

      if trace_globals.proxy_ctor_hook:
        ctorMethod, ctorArgs, extraCtorCommands = trace_globals.proxy_ctor_hook(info,
          ctorMethod, ctorArgs, extraCtorCommands)

      ctorArgString = make_comma_separated_string(ctorArgs)
      traceOutput = "%s = %s(%s)\n%s" % (info.PyVariable, ctorMethod, ctorArgString, extraCtorCommands)
      info.ctor_traced = True

    # Set properties on the proxy
    for propName, propValue in propNameValues:
      traceOutput += "%s.%s = %s\n" % (info.PyVariable, propName, propValue)

    if traceOutput:
      trace_globals.trace_output.append(traceOutput)

    if deferredProperties:
      modified_proxies.append(info)
      for pair in deferredProperties:
        info.ModifiedProps[pair[0]] = pair[1]

  trace_globals.registered_proxies += trace_globals.last_registered_proxies
  del trace_globals.last_registered_proxies[:]


def get_trace_string():
  append_trace()
  s = str()
  for line in trace_globals.trace_output:
    s += line + "\n"
  s += trace_globals.trace_output_endblock + "\n"
  return s

def save_trace(fileName):
  append_trace()
  outFile = open(fileName, 'w')
  for line in trace_globals.trace_output:
    outFile.write(line)
    outFile.write("\n")
  outFile.write(trace_globals.trace_output_endblock + "\n")
  outFile.close()

def print_trace():
  append_trace()
  for line in trace_globals.trace_output:
    print line
  print trace_globals.trace_output_endblock

def on_proxy_registered(o, e):
  '''Called when a proxy is registered with the proxy manager'''
  p = o.GetLastProxyRegistered()
  pGroup = o.GetLastProxyRegisteredGroup()
  pName = o.GetLastProxyRegisteredName()
  if p and pGroup and pName:
    proxy_info = get_proxy_info(p, search_existing=False)
    # handle source proxy rename, no-op if proxy isn't a source
    if proxy_info:
      trace_proxy_rename(proxy_info, pName)
    else:
      trace_proxy_registered(p, pGroup, pName)

def on_proxy_unregistered(o, e):
  '''Called when a proxy is registered with the proxy manager'''
  p = o.GetLastProxyUnRegistered()
  pGroup = o.GetLastProxyUnRegisteredGroup()
  pName = o.GetLastProxyUnRegisteredName()
  if p and pGroup and pName:
    proxy_info = get_proxy_info(p)
    if proxy_info:
      if proxy_info.ignore_next_unregister:
        proxy_info.ignore_next_unregister = False
        return
      trace_globals.trace_output.append("Delete(%s)" % proxy_info.PyVariable)
      if proxy_info in trace_globals.last_registered_proxies:
        trace_globals.last_registered_proxies.remove(proxy_info)
      if proxy_info in trace_globals.registered_proxies:
        trace_globals.registered_proxies.remove(proxy_info)

def on_property_modified(o, e):
  '''Called when a property of a registered proxy is modified'''
  propName = o.GetLastPropertyModifiedName()
  proxy = o.GetLastPropertyModifiedProxy()
  if propName and proxy:
    prop = proxy.GetProperty(propName)
    if prop.GetInformationOnly() or prop.GetIsInternal(): return
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
  o.AddObserver("UnRegisterEvent", on_proxy_unregistered)
  o.AddObserver("PropertyModifiedEvent", on_property_modified)
  o.AddObserver("UpdateInformationEvent", on_update_information)
  trace_globals.observer_active = True

def clear_trace():
  reset_trace_globals()

def stop_trace():
  reset_trace_observer()

# clear trace globals and initialize trace observer
def start_trace(CaptureAllProperties=False, UseGuiName=False, Verbose=False):
  clear_trace()
  add_observers()
  trace_globals.active_source_at_start = simple.GetActiveSource()
  trace_globals.active_view_at_start = simple.GetActiveView()
  trace_globals.capture_all_properties = CaptureAllProperties
  trace_globals.use_gui_name = UseGuiName
  trace_globals.verbose = Verbose


