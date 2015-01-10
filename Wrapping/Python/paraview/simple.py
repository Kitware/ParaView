r"""simple is a module for using paraview server manager in Python. It
provides a simple convenience layer to functionality provided by the
C++ classes wrapped to Python as well as the servermanager module.

A simple example::

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
import servermanager
import lookuptable


def GetParaViewVersion():
    """Returns the version of the ParaView build"""
    return paraview._version(servermanager.vtkSMProxyManager.GetVersionMajor(),
                             servermanager.vtkSMProxyManager.GetVersionMinor())
def GetParaViewSourceVersion():
    """Returns the paraview source version string e.g.
    'paraview version x.x.x, Date: YYYY-MM-DD'."""
    return servermanager.vtkSMProxyManager.GetParaViewSourceVersion()


#==============================================================================
# Client/Server Connection methods
#==============================================================================

def Disconnect(ns=None, force=True):
    """Free the current active session"""
    if not ns:
        ns = globals()

    supports_simutaneous_connections =\
        servermanager.vtkProcessModule.GetProcessModule().GetMultipleSessionsSupport()
    if not force and supports_simutaneous_connections:
        # This is an internal Disconnect request that doesn't need to happen in
        # multi-server setup. Ignore it.
        return
    if servermanager.ActiveConnection:
        _remove_functions(ns)
        servermanager.Disconnect()
        import gc
        gc.collect()

# -----------------------------------------------------------------------------

def Connect(ds_host=None, ds_port=11111, rs_host=None, rs_port=11111):
    """Creates a connection to a server. Example usage::

    > Connect("amber") # Connect to a single server at default port
    > Connect("amber", 12345) # Connect to a single server at port 12345
    > Connect("amber", 11111, "vis_cluster", 11111) # connect to data server, render server pair"""
    Disconnect(globals(), False)
    connection = servermanager.Connect(ds_host, ds_port, rs_host, rs_port)
    _initializeSession(connection)
    _add_functions(globals())
    return connection

# -----------------------------------------------------------------------------

def ReverseConnect(port=11111):
    """Create a reverse connection to a server.  Listens on port and waits for
    an incoming connection from the server."""
    Disconnect(globals(), False)
    connection = servermanager.ReverseConnect(port)
    _initializeSession(connection)
    _add_functions(globals())
    return connection

#==============================================================================
# Multi-servers
#==============================================================================

def SetActiveConnection(connection=None, ns=None):
    """Set the active connection. If the process was run without multi-server
       enabled and this method is called with a non-None argument while an
       ActiveConnection is present, it will raise a RuntimeError."""
    if not ns:
        ns = globals()
    if servermanager.ActiveConnection != connection:
        _remove_functions(ns)
        servermanager.SetActiveConnection(connection)
        _add_functions(ns)

#==============================================================================
# Views and Layout methods
#==============================================================================

def CreateView(view_xml_name, **params):
    "Creates and returns the specified proxy view based on its name/label."
    view = servermanager._create_view(view_xml_name)
    if not view:
        raise RuntimeError, "Failed to create requested view", view_xml_name

    try:
        registrationName = params["registrationName"]
        del params["registrationName"]
    except KeyError:
        try:
            registrationName = params["guiName"]
            del params["guiName"]
        except KeyError:
            registrationName = None

    controller = servermanager.ParaViewPipelineController()
    controller.PreInitializeProxy(view)
    SetProperties(view, **params)
    controller.PostInitializeProxy(view)
    controller.RegisterViewProxy(view, registrationName)
    return view

# -----------------------------------------------------------------------------

def CreateRenderView(**params):
    """"Create standard 3D render view"""
    return CreateView("RenderView", **params)

# -----------------------------------------------------------------------------

def CreateXYPlotView(**params):
    """Create XY plot Chart view"""
    return CreateView("XYChartView", **params)

# -----------------------------------------------------------------------------

def CreateBarChartView(**params):
    """"Create Bar Chart view"""
    return CreateView("XYBarChartView", **params)

# -----------------------------------------------------------------------------

def CreateComparativeRenderView(**params):
    """"Create Comparative view"""
    return CreateView("ComparativeRenderView", **params)

# -----------------------------------------------------------------------------

def CreateComparativeXYPlotView(**params):
    """"Create comparative XY plot Chart view"""
    return CreateView("ComparativeXYPlotView", **params)

# -----------------------------------------------------------------------------

def CreateComparativeBarChartView(**params):
    """"Create comparative Bar Chart view"""
    return CreateView("ComparativeBarChartView", **params)

# -----------------------------------------------------------------------------

def CreateParallelCoordinatesChartView(**params):
    """"Create Parallele coordinate Chart view"""
    return CreateView("ParallelCoordinatesChartView", **params)

# -----------------------------------------------------------------------------

def Create2DRenderView(**params):
    """"Create the standard 3D render view with the 2D interaction mode turned ON"""
    return CreateView("2DRenderView", **params)

# -----------------------------------------------------------------------------

def GetRenderView():
    "Returns the active view if there is one. Else creates and returns a new view."
    view = active_objects.view
    if not view:
        # it's possible that there's no active view, but a render view exists.
        # If so, locate that and return it (before trying to create a new one).
        view = servermanager.GetRenderView()
    if not view:
        view = CreateRenderView()
    return view

# -----------------------------------------------------------------------------

def GetRenderViews():
    "Returns all render views as a list."
    return servermanager.GetRenderViews()

def GetViews(viewtype=None):
    """Returns all views. If viewtype is specified, only the views of the
       specified type are returned"""
    val = []
    for aProxy in servermanager.ProxyManager().GetProxiesInGroup("views").values():
        if aProxy.IsA("vtkSMViewProxy") and \
            (viewtype is None or aProxy.GetXMLName() == viewtype):
            val.append(aProxy)
    return val

# -----------------------------------------------------------------------------

def SetViewProperties(view=None, **params):
    """Sets one or more properties of the given view. If an argument
    is not provided, the active view is used. Pass a list of property_name=value
    pairs to this function to set property values. For example::

        SetProperties(Background=[1, 0, 0], UseImmediateMode=0)
    """
    if not view:
        view = active_objects.view
    SetProperties(view, **params)

# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------
def RenderAllViews():
    """Render all views"""
    for view in GetViews(): Render(view)

# -----------------------------------------------------------------------------

def ResetCamera(view=None):
    """Resets the settings of the camera to preserver orientation but include
    the whole scene. If an argument is not provided, the active view is
    used."""
    if not view:
        view = active_objects.view
    if hasattr(view, "ResetCamera"):
        view.ResetCamera()
    if hasattr(view, "ResetDisplay"):
        view.ResetDisplay()
    Render(view)

# -----------------------------------------------------------------------------

def GetLayouts():
    """Returns the layout proxies on the active session.
    Layout proxies are used to place views in a grid."""
    return servermanager.ProxyManager().GetProxiesInGroup("layouts")

# -----------------------------------------------------------------------------

def GetLayout(view=None):
    """Return the layout containing the give view, if any.
    If no view is specified, active view is used.
    """
    if not view:
        view = GetActiveView()
    if not view:
        raise RuntimeError, "No active view was found."
    layouts = GetLayouts()
    for layout in layouts.values():
        if layout.GetViewLocation(view) != -1:
            return layout
    return None


def GetViewsInLayout(layout=None):
    """Returns a list of views in the given layout. If not layout is specified,
    the layout for the active view is used, if possible."""
    layout = layout if layout else GetLayout()
    if not layout:
        raise RuntimeError, "Layout couldn't be determined. Please specify a valid layout."
    views = GetViews()
    return [x for x in views if layout.GetViewLocation(x) != -1]

# -----------------------------------------------------------------------------

def RemoveViewsAndLayouts():
    pxm = servermanager.ProxyManager()
    layouts = pxm.GetProxiesInGroup("layouts")

    for view in GetRenderViews():
        Delete(view)

    # Can not use regular delete for layouts
    for name, id in layouts:
        proxy = layouts[(name, id)]
        pxm.UnRegisterProxy('layouts', name, layouts[(name, id)])

#==============================================================================
# XML State management
#==============================================================================

def LoadState(filename, connection=None):
    RemoveViewsAndLayouts()
    servermanager.LoadState(filename, connection)
    # Try to set the new view active
    if len(GetRenderViews()) > 0:
        SetActiveView(GetRenderViews()[0])

# -----------------------------------------------------------------------------

def SaveState(filename):
    servermanager.SaveXMLState(filename)

#==============================================================================
# Representation methods
#==============================================================================

def GetRepresentation(proxy=None, view=None):
    """"Given a pipeline object and view, returns the corresponding representation object.
    If pipeline object and view are not specified, active objects are used."""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError, "view argument cannot be None."
    if not proxy:
        proxy = active_objects.source
    if not proxy:
        raise ValueError, "proxy argument cannot be None."
    rep = servermanager.GetRepresentation(proxy, view)
    if not rep:
        controller = servermanager.ParaViewPipelineController()
        return controller.Show(proxy, proxy.Port, view)
    return rep

# -----------------------------------------------------------------------------
def GetDisplayProperties(proxy=None, view=None):
    """"Given a pipeline object and view, returns the corresponding representation object.
    If pipeline object and/or view are not specified, active objects are used."""
    return GetRepresentation(proxy, view)

# -----------------------------------------------------------------------------
def Show(proxy=None, view=None, **params):
    """Turns the visibility of a given pipeline object on in the given view.
    If pipeline object and/or view are not specified, active objects are used."""
    if proxy == None:
        proxy = GetActiveSource()
    if proxy == None:
        raise RuntimeError, "Show() needs a proxy argument or that an active source is set."
    if not view:
        # it here's now active view, controller.Show() will create a new preferred view.
        # if possible.
        view = active_objects.view
    controller = servermanager.ParaViewPipelineController()
    rep = controller.Show(proxy, proxy.Port, view)
    if rep == None:
        raise RuntimeError, "Could not create a representation object for proxy %s" % proxy.GetXMLLabel()
    for param in params.keys():
        setattr(rep, param, params[param])
    return rep

# -----------------------------------------------------------------------------
def Hide(proxy=None, view=None):
    """Turns the visibility of a given pipeline object off in the given view.
    If pipeline object and/or view are not specified, active objects are used."""
    if not proxy:
      proxy = active_objects.source
    if not view:
        view = active_objects.view
    if not proxy:
        raise ValueError, "proxy argument cannot be None when no active source is present."
    controller = servermanager.ParaViewPipelineController()
    controller.Hide(proxy, proxy.Port, view)

# -----------------------------------------------------------------------------
def SetDisplayProperties(proxy=None, view=None, **params):
    """Sets one or more display properties of the given pipeline object. If an argument
    is not provided, the active source is used. Pass a list of property_name=value
    pairs to this function to set property values. For example::

        SetProperties(Color=[1, 0, 0], LineWidth=2)
    """
    rep = GetDisplayProperties(proxy, view)
    SetProperties(rep, **params)

# -----------------------------------------------------------------------------
def ColorBy(rep=None, value=None):
    """Set scalar color. This will automatically setup the color maps and others
    necessary state for the representations. 'rep' must be the display
    properties proxy i.e. the value returned by GetDisplayProperties() function.
    If none is provided the display properties for the active source will be
    used, if possible."""
    rep = rep if rep else GetDisplayProperties()
    if not rep:
        raise ValueError, "No display properties can be determined."

    association = rep.ColorArrayName.GetAssociation()
    arrayname = rep.ColorArrayName.GetArrayName()

    if value == None:
        rep.SetScalarColoring(None, servermanager.GetAssociationFromString(association))
        return
    if not isinstance(value, tuple) and not isinstance(value, list):
        value = (value,)
    if len(value) == 1:
        arrayname = value[0]
    else:
        association = value[0]
        arrayname = value[1]
    rep.SetScalarColoring(arrayname, servermanager.GetAssociationFromString(association))


# -----------------------------------------------------------------------------
def _DisableFirstRenderCameraReset():
    """Disable the first render camera reset.  Normally a ResetCamera is called
    automatically when Render is called for the first time after importing
    this module."""
    _funcs_internals.first_render = False

#==============================================================================
# Proxy handling methods
#==============================================================================

def SetProperties(proxy=None, **params):
    """Sets one or more properties of the given pipeline object. If an argument
    is not provided, the active source is used. Pass a list of property_name=value
    pairs to this function to set property values. For example::

        SetProperties(Center=[1, 2, 3], Radius=3.5)
    """
    if not proxy:
        proxy = active_objects.source
    for param in params.keys():
        if not hasattr(proxy, param):
            raise AttributeError("object has no property %s" % param)
        setattr(proxy, param, params[param])

# -----------------------------------------------------------------------------

def GetProperty(*arguments, **keywords):
    """Get one property of the given pipeline object. If keywords are used,
       you can set the proxy and the name of the property that you want to get
       like in the following example::

            GetProperty({proxy=sphere, name="Radius"})

       If it's arguments that are used, then you have two case:
         - if only one argument is used that argument will be
           the property name.
         - if two arguments are used then the first one will be
           the proxy and the second one the property name.
       Several example are given below::

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

# -----------------------------------------------------------------------------
def GetDisplayProperty(*arguments, **keywords):
    """Same as GetProperty, except that if no 'proxy' is passed, it will use
    the active display properties, rather than the active source"""
    proxy = None
    name = None
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
    if not proxy:
        proxy = GetDisplayProperties()
    return GetProperty(proxy, name)

# -----------------------------------------------------------------------------
def GetViewProperty(*arguments, **keywords):
    """Same as GetProperty, except that if no 'proxy' is passed, it will use
    the active view properties, rather than the active source"""
    proxy = None
    name = None
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
    if not proxy:
        proxy = GetViewProperties()
    return GetProperty(proxy, name)

# -----------------------------------------------------------------------------
def GetViewProperties(view=None):
    """"Same as GetActiveView(), this API is provided just for consistency with
    GetDisplayProperties()."""
    return GetActiveView()

#==============================================================================
# ServerManager methods
#==============================================================================

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

# -----------------------------------------------------------------------------

def FindSource(name):
    """
    Return a proxy base on the name that was used to register it
    into the ProxyManager.
    Example usage::

       Cone(guiName='MySuperCone')
       Show()
       Render()
       myCone = FindSource('MySuperCone')
    """
    return servermanager.ProxyManager().GetProxy("sources", name)

def FindView(name):
    """
    Return a view proxy on the name that was used to register it
    into the ProxyManager.
    Example usage::

       CreateRenderView(guiName='RenderView1')
       myView = FindSource('RenderView1')
    """
    return servermanager.ProxyManager().GetProxy("views", name)


def GetActiveViewOrCreate(viewtype):
    """
    Returns the active view, if the active view is of the given type,
    otherwise creates a new view of the requested type."""
    view = GetActiveView()
    if view is None or view.GetXMLName() != viewtype:
        view = CreateView(viewtype)
    if not view:
        raise RuntimeError, "Failed to create/locate the specified view"
    return view

def FindViewOrCreate(name, viewtype):
    """
    Returns the view, if a view with the given name exists and is of the
    the given type, otherwise creates a new view of the requested type."""
    view = FindView(name)
    if view is None or view.GetXMLName() != viewtype:
        view = CreateView(viewtype)
    if not view:
        raise RuntimeError, "Failed to create/locate the specified view"
    return view


def LocateView(displayProperties=None):
    """
    Given a displayProperties object i.e. the object returned by
    GetDisplayProperties() or Show() functions, this function will locate a view
    to which the displayProperties object corresponds."""
    if displayProperties is None:
        displayProperties = GetDisplayProperties()
    if displayProperties is None:
        raise ValueError, "'displayProperties' must be set"
    for view in GetViews():
        try:
            if displayProperties in view.Representations: return view
        except AttributeError:
            pass
    return None



# -----------------------------------------------------------------------------

def GetSources():
    """Given the name of a source, return its Python object."""
    return servermanager.ProxyManager().GetProxiesInGroup("sources")

# -----------------------------------------------------------------------------

def GetRepresentations():
    """Returns all representations (display properties)."""
    return servermanager.ProxyManager().GetProxiesInGroup("representations")

# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------

def Delete(proxy=None):
    """Deletes the given pipeline object or the active source if no argument
    is specified."""
    if not proxy:
        proxy = active_objects.source
    if not proxy:
        raise RuntimeError, "Could not locate proxy to 'Delete'"
    controller = servermanager.ParaViewPipelineController()
    controller.UnRegisterProxy(proxy)


#==============================================================================
# Active Source / View / Camera / AnimationScene
#==============================================================================

def GetActiveView():
    """.. _GetActiveView:
        Returns the active view."""
    return active_objects.view

# -----------------------------------------------------------------------------

def SetActiveView(view):
    """.. _SetActiveView:
        Sets the active view."""
    active_objects.view = view

# -----------------------------------------------------------------------------

def GetActiveSource():
    """.. _GetActiveSource:
        Returns the active source."""
    return active_objects.source

# -----------------------------------------------------------------------------

def SetActiveSource(source):
    """.. _SetActiveSource:
        Sets the active source."""
    active_objects.source = source

# -----------------------------------------------------------------------------

def GetActiveCamera():
    """Returns the active camera for the active view. The returned object
    is an instance of vtkCamera."""
    return GetActiveView().GetActiveCamera()

#==============================================================================
# I/O methods
#==============================================================================

def OpenDataFile(filename, **extraArgs):
    """Creates a reader to read the give file, if possible.
       This uses extension matching to determine the best reader possible.
       If a reader cannot be identified, then this returns None."""
    session = servermanager.ActiveConnection.Session
    reader_factor = servermanager.vtkSMProxyManager.GetProxyManager().GetReaderFactory()
    if reader_factor.GetNumberOfRegisteredPrototypes() == 0:
        reader_factor.UpdateAvailableReaders()
    first_file = filename
    if type(filename) == list:
        first_file = filename[0]
    if not reader_factor.TestFileReadability(first_file, session):
        msg = "File not readable: %s " % first_file
        raise RuntimeError, msg
    if not reader_factor.CanReadFile(first_file, session):
        msg = "File not readable. No reader found for '%s' " % first_file
        raise RuntimeError, msg
    prototype = servermanager.ProxyManager().GetPrototypeProxy(
      reader_factor.GetReaderGroup(), reader_factor.GetReaderName())
    xml_name = paraview.make_name_valid(prototype.GetXMLLabel())
    reader_func = _create_func(xml_name, servermanager.sources)
    pname = servermanager.vtkSMCoreUtilities.GetFileNameProperty(prototype)
    if pname:
        extraArgs[pname] = filename
        reader = reader_func(**extraArgs)
    return reader

# -----------------------------------------------------------------------------

def CreateWriter(filename, proxy=None, **extraArgs):
    """Creates a writer that can write the data produced by the source proxy in
       the given file format (identified by the extension). If no source is
       provided, then the active source is used. This doesn't actually write the
       data, it simply creates the writer and returns it."""
    if not filename:
       raise RuntimeError, "filename must be specified"
    session = servermanager.ActiveConnection.Session
    writer_factory = servermanager.vtkSMProxyManager.GetProxyManager().GetWriterFactory()
    if writer_factory.GetNumberOfRegisteredPrototypes() == 0:
        writer_factory.UpdateAvailableWriters()
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError, "Could not locate source to write"
    writer_proxy = writer_factory.CreateWriter(filename, proxy.SMProxy, proxy.Port)
    writer_proxy.UnRegister(None)
    pyproxy = servermanager._getPyProxy(writer_proxy)
    if pyproxy and extraArgs:
        SetProperties(pyproxy, **extraArgs)
    return pyproxy


def SaveData(filename, proxy=None, **extraArgs):
    """Save data produced by 'proxy' in a file. If no proxy is specified the
    active source is used. Properties to configure the writer can be passed in
    as keyword arguments. Example usage::

        SaveData("sample.pvtp", source0)
        SaveData("sample.csv", FieldAssociation="Points")
    """
    writer = CreateWriter(filename, proxy, **extraArgs)
    if not writer:
        raise RuntimeError, "Could not create writer for specified file or data type"
    writer.UpdateVTKObjects()
    writer.UpdatePipeline()
    del writer


# -----------------------------------------------------------------------------

def WriteImage(filename, view=None, **params):
    """Saves the given view (or the active one if none is given) as an
    image. Optionally, you can specify the writer and the magnification
    using the Writer and Magnification named arguments. For example::

        WriteImage("foo.mypng", aview, Writer=vtkPNGWriter, Magnification=2)

    If no writer is provided, the type is determined from the file extension.
    Currently supported extensions are png, bmp, ppm, tif, tiff, jpg and jpeg.
    The writer is a VTK class that is capable of writing images.
    Magnification is used to determine the size of the written image. The size
    is obtained by multiplying the size of the view with the magnification.
    Rendering may be done using tiling to obtain the correct size without
    resizing the view.

    ** DEPRECATED: Use SaveScreenshot() instead. **
    """
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

# -----------------------------------------------------------------------------
def SaveScreenshot(filename,
    view=None, layout=None, magnification=None, quality=None, **params):
    if not view is None and not layout is None:
        raise ValueError, "both view and layout cannot be specified"

    viewOrLayout = view if view else layout
    viewOrLayout = viewOrLayout if viewOrLayout else GetActiveView()
    if not viewOrLayout:
        raise ValueError, "view or layout needs to be specified"
    try:
        magnification = int(magnification) if int(magnification) > 0 else 1
    except TypeError:
        magnification = 1
    try:
        quality = int(quality)
    except TypeError:
        quality = -1

    controller = servermanager.ParaViewPipelineController()
    return controller.WriteImage(\
        viewOrLayout, filename, magnification, quality)

# -----------------------------------------------------------------------------

def WriteAnimation(filename, **params):
    """Writes the current animation as a file. Optionally one can specify
    arguments that qualify the saved animation files as keyword arguments.
    Accepted options are as follows:

    * **Magnification** *(integer)* : set the maginification factor for the saved
      animation.
    * **Quality** *(0 [worst] or 1 or 2 [best])* : set the quality of the generated
      movie (if applicable).
    * **Subsampling** *(integer)* : setting whether the movie encoder should use
      subsampling of the chrome planes or not, if applicable. Since the human
      eye is more sensitive to brightness than color variations, subsampling
      can be useful to reduce the bitrate. Default value is 0.
    * **BackgroundColor** *(3-tuple of doubles)* : set the RGB background color to
      use to fill empty spaces in the image.
    * **FrameRate** *(double)*: set the frame rate (if applicable).
    * **StartFileCount** *(int)*: set the first number used for the file name
      (23 => i.e. image-0023.png).
    * **PlaybackTimeWindow** *([double, double])*: set the time range that
      should be used to play a subset of the total animation time.
      (By default the whole application will play).
    """
    scene = GetAnimationScene()
    # ensures that the TimeKeeper track is created.
    GetTimeTrack()
    iw = servermanager.vtkSMAnimationSceneImageWriter()
    iw.SetAnimationScene(scene.SMProxy)
    iw.SetFileName(filename)
    if params.has_key("Magnification"):
        iw.SetMagnification(int(params["Magnification"]))
    if params.has_key("Quality"):
        iw.SetQuality(int(params["Quality"]))
    if params.has_key("Subsampling"):
        iw.SetSubsampling(int(params["Subsampling"]))
    if params.has_key("BackgroundColor"):
        iw.SetBackgroundColor(params["BackgroundColor"])
    if params.has_key("FrameRate"):
        iw.SetFrameRate(float(params["FrameRate"]))
    iw.Save()

def WriteAnimationGeometry(filename, view=None):
    """Save the animation geometry from a specific view to a file specified.
    The animation geometry is written out as a PVD file. If no view is
    specified, the active view will be used of possible."""
    view = view if view else GetActiveView()
    if not view:
        raise ValueError, "Please specify the view to use"
    scene = GetAnimationScene()
    writer = servermanager.vtkSMAnimationSceneGeometryWriter()
    writer.SetFileName(filename)
    writer.SetAnimationScene(scene.SMProxy)
    writer.SetViewModule(view.SMProxy)
    writer.Save()

#==============================================================================
# Lookup Table / Scalarbar methods
#==============================================================================
# -----------------------------------------------------------------------------
def HideUnusedScalarBars(view=None):
    """Hides all unused scalar bars from the view. A scalar bar is used if some
    data is shown in that view that is coloring using the transfer function
    shown by the scalar bar."""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError, "'view' argument cannot be None with no active is present."
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.UpdateScalarBars(view.SMProxy, tfmgr.HIDE_UNUSED_SCALAR_BARS)

def UpdateScalarBars(view=None):
    """Hides all unused scalar bar and shows used scalar bars. A scalar bar is used
    if some data is shown in that view that is coloring using the transfer function
    shown by the scalar bar."""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError, "'view' argument cannot be None with no active is present."
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.UpdateScalarBars(view.SMProxy, tfmgr.HIDE_UNUSED_SCALAR_BARS | tfmgr.SHOW_USED_SCALAR_BARS)

def GetScalarBar(ctf, view=None):
    """Returns the scalar bar for color transfer function in the given view.
    If view is None, the active view will be used, if possible.
    This will either return an existing scalar bar or create a new one."""
    view = view if view else active_objects.view
    if not view:
        raise ValueError, "'view' argument cannot be None when no active view is present"
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    sb = servermanager._getPyProxy(\
        tfmgr.GetScalarBarRepresentation(ctf.SMProxy, view.SMProxy))
    return sb

# -----------------------------------------------------------------------------
def GetColorTransferFunction(arrayname, **params):
    """Get the color transfer function used to mapping a data array with the
    given name to colors. This may create a new color transfer function
    if none exists, or return an existing one"""
    if not servermanager.ActiveConnection:
        raise RuntimeError, "Missing active session"
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    lut = servermanager._getPyProxy(\
            tfmgr.GetColorTransferFunction(arrayname, session.GetSessionProxyManager()))
    SetProperties(lut, **params)
    return lut

def GetOpacityTransferFunction(arrayname, **params):
    """Get the opacity transfer function used to mapping a data array with the
    given name to opacity. This may create a new opacity transfer function
    if none exists, or return an existing one"""
    if not servermanager.ActiveConnection:
        raise RuntimeError, "Missing active session"
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    otf = servermanager._getPyProxy(\
            tfmgr.GetOpacityTransferFunction(arrayname, session.GetSessionProxyManager()))
    SetProperties(otf, **params)
    return otf

# -----------------------------------------------------------------------------
def CreateLookupTable(**params):
    """Create and return a lookup table.  Optionally, parameters can be given
    to assign to the lookup table.
    """
    lt = servermanager.rendering.PVLookupTable()
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeProxy(lt)
    SetProperties(lt, **params)
    controller.RegisterColorTransferFunctionProxy(lt)
    return lt

# -----------------------------------------------------------------------------

def CreatePiecewiseFunction(**params):
    """Create and return a piecewise function.  Optionally, parameters can be
    given to assign to the piecewise function.
    """
    pfunc = servermanager.piecewise_functions.PiecewiseFunction()
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeProxy(pfunc)
    SetProperties(pfunc, **params)
    controller.RegisterOpacityTransferFunction(pfunc)
    return pfunc

# -----------------------------------------------------------------------------

def GetLookupTableForArray(arrayname, num_components, **params):
    """Used to get an existing lookuptable for a array or to create one if none
    exists. Keyword arguments can be passed in to initialize the LUT if a new
    one is created.
    *** DEPRECATED ***: Use GetColorTransferFunction instead"""
    return GetColorTransferFunction(arrayname, **params)


# global lookup table reader instance
# the user can use the simple api below
# rather than creating a lut reader themself
_lutReader = None
def _GetLUTReaderInstance():
    """ Internal api. Return the lookup table reader singleton. Create
    it if needed."""
    global _lutReader
    if _lutReader is None:
      _lutReader = lookuptable.vtkPVLUTReader()
    return _lutReader

# -----------------------------------------------------------------------------

def AssignLookupTable(arrayObject, LUTName, rangeOveride=[]):
    """Assign a lookup table to an array by lookup table name. The array
    may ber obtained from a ParaView source in it's point or cell data.
    The lookup tables available in ParaView's GUI are loaded by default.
    To get a list of the available lookup table names see GetLookupTableNames.
    To load a custom lookup table see LoadLookupTable."""
    return _GetLUTReaderInstance().GetLUT(arrayObject, LUTName, rangeOveride)

# -----------------------------------------------------------------------------

def GetLookupTableNames():
    """Return a list containing the currently available lookup table names.
    A name maybe used to assign a lookup table to an array. See
    AssignLookupTable.
    """
    return _GetLUTReaderInstance().GetLUTNames()

# -----------------------------------------------------------------------------

def LoadLookupTable(fileName):
    """Read the lookup tables in the named file and append them to the
    global collection of lookup tables. The newly loaded lookup tables
    may then be used with AssignLookupTable function.
    """
    return _GetLUTReaderInstance().Read(fileName)

# -----------------------------------------------------------------------------

def CreateScalarBar(**params):
    """Create and return a scalar bar widget.  The returned widget may
    be added to a render view by appending it to the view's representations
    The widget must have a valid lookup table before it is added to a view.
    It is possible to pass the lookup table (and other properties) as arguments
    to this method::

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

# -----------------------------------------------------------------------------

# TODO: Change this to take the array name and number of components. Register
# the lt under the name ncomp.array_name
def MakeBlueToRedLT(min, max):
    """
    Create a LookupTable that go from blue to red using the scalar range
    provided by the min and max arguments.
    """
    # Define RGB points. These are tuples of 4 values. First one is
    # the scalar values, the other 3 the RGB values.
    rgbPoints = [min, 0, 0, 1, max, 1, 0, 0]
    return CreateLookupTable(RGBPoints=rgbPoints, ColorSpace="HSV")

#==============================================================================
# CameraLink methods
#==============================================================================

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

# -----------------------------------------------------------------------------

def RemoveCameraLink(linkName):
    """Remove a camera link with the given name."""
    servermanager.ProxyManager().UnRegisterLink(linkName)

#==============================================================================
# Animation methods
#==============================================================================

def GetAnimationScene():
    """Returns the application-wide animation scene. ParaView has only one
    global animation scene. This method provides access to that. Users are
    free to create additional animation scenes directly, but those scenes
    won't be shown in the ParaView GUI."""
    if not servermanager.ActiveConnection:
        raise RuntimeError, "Missing active session"
    session = servermanager.ActiveConnection.Session
    controller = servermanager.ParaViewPipelineController()
    return controller.GetAnimationScene(session)

# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------

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
    # helper must have been created during RegisterPipelineProxy().
    return None

# -----------------------------------------------------------------------------

def GetAnimationTrack(propertyname_or_property, index=None, proxy=None):
    """Returns an animation cue for the property. If one doesn't exist then a
    new one will be created.
    Typical usage::

      track = GetAnimationTrack("Center", 0, sphere) or
      track = GetAnimationTrack(sphere.GetProperty("Radius")) or

      # this returns the track to animate visibility of the active source in
      # all views.
      track = GetAnimationTrack("Visibility")

    For animating properties on implicit planes etc., use the following
    signatures::

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

# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------

def GetTimeTrack():
    """Returns the animation track used to control the time requested from all
    readers/filters during playback.
    This is the "TimeKeeper - Time" track shown in ParaView's 'Animation View'."""
    scene = GetAnimationScene()
    if not scene:
        raise RuntimeError, "Missing animation scene"
    controller = servermanager.ParaViewPipelineController()
    return controller.GetTimeAnimationTrack(scene)

#==============================================================================
# Plugin Management
#==============================================================================

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

# -----------------------------------------------------------------------------

def LoadPlugin(filename, remote=True, ns=None):
    """Loads a ParaView plugin and updates this module with new constructors
    if any. The remote argument (default to True) is to specify whether
    the plugin will be loaded on client (remote=False) or on server (remote=True).
    If you loaded the simple module with from paraview.simple import *,
    make sure to pass globals() as an argument::

        LoadPlugin("myplugin", False, globals()) # to load on client
        LoadPlugin("myplugin", True, globals())  # to load on server
        LoadPlugin("myplugin", ns=globals())     # to load on server

    Otherwise, the new functions will not appear in the global namespace."""

    if not ns:
        ns = globals()
    servermanager.LoadPlugin(filename, remote)
    _add_functions(ns)

# -----------------------------------------------------------------------------

def LoadDistributedPlugin(pluginname, remote=True, ns=None):
    """Loads a plugin that's distributed with the executable. This uses the
    information known about plugins distributed with ParaView to locate the
    shared library for the plugin to load. Raises a RuntimeError if the plugin
    was not found."""
    if not servermanager.ActiveConnection:
        raise RuntimeError, "Cannot load a plugin without a session."
    plm = servermanager.vtkSMProxyManager.GetProxyManager().GetPluginManager()
    if remote:
        session = servermanager.ActiveConnection.Session
        info = plm.GetRemoteInformation(session)
    else:
        info = plm.GetLocalInformation()
    for cc in range(0, info.GetNumberOfPlugins()):
        if info.GetPluginName(cc) == pluginname:
            return LoadPlugin(info.GetPluginFileName(cc), remote, ns)
    raise RuntimeError, "Plugin '%s' not found" % pluginname

#==============================================================================
# Selection Management
#==============================================================================
def _select(seltype, query=None, proxy=None):
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError, "No active source was found."

    if not query:
        # This ends up being true for all cells.
        query = "id >= 0"

    # Note, selSource is not registered with the proxy manager.
    selSource = servermanager.sources.SelectionQuerySource()
    selSource.FieldType = seltype
    selSource.QueryString = str(query)
    proxy.SMProxy.SetSelectionInput(proxy.Port, selSource.SMProxy, 0)
    return selSource

# -----------------------------------------------------------------------------

def SelectCells(query=None, proxy=None):
    """Select cells satisfying the query. If query is None, then all cells are
    selected. If proxy is None, then the active source is used."""
    return _select("CELL", query, proxy)

# -----------------------------------------------------------------------------

def SelectPoints(query=None, proxy=None):
    """Select points satisfying the query. If query is None, then all points are
    selected. If proxy is None, then the active source is used."""
    return _select("POINT", query, proxy)

# -----------------------------------------------------------------------------

def ClearSelection(proxy=None):
    """Clears the selection on the active source."""
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError, "No active source was found."
    proxy.SMProxy.SetSelectionInput(proxy.Port, None, 0)

#==============================================================================
# Miscellaneous functions.
#==============================================================================
def Show3DWidgets(proxy=None):
    """If possible in the current environment, this method will
    request the application to show the 3D widget(s) for proxy"""
    proxy = proxy if proxy else GetActiveSource()
    if not proxy:
        raise ValueError, "No 'proxy' was provided and no active source was found."
    proxy.InvokeEvent('UserEvent', "ShowWidget")

def Hide3DWidgets(proxy=None):
    """If possible in the current environment, this method will
    request the application to hide the 3D widget(s) for proxy"""
    proxy = proxy if proxy else GetActiveSource()
    if not proxy:
        raise ValueError, "No 'proxy' was provided and no active source was found."
    proxy.InvokeEvent('UserEvent', "HideWidget")

def ExportView(filename, view=None, **params):
    """Export a view to the specified output file."""
    view = view if view else GetActiveView()
    if not view:
        raise ValueError, "No 'view' was provided and no active view was found."
    if not filename:
        raise ValueError, "No filename specified"

    # ensure that the view is up-to-date.
    view.StillRender()
    helper = servermanager.vtkSMViewExportHelper()
    proxy = helper.CreateExporter(filename, view.SMProxy)
    if not proxy:
        raise RuntimeError, "Failed to create exporter for ", filename
    proxy.UnRegister(None)
    proxy = servermanager._getPyProxy(proxy)
    SetProperties(proxy, **params)
    proxy.Write()
    del proxy
    del helper

#==============================================================================
# Usage and demo code set
#==============================================================================

def demo1():
    """
    Simple demo that create the following pipeline::

       sphere - shrink +
       cone            + > append

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

# -----------------------------------------------------------------------------

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
    SetDisplayProperties(ColorArrayName = ("POINTS", "Pres"))
    Render()

#==============================================================================
# Set of Internal functions
#==============================================================================

def _initializeSession(connection):
    """Internal method used to initialize a session. Users don't need to
    call this directly. Whenever a new session is created this method is called
    by API in this module."""
    if not connection:
      raise RuntimeError, "'connection' cannot be empty."
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeSession(connection.Session)

def _create_func(key, module):
    "Internal function."

    def CreateObject(*input, **params):
        """This function creates a new proxy. For pipeline objects that accept inputs,
        all non-keyword arguments are assumed to be inputs. All keyword arguments are
        assumed to be property,value pairs and are passed to the new proxy."""

        # Create a controller instance.
        controller = servermanager.ParaViewPipelineController()


        # Instantiate the actual object from the given module.
        px = module.__dict__[key]()

        # preinitialize the proxy.
        controller.PreInitializeProxy(px)

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
        SetProperties(px, **params)

        # post initialize
        controller.PostInitializeProxy(px)

        # Register the proxy with the proxy manager (assuming we are only using
        # these functions for pipeline proxies or animation proxies.
        if isinstance(px, servermanager.SourceProxy):
            controller.RegisterPipelineProxy(px, registrationName)
        return px

    return CreateObject

# -----------------------------------------------------------------------------

def _create_doc(new, old):
    "Internal function."
    import string
    res = new + '\n'
    ts = []
    strpd = old.split('\n')
    for s in strpd:
        ts.append(s.lstrip())
    res += string.join(ts)
    res += '\n'
    return res

# -----------------------------------------------------------------------------

def _func_name_valid(name):
    "Internal function."
    valid = True
    for c in name:
        if c == '(' or c ==')':
            valid = False
            break
    return valid

# -----------------------------------------------------------------------------

def _add_functions(g):
    if not servermanager.ActiveConnection:
        return

    activeModule = servermanager.ActiveConnection.Modules
    for m in [activeModule.filters, activeModule.sources,
              activeModule.writers, activeModule.animation]:
        dt = m.__dict__
        for key in dt.keys():
            cl = dt[key]
            if not isinstance(cl, str):
                if not key in g and _func_name_valid(key):
                    #print "add %s function" % key
                    g[key] = _create_func(key, m)
                    exec "g[key].__doc__ = _create_doc(m.%s.__doc__, g[key].__doc__)" % key

# -----------------------------------------------------------------------------

def _get_generated_proxies():
    activeModule = servermanager.ActiveConnection.Modules
    proxies = []
    for m in [activeModule.filters, activeModule.sources,
              activeModule.writers, activeModule.animation]:
        dt = m.__dict__
        for key in dt.keys():
            cl = dt[key]
            if not isinstance(cl, str):
                if _func_name_valid(key):
                    proxies.append(key)
    return proxies
# -----------------------------------------------------------------------------

def _remove_functions(g):
    list = []
    if servermanager.ActiveConnection:
       list = [m for m in dir(servermanager.ActiveConnection.Modules) if m[0] != '_']

    for m in list:
        dt = servermanager.ActiveConnection.Modules.__dict__[m].__dict__
        for key in dt.keys():
            cl = dt[key]
            if not isinstance(cl, str) and g.has_key(key):
                g.pop(key)
                #print "remove %s function" % key

# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------

def _switchToActiveConnectionCallback(caller, event):
    """Callback called when the active session/connection changes in the
        ServerManager. We update the Python state to reflect the change."""
    if servermanager:
        session = servermanager.vtkSMProxyManager.GetProxyManager().GetActiveSession()
        connection = servermanager.GetConnectionFromSession(session)
        SetActiveConnection(connection)

#==============================================================================
# Set of Internal classes
#==============================================================================

class _active_session_observer:
    def __init__(self):
        pxm = servermanager.vtkSMProxyManager.GetProxyManager()
        self.ObserverTag = pxm.AddObserver(pxm.ActiveSessionChanged,
            _switchToActiveConnectionCallback)

    def __del__(self):
        if servermanager:
            servermanager.vtkSMProxyManager.GetProxyManager().RemoveObserver(self.ObserverTag)

# -----------------------------------------------------------------------------

class _active_objects(object):
    """This class manages the active objects (source and view). The active
    objects are shared between Python and the user interface. This class
    is for internal use. Use the :ref:`SetActiveSource`,
    :ref:`GetActiveSource`, :ref:`SetActiveView`, and :ref:`GetActiveView`
    methods for setting and getting active objects."""
    def __get_selection_model(self, name, session=None):
        "Internal method."
        if session and session != servermanager.ActiveConnection.Session:
            raise RuntimeError, "Try to set an active object with invalid active connection."
        pxm = servermanager.ProxyManager(session)
        model = pxm.GetSelectionModel(name)
        if not model:
            model = servermanager.vtkSMProxySelectionModel()
            pxm.RegisterSelectionModel(name, model)
        return model

    def set_view(self, view):
        "Sets the active view."
        active_view_model = self.__get_selection_model("ActiveView")
        if view:
            active_view_model = self.__get_selection_model("ActiveView", view.GetSession())
            active_view_model.SetCurrentProxy(view.SMProxy,
                active_view_model.CLEAR_AND_SELECT)
        else:
            active_view_model = self.__get_selection_model("ActiveView")
            active_view_model.SetCurrentProxy(None,
                active_view_model.CLEAR_AND_SELECT)

    def get_view(self):
        "Returns the active view."
        return servermanager._getPyProxy(
            self.__get_selection_model("ActiveView").GetCurrentProxy())

    def set_source(self, source):
        "Sets the active source."
        active_sources_model = self.__get_selection_model("ActiveSources")
        if source:
            # 3 == CLEAR_AND_SELECT
            active_sources_model = self.__get_selection_model("ActiveSources", source.GetSession())
            active_sources_model.SetCurrentProxy(source.SMProxy,
                active_sources_model.CLEAR_AND_SELECT)
        else:
            active_sources_model = self.__get_selection_model("ActiveSources")
            active_sources_model.SetCurrentProxy(None,
                active_sources_model.CLEAR_AND_SELECT)

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

# -----------------------------------------------------------------------------

class _funcs_internals:
    "Internal class."
    first_render = True

#==============================================================================
# Start the session and initialize the ServerManager
#==============================================================================

active_session_observer = _active_session_observer()

if not servermanager.ActiveConnection:
    Connect()
else:
    _add_functions(globals())

active_objects = _active_objects()
