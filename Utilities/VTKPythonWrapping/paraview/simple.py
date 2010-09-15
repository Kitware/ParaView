r"""simple is a module for using paraview server manager in Python. It 
provides a simple convenience layer to functionality provided by the
C++ classes wrapped to Python as well as the servermanager module.

A simple example:
  from paraview.simple import *

  # Create a new sphere proxy on the active connection and register it
  # in the sources group.
  sphere = Sphere(ThetaResolution=16, PhiResolution=32)

  # Apply a shrink filter
  shrink = Shrink(sphere)
  
  # Turn the visiblity of the shrink object on.
  Show(shrink)
  
  # Render the scene
  Render()
"""
#==============================================================================
#
#  Program:   ParaView
#  Module:    simple.py
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#==============================================================================

import paraview
paraview.compatibility.major = 3
paraview.compatibility.minor = 5
import servermanager

def _disconnect():
    servermanager.ProxyManager().UnRegisterProxies()
    active_objects.view = None
    active_objects.source = None
    import gc
    gc.collect()
    servermanager.Disconnect()

def Connect(ds_host=None, ds_port=11111, rs_host=None, rs_port=11111):
    """Creates a connection to a server. Example usage:
    > Connect("amber") # Connect to a single server at default port
    > Connect("amber", 12345) # Connect to a single server at port 12345
    > Connect("amber", 11111, "vis_cluster", 11111) # connect to data server, render server pair"""
    _disconnect()
    cid = servermanager.Connect(ds_host, ds_port, rs_host, rs_port)
    tk =  servermanager.misc.TimeKeeper()
    servermanager.ProxyManager().RegisterProxy("timekeeper", "tk", tk)
    scene = AnimationScene()
    scene.TimeKeeper = tk
    return cid

def ReverseConnect(port=11111):
    """Create a reverse connection to a server.  Listens on port and waits for
    an incoming connection from the server."""
    _disconnect()
    cid = servermanager.ReverseConnect(port)
    tk =  servermanager.misc.TimeKeeper()
    servermanager.ProxyManager().RegisterProxy("timekeeper", "tk", tk)
    scene = AnimationScene()
    scene.TimeKeeper = tk
    return cid

def _create_view(view_xml_name):
    "Creates and returns a 3D render view."
    view = servermanager._create_view(view_xml_name)
    servermanager.ProxyManager().RegisterProxy("views", \
      "my_view%d" % _funcs_internals.view_counter, view)
    active_objects.view = view
    _funcs_internals.view_counter += 1
    
    tk = servermanager.ProxyManager().GetProxiesInGroup("timekeeper").values()[0]
    views = tk.Views
    if not view in views:
        views.append(view)
    try:
        scene = GetAnimationScene()
        if not view in scene.ViewModules:
            scene.ViewModules.append(view)
    except servermanager.MissingProxy:
        pass
    return view

def CreateRenderView():
    return _create_view("RenderView")

def CreateXYPlotView():
    return _create_view("XYChartView")

def CreateBarChartView():
    return _create_view("XYBarChartView")

def CreateComparativeRenderView():
    return _create_view("ComparativeRenderView")

def CreateComparativeXYPlotView():
    return _create_view("ComparativeXYPlotView")
 
def CreateComparativeBarChartView():
    return _create_view("ComparativeBarChartView")

def OpenDataFile(filename, **extraArgs):
    """Creates a reader to read the give file, if possible.
       This uses extension matching to determine the best reader possible.
       If a reader cannot be identified, then this returns None."""
    reader_factor = servermanager.ProxyManager().GetReaderFactory()
    if  reader_factor.GetNumberOfRegisteredPrototypes() == 0:
      reader_factor.RegisterPrototypes("sources")
    cid = servermanager.ActiveConnection.ID
    first_file = filename
    if type(filename) == list:
        first_file = filename[0]
    if not reader_factor.TestFileReadability(first_file, cid):
        raise RuntimeError, "File not readable: %s " % first_file
    if not reader_factor.CanReadFile(first_file, cid):
        raise RuntimeError, "File not readable. No reader found for '%s' " % first_file
    prototype = servermanager.ProxyManager().GetPrototypeProxy(
      reader_factor.GetReaderGroup(), reader_factor.GetReaderName())
    xml_name = paraview.make_name_valid(prototype.GetXMLLabel())
    if prototype.GetProperty("FileNames"):
      reader = globals()[xml_name](FileNames=filename, **extraArgs)
    else :
      reader = globals()[xml_name](FileName=filename, **extraArgs)
    return reader

def CreateWriter(filename, proxy=None, **extraArgs):
    """Creates a writer that can write the data produced by the source proxy in
       the given file format (identified by the extension). If no source is
       provided, then the active source is used. This doesn't actually write the
       data, it simply creates the writer and returns it."""
    if not filename:
       raise RuntimeError, "filename must be specified"
    writer_factory = servermanager.ProxyManager().GetWriterFactory()
    if writer_factory.GetNumberOfRegisteredPrototypes() == 0:
        writer_factory.RegisterPrototypes("writers")
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError, "Could not locate source to write"
    writer_proxy = writer_factory.CreateWriter(filename, proxy.SMProxy, proxy.Port)
    return servermanager._getPyProxy(writer_proxy)

def GetRenderView():
    "Returns the active view if there is one. Else creates and returns a new view."
    view = active_objects.view
    if not view: view = CreateRenderView()
    return view

def GetRenderViews():
    "Returns all render views as a list."
    return servermanager.GetRenderViews()

def GetRepresentation(proxy=None, view=None):
    """"Given a pipeline object and view, returns the corresponding representation object.
    If pipeline object and view are not specified, active objects are used."""
    if not view:
        view = active_objects.view
    if not proxy:
        proxy = active_objects.source
    rep = servermanager.GetRepresentation(proxy, view)
    if not rep:
        rep = servermanager.CreateRepresentation(proxy, view)
        servermanager.ProxyManager().RegisterProxy("representations", \
          "my_representation%d" % _funcs_internals.rep_counter, rep)
        _funcs_internals.rep_counter += 1
    return rep
    
def GetDisplayProperties(proxy=None, view=None):
    """"Given a pipeline object and view, returns the corresponding representation object.
    If pipeline object and/or view are not specified, active objects are used."""
    return GetRepresentation(proxy, view)
    
def Show(proxy=None, view=None, **params):
    """Turns the visibility of a given pipeline object on in the given view.
    If pipeline object and/or view are not specified, active objects are used."""
    if proxy == None:
        proxy = GetActiveSource()
    if proxy == None:
        raise RuntimeError, "Show() needs a proxy argument or that an active source is set."
    if not view and not active_objects.view:
        CreateRenderView()
    rep = GetDisplayProperties(proxy, view)
    if rep == None:
        raise RuntimeError, "Could not create a representation object for proxy %s" % proxy.GetXMLLabel()
    for param in params.keys():
        setattr(rep, param, params[param])
    rep.Visibility = 1
    return rep

def Hide(proxy=None, view=None):
    """Turns the visibility of a given pipeline object off in the given view.
    If pipeline object and/or view are not specified, active objects are used."""
    rep = GetDisplayProperties(proxy, view)
    rep.Visibility = 0

def Render(view=None):
    """Renders the given view (default value is active view)"""
    if not view:
        view = active_objects.view
    view.StillRender()
    if _funcs_internals.first_render:
        # Not all views have a ResetCamera method
        try:
            view.ResetCamera()
            view.StillRender()
        except AttributeError: pass
        _funcs_internals.first_render = False
    return view
        
def ResetCamera(view=None):
    """Resets the settings of the camera to preserver orientation but include
    the whole scene. If an argument is not provided, the active view is
    used."""
    if not view:
        view = active_objects.view
    view.ResetCamera()
    Render(view)

def _DisableFirstRenderCameraReset():
    """Disable the first render camera reset.  Normally a ResetCamera is called
    automatically when Render is called for the first time after importing
    this module."""
    _funcs_internals.first_render = False

def SetProperties(proxy=None, **params):
    """Sets one or more properties of the given pipeline object. If an argument
    is not provided, the active source is used. Pass a list of property_name=value
    pairs to this function to set property values. For example:
     SetProperties(Center=[1, 2, 3], Radius=3.5)
    """
    if not proxy:
        proxy = active_objects.source
    for param in params.keys():
        if not hasattr(proxy, param):
            raise AttributeError("object has no property %s" % param)
        setattr(proxy, param, params[param])

def GetProperty(*arguments, **keywords):
    """Get one property of the given pipeline object. If keywords are used,
       you can set the proxy and the name of the property that you want to get
       like in the following example :
            GetProperty({proxy=sphere, name="Radius"})
       If it's arguments that are used, then you have two case:
         - if only one argument is used that argument will be
           the property name.
         - if two arguments are used then the first one will be
           the proxy and the second one the property name.
       Several example are given below:
           GetProperty({name="Radius"})
           GetProperty({proxy=sphereProxy, name="Radius"})
           GetProperty( sphereProxy, "Radius" )
           GetProperty( "Radius" )
    """
    name = None
    proxy = None
    for key in keywords:
        if key == "name":
            name = keywords[key]
        if key == "proxy":
            proxy = keywords[key]
    if len(arguments) == 1 :
        name = arguments[0]
    if len(arguments) == 2 :
        proxy = arguments[0]
        name  = arguments[1]
    if not name:
        raise RuntimeError, "Expecting at least a property name as input. Otherwise keyword could be used to set 'proxy' and property 'name'"
    if not proxy:
        proxy = active_objects.source
    return proxy.GetProperty(name)

def SetDisplayProperties(proxy=None, view=None, **params):
    """Sets one or more display properties of the given pipeline object. If an argument
    is not provided, the active source is used. Pass a list of property_name=value
    pairs to this function to set property values. For example:
     SetProperties(Color=[1, 0, 0], LineWidth=2)
    """
    rep = GetDisplayProperties(proxy, view)
    SetProperties(rep, **params)

def SetViewProperties(view=None, **params):
    """Sets one or more properties of the given view. If an argument
    is not provided, the active view is used. Pass a list of property_name=value
    pairs to this function to set property values. For example:
     SetProperties(Background=[1, 0, 0], UseImmediateMode=0)
    """
    if not view:
        view = active_objects.view
    SetProperties(view, **params)

def RenameSource(newName, proxy=None):
    """Renames the given source.  If the given proxy is not registered
    in the sources group this method will have no effect.  If no source is
    provided, the active source is used."""
    if not proxy:
        proxy = active_objects.source
    pxm = servermanager.ProxyManager()
    oldName = pxm.GetProxyName("sources", proxy)
    if oldName:
      pxm.RegisterProxy("sources", newName, proxy)
      pxm.UnRegisterProxy("sources", oldName, proxy)

def FindSource(name):
    return servermanager.ProxyManager().GetProxy("sources", name)

def GetSources():
    """Given the name of a source, return its Python object."""
    return servermanager.ProxyManager().GetProxiesInGroup("sources")

def GetRepresentations():
    """Returns all representations (display properties)."""
    return servermanager.ProxyManager().GetProxiesInGroup("representations")

def UpdatePipeline(time=None, proxy=None):
    """Updates (executes) the given pipeline object for the given time as
    necessary (i.e. if it did not already execute). If no source is provided,
    the active source is used instead."""
    if not proxy:
        proxy = active_objects.source
    if time:
        proxy.UpdatePipeline(time)
    else:
        proxy.UpdatePipeline()

def Delete(proxy=None):
    """Deletes the given pipeline object or the active source if no argument
    is specified."""
    if not proxy:
        proxy = active_objects.source
    # Unregister any helper proxies stored by a vtkSMProxyListDomain
    for prop in proxy:
        listdomain = prop.GetDomain('proxy_list')
        if listdomain:
            if listdomain.GetClassName() != 'vtkSMProxyListDomain':
                continue
            group = "pq_helper_proxies." + proxy.GetSelfIDAsString()
            for i in xrange(listdomain.GetNumberOfProxies()):
                pm = servermanager.ProxyManager()
                iproxy = listdomain.GetProxy(i)
                name = pm.GetProxyName(group, iproxy)
                if iproxy and name:
                    pm.UnRegisterProxy(group, name, iproxy)
                    
    # Remove source/view from time keeper
    tk = servermanager.ProxyManager().GetProxiesInGroup("timekeeper").values()[0]
    if isinstance(proxy, servermanager.SourceProxy):
        try:
            idx = tk.TimeSources.index(proxy)
            del tk.TimeSources[idx]
        except ValueError:
            pass
    else:
        try:
            idx = tk.Views.index(proxy)
            del tk.Views[idx]
        except ValueError:
            pass
    servermanager.UnRegister(proxy)
    
    # If this is a representation, remove it from all views.
    if proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        for view in GetRenderViews():
            view.Representations.remove(proxy)
    # If this is a source, remove the representation iff it has no consumers
    # Also change the active source if necessary
    elif proxy.SMProxy.IsA("vtkSMSourceProxy"):
        sources = servermanager.ProxyManager().GetProxiesInGroup("sources")
        for i in range(proxy.GetNumberOfConsumers()):
            if proxy.GetConsumerProxy(i) in sources:
                raise RuntimeError("Source has consumers. It cannot be deleted " +
                  "until all consumers are deleted.")
        if proxy == GetActiveSource():
            if hasattr(proxy, "Input") and proxy.Input:
                if isinstance(proxy.Input, servermanager.Proxy):
                    SetActiveSource(proxy.Input)
                else:
                    SetActiveSource(proxy.Input[0])
            else: SetActiveSource(None)
        for rep in GetRepresentations().values():
            if rep.Input == proxy:
                Delete(rep)
    # Change the active view if necessary
    elif proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if proxy == GetActiveView():
            if len(GetRenderViews()) > 0:
                SetActiveView(GetRenderViews()[0])
            else:
                SetActiveView(None)

def CreateLookupTable(**params):
    """Create and return a lookup table.  Optionally, parameters can be given
    to assign to the lookup table.
    """
    lt = servermanager.rendering.PVLookupTable()
    servermanager.Register(lt)
    SetProperties(lt, **params)
    return lt

def CreatePiecewiseFunction(**params):
    """Create and return a piecewise function.  Optionally, parameters can be
    given to assign to the piecewise function.
    """
    pfunc = servermanager.piecewise_functions.PiecewiseFunction()
    servermanager.Register(pfunc)
    SetProperties(pfunc, **params)
    return pfunc

def GetLookupTableForArray(arrayname, num_components, **params):
    """Used to get an existing lookuptable for a array or to create one if none
    exists. Keyword arguments can be passed in to initialize the LUT if a new
    one is created."""
    proxyName = "%d.%s.PVLookupTable" % (int(num_components), arrayname)
    lut = servermanager.ProxyManager().GetProxy("lookup_tables", proxyName)
    if lut:
        return lut
    # No LUT exists for this array, create a new one.
    # TODO: Change this to go a LookupTableManager that is shared with the GUI,
    # so that the GUI and python end up create same type of LUTs. For now,
    # python will create a Blue-Red LUT, unless overridden by params.
    lut = servermanager.rendering.PVLookupTable(
            ColorSpace="HSV", RGBPoints=[0, 0, 0, 1, 1, 1, 0, 0])
    SetProperties(lut, **params)
    servermanager.Register(lut, registrationName=proxyName)
    return lut

def CreateScalarBar(**params):
    """Create and return a scalar bar widget.  The returned widget may
    be added to a render view by appending it to the view's representations
    The widget must have a valid lookup table before it is added to a view.
    It is possible to pass the lookup table (and other properties) as arguments
    to this method:
    
    lt = MakeBlueToRedLt(3.5, 7.5)
    bar = CreateScalarBar(LookupTable=lt, Title="Velocity")
    GetRenderView().Representations.append(bar)
    
    By default the returned widget is selectable and resizable.
    """
    sb = servermanager.rendering.ScalarBarWidgetRepresentation()
    servermanager.Register(sb)
    sb.Selectable = 1
    sb.Resizable = 1
    sb.Enabled = 1
    sb.Title = "Scalars"
    SetProperties(sb, **params)
    return sb

# TODO: Change this to take the array name and number of components. Register 
# the lt under the name ncomp.array_name
def MakeBlueToRedLT(min, max):
    # Define RGB points. These are tuples of 4 values. First one is
    # the scalar values, the other 3 the RGB values. 
    rgbPoints = [min, 0, 0, 1, max, 1, 0, 0]
    return CreateLookupTable(RGBPoints=rgbPoints, ColorSpace="HSV")
    
def _find_writer(filename):
    "Internal function."
    extension = None
    parts = filename.split('.')
    if len(parts) > 1:
        extension = parts[-1]
    else:
        raise RuntimeError, "Filename has no extension, please specify a write"
        
    if extension == 'png':
        return 'vtkPNGWriter'
    elif extension == 'bmp':
        return 'vtkBMPWriter'
    elif extension == 'ppm':
        return 'vtkPNMWriter'
    elif extension == 'tif' or extension == 'tiff':
        return 'vtkTIFFWriter'
    elif extension == 'jpg' or extension == 'jpeg':
        return 'vtkJPEGWriter'
    else:
        raise RuntimeError, "Cannot infer filetype from extension:", extension

def AddCameraLink(viewProxy, viewProxyOther, linkName):
    """Create a camera link between two view proxies.  A name must be given
    so that the link can be referred to by name.  If a link with the given
    name already exists it will be removed first."""
    if not viewProxyOther: viewProxyOther = GetActiveView()
    link = servermanager.vtkSMCameraLink()
    link.AddLinkedProxy(viewProxy.SMProxy, 1)
    link.AddLinkedProxy(viewProxyOther.SMProxy, 2)
    link.AddLinkedProxy(viewProxyOther.SMProxy, 1)
    link.AddLinkedProxy(viewProxy.SMProxy, 2)
    RemoveCameraLink(linkName)
    servermanager.ProxyManager().RegisterLink(linkName, link)

def RemoveCameraLink(linkName):
    """Remove a camera link with the given name."""
    servermanager.ProxyManager().UnRegisterLink(linkName)

def WriteImage(filename, view=None, **params):
    """Saves the given view (or the active one if none is given) as an
    image. Optionally, you can specify the writer and the magnification
    using the Writer and Magnification named arguments. For example:
     WriteImage("foo.mypng", aview, Writer=vtkPNGWriter, Magnification=2)
    If no writer is provided, the type is determined from the file extension.
    Currently supported extensions are png, bmp, ppm, tif, tiff, jpg and jpeg.
    The writer is a VTK class that is capable of writing images.
    Magnification is used to determine the size of the written image. The size
    is obtained by multiplying the size of the view with the magnification.
    Rendering may be done using tiling to obtain the correct size without
    resizing the view."""
    if not view:
        view = active_objects.view
    writer = None
    if params.has_key('Writer'):
        writer = params['Writer']
    mag = 1
    if params.has_key('Magnification'):
        mag = int(params['Magnification'])
    if not writer:
        writer = _find_writer(filename)
    view.WriteImage(filename, writer, mag)

def AnimateReader(reader=None, view=None, filename=None):
    """This is a utility function that, given a reader and a view
    animates over all time steps of the reader. If the optional
    filename is provided, a movie is created (type depends on the
    extension of the filename."""
    if not reader:
        reader = active_objects.source
    if not view:
        view = active_objects.view
        
    return servermanager.AnimateReader(reader, view, filename)


def _create_func(key, module):
    "Internal function."

    def CreateObject(*input, **params):
        """This function creates a new proxy. For pipeline objects that accept inputs,
        all non-keyword arguments are assumed to be inputs. All keyword arguments are
        assumed to be property,value pairs and are passed to the new proxy."""

        # Instantiate the actual object from the given module.
        px = module.__dict__[key]()

        # Make sure non-keyword arguments are valid
        for inp in input:
            if inp != None and not isinstance(inp, servermanager.Proxy):
                if px.GetProperty("Input") != None:
                    raise RuntimeError, "Expecting a proxy as input."
                else:
                    raise RuntimeError, "This function does not accept non-keyword arguments."

        # Assign inputs
        if px.GetProperty("Input") != None:
            if len(input) > 0:
                px.Input = input
            else:
                # If no input is specified, try the active pipeline object
                if px.GetProperty("Input").GetRepeatable() and active_objects.get_selected_sources():
                    px.Input = active_objects.get_selected_sources()
                elif active_objects.source:
                    px.Input = active_objects.source
        else:
            if len(input) > 0:
                raise RuntimeError, "This function does not expect an input."

        registrationName = None
        for nameParam in ['registrationName', 'guiName']:
          if nameParam in params:
              registrationName = params[nameParam]
              del params[nameParam]

        # Pass all the named arguments as property,value pairs
        for param in params.keys():
            setattr(px, param, params[param])

        try:
            # Register the proxy with the proxy manager.
            if registrationName:
                group, name = servermanager.Register(px, registrationName=registrationName)
            else:
                group, name = servermanager.Register(px)


            # Register pipeline objects with the time keeper. This is used to extract time values
            # from sources. NOTE: This should really be in the servermanager controller layer.
            if group == "sources":
                tk = servermanager.ProxyManager().GetProxiesInGroup("timekeeper").values()[0]
                sources = tk.TimeSources
                if not px in sources:
                    sources.append(px)

                active_objects.source = px
        except servermanager.MissingRegistrationInformation:
            pass

        return px

    return CreateObject

def _create_doc(new, old):
    "Internal function."
    import string
    res = ""
    for doc in (new, old):
        ts = []
        strpd = doc.split('\n')
        for s in strpd:
            ts.append(s.lstrip())
        res += string.join(ts)
        res += '\n'
    return res
    
def _func_name_valid(name):
    "Internal function."
    valid = True
    for c in name:
        if c == '(' or c ==')':
            valid = False
            break
    return valid

def _add_functions(g):
    for m in [servermanager.filters, servermanager.sources,
              servermanager.writers, servermanager.animation]:
        dt = m.__dict__
        for key in dt.keys():
            cl = dt[key]
            if not isinstance(cl, str):
                if not key in g and _func_name_valid(key):
                    g[key] = _create_func(key, m)
                    exec "g[key].__doc__ = _create_doc(m.%s.__doc__, g[key].__doc__)" % key

def GetActiveView():
    "Returns the active view."
    return active_objects.view
    
def SetActiveView(view):
    "Sets the active view."
    active_objects.view = view
    
def GetActiveSource():
    "Returns the active source."
    return active_objects.source
    
def SetActiveSource(source):
    "Sets the active source."
    active_objects.source = source
    
def GetActiveCamera():
    """Returns the active camera for the active view. The returned object
    is an instance of vtkCamera."""
    return GetActiveView().GetActiveCamera()

def GetAnimationScene():
    """Returns the application-wide animation scene. ParaView has only one
    global animation scene. This method provides access to that. Users are
    free to create additional animation scenes directly, but those scenes
    won't be shown in the ParaView GUI."""
    animation_proxies = servermanager.ProxyManager().GetProxiesInGroup("animation")
    scene = None
    for aProxy in animation_proxies.values():
        if aProxy.GetXMLName() == "AnimationScene":
            scene = aProxy
            break
    if not scene:
        raise servermanager.MissingProxy, "Could not locate global AnimationScene."
    return scene

def WriteAnimation(filename):
    """Helper to automate saving an animation."""
    scene = GetAnimationScene()
    iw = servermanager.vtkSMAnimationSceneImageWriter()
    iw.SetAnimationScene(scene.SMProxy)
    iw.SetFileName(filename)
    iw.Save()

def _GetRepresentationAnimationHelper(sourceproxy):
    """Internal method that returns the representation animation helper for a
       source proxy. It creates a new one if none exists."""
    # ascertain that proxy is a source proxy
    if not sourceproxy in GetSources().values():
        return None
    for proxy in servermanager.ProxyManager():
        if proxy.GetXMLName() == "RepresentationAnimationHelper" and\
           proxy.GetProperty("Source").IsProxyAdded(sourceproxy.SMProxy):
             return proxy
    # create a new helper
    proxy = servermanager.misc.RepresentationAnimationHelper(
      Source=sourceproxy)
    servermanager.ProxyManager().RegisterProxy(
      "pq_helper_proxies.%s" % sourceproxy.GetSelfIDAsString(),
      "RepresentationAnimationHelper", proxy)
    return proxy

def GetAnimationTrack(propertyname_or_property, index=None, proxy=None):
    """Returns an animation cue for the property. If one doesn't exist then a
    new one will be created.
    Typical usage:
        track = GetAnimationTrack("Center", 0, sphere) or
        track = GetAnimationTrack(sphere.GetProperty("Radius")) or

        # this returns the track to animate visibility of the active source in
        # all views.
        track = GetAnimationTrack("Visibility")

     For animating properties on implicit planes etc., use the following
     signatures:
        track = GetAnimationTrack(slice.SliceType.GetProperty("Origin"), 0) or
        track = GetAnimationTrack("Origin", 0, slice.SliceType)

    """
    if not proxy:
        proxy = GetActiveSource()
    if not isinstance(proxy, servermanager.Proxy):
        raise TypeError, "proxy must be a servermanager.Proxy instance"
    if isinstance(propertyname_or_property, str):
        propertyname = propertyname_or_property
    elif isinstance(propertyname_or_property, servermanager.Property):
        prop = propertyname_or_property
        propertyname = prop.Name
        proxy = prop.Proxy
    else:
        raise TypeError, "propertyname_or_property must be a string or servermanager.Property"

    # To handle the case where the property is actually a "display" property, in
    # which case we are actually animating the "RepresentationAnimationHelper"
    # associated with the source.
    if propertyname in ["Visibility", "Opacity"]:
        proxy = _GetRepresentationAnimationHelper(proxy)
    if not proxy or not proxy.GetProperty(propertyname):
        raise AttributeError, "Failed to locate property %s" % propertyname

    scene = GetAnimationScene()
    for cue in scene.Cues:
        try:
            if cue.AnimatedProxy == proxy and\
               cue.AnimatedPropertyName == propertyname:
                if index == None or index == cue.AnimatedElement:
                    return cue
        except AttributeError:
            pass

    # matching animation track wasn't found, create a new one.
    cue = KeyFrameAnimationCue()
    cue.AnimatedProxy = proxy
    cue.AnimatedPropertyName = propertyname
    if index != None:
        cue.AnimatedElement = index
    scene.Cues.append(cue)
    return cue

def GetCameraTrack(view=None):
    """Returns the camera animation track for the given view. If no view is
    specified, active view will be used. If no exisiting camera animation track
    is found, a new one will be created."""
    if not view:
        view = GetActiveView()
    if not view:
        raise ValueError, "No view specified"
    scene = GetAnimationScene()
    for cue in scene.Cues:
        if cue.AnimatedProxy == view and\
           cue.GetXMLName() == "CameraAnimationCue":
            return cue
    # no cue was found, create a new one.
    cue = CameraAnimationCue()
    cue.AnimatedProxy = view
    scene.Cues.append(cue)
    return cue

def GetTimeTrack():
    """Returns the animation track used to control the time requested from all
    readers/filters during playback.
    This is the "TimeKeeper - Time" track shown in ParaView's 'Animation View'.
    If none exists, a new one will be created."""
    scene = GetAnimationScene()
    tk = scene.TimeKeeper
    for cue in scene.Cues:
        if cue.GetXMLName() == "TimeAnimationCue" and cue.AnimatedProxy == tk:
            return cue
    # no cue was found, create a new one.
    cue = TimeAnimationCue()
    cue.AnimatedProxy = tk
    scene.Cues.append(cue)
    return cue

def LoadXML(xmlstring, ns=None):
    """Given a server manager XML as a string, parse and process it.
    If you loaded the simple module with from paraview.simple import *,
    make sure to pass globals() as the second arguments:
    LoadXML(xmlstring, globals())
    Otherwise, the new functions will not appear in the global namespace."""
    if not ns:
        ns = globals()
    servermanager.LoadXML(xmlstring)
    _add_functions(ns)

def LoadPlugin(filename, remote=True, ns=None):
    """Loads a ParaView plugin and updates this module with new constructors
    if any. The remote argument (default to True) is to specify whether 
    the plugin will be loaded on client (remote=False) or on server (remote=True).
    If you loaded the simple module with from paraview.simple import *,
    make sure to pass globals() as an argument:
    LoadPlugin("myplugin", False, globals()), to load on client;
    LoadPlugin("myplugin", True, globals()), to load on server; 
    LoadPlugin("myplugin", ns=globals()), to load on server.
    Otherwise, the new functions will not appear in the global namespace."""
    
    if not ns:
        ns = globals()
    servermanager.LoadPlugin(filename, remote)
    _add_functions(ns)

class ActiveObjects(object):
    """This class manages the active objects (source and view). The active
    objects are shared between Python and the user interface. This class
    is for internal use. Use the Set/Get methods for setting and getting
    active objects."""
    def __get_selection_model(self, name):
        "Internal method."
        pxm = servermanager.ProxyManager()
        model = pxm.GetSelectionModel(name)
        if not model:
            model = servermanager.vtkSMProxySelectionModel()
            pxm.RegisterSelectionModel(name, model)
        return model

    def set_view(self, view):
        "Sets the active view."
        active_view_model = self.__get_selection_model("ActiveView") 
        if view:
            active_view_model.SetCurrentProxy(view.SMProxy, 0)
        else:
            active_view_model.SetCurrentProxy(None, 0)

    def get_view(self):
        "Returns the active view."
        return servermanager._getPyProxy(
            self.__get_selection_model("ActiveView").GetCurrentProxy())

    def set_source(self, source):
        "Sets the active source."
        active_sources_model = self.__get_selection_model("ActiveSources") 
        if source:
            # 3 == CLEAR_AND_SELECT
            active_sources_model.SetCurrentProxy(source.SMProxy, 3)
        else:
            active_sources_model.SetCurrentProxy(None, 3)

    def __convert_proxy(self, px):
        "Internal method."
        if not px:
            return None
        if px.IsA("vtkSMSourceProxy"):
            return servermanager._getPyProxy(px)
        else:
            return servermanager.OutputPort(
              servermanager._getPyProxy(px.GetSourceProxy()),
              px.GetPortIndex())
        
    def get_source(self):
        "Returns the active source."
        return self.__convert_proxy(
          self.__get_selection_model("ActiveSources").GetCurrentProxy())

    def get_selected_sources(self):
        "Returns the set of sources selected in the pipeline browser."
        model = self.__get_selection_model("ActiveSources")
        proxies = []
        for i in xrange(model.GetNumberOfSelectedProxies()):
            proxies.append(self.__convert_proxy(model.GetSelectedProxy(i)))
        return proxies

    view = property(get_view, set_view)
    source = property(get_source, set_source)

class _funcs_internals:
    "Internal class."
    first_render = True
    view_counter = 0
    rep_counter = 0

def demo1():
    """Simple demo that create the following pipeline
    sphere - shrink - \
                       - append
    cone            - /
    """
    # Create a sphere of radius = 2, theta res. = 32
    # This object becomes the active source.
    ss = Sphere(Radius=2, ThetaResolution=32)
    # Apply the shrink filter. The Input property is optional. If Input
    # is not specified, the filter is applied to the active source.
    shr = Shrink(Input=ss)
    # Create a cone source.
    cs = Cone()
    # Append cone and shrink
    app = AppendDatasets()
    app.Input = [shr, cs]
    # Show the output of the append filter. The argument is optional
    # as the app filter is now the active object.
    Show(app)
    # Render the default view.
    Render()

def demo2(fname="/Users/berk/Work/ParaView/ParaViewData/Data/disk_out_ref.ex2"):
    """This demo shows the use of readers, data information and display
    properties."""
    
    # Create the exodus reader and specify a file name
    reader = ExodusIIReader(FileName=fname)
    # Get the list of point arrays.
    avail = reader.PointVariables.Available
    print avail
    # Select all arrays
    reader.PointVariables = avail

    # Turn on the visibility of the reader
    Show(reader)
    # Set representation to wireframe
    SetDisplayProperties(Representation = "Wireframe")
    # Black background is not pretty
    SetViewProperties(Background = [0.4, 0.4, 0.6])
    Render()
    # Change the elevation of the camera. See VTK documentation of vtkCamera
    # for camera parameters.
    # NOTE: THIS WILL BE SIMPLER
    GetActiveCamera().Elevation(45)
    Render()
    # Now that the reader executed, let's get some information about it's
    # output.
    pdi = reader[0].PointData
    # This prints a list of all read point data arrays as well as their
    # value ranges.
    print 'Number of point arrays:', len(pdi)
    for i in range(len(pdi)):
        ai = pdi[i]
        print "----------------"
        print "Array:", i, " ", ai.Name, ":"
        numComps = ai.GetNumberOfComponents()
        print "Number of components:", numComps
        for j in range(numComps):
            print "Range:", ai.GetRange(j)
    # White is boring. Let's color the geometry using a variable.
    # First create a lookup table. This object controls how scalar
    # values are mapped to colors. See VTK documentation for
    # details.
    # Map min (0.00678) to blue, max (0.0288) to red
    SetDisplayProperties(LookupTable = MakeBlueToRedLT(0.00678, 0.0288))
    # Color by point array called Pres
    SetDisplayProperties(ColorAttributeType = "POINT_DATA")
    SetDisplayProperties(ColorArrayName = "Pres")
    Render()

_add_functions(globals())
active_objects = ActiveObjects()

if not servermanager.ActiveConnection:
    Connect()
