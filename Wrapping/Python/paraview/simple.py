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

  # Turn the visibility of the shrink object on.
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

from __future__ import absolute_import, division, print_function

import paraview
from paraview import servermanager
import paraview._backwardscompatibilityhelper

# Bring OutputPort in our namespace.
from paraview.servermanager import OutputPort

import sys
import warnings

if sys.version_info >= (3,):
    xrange = range

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

def Connect(ds_host=None, ds_port=11111, rs_host=None, rs_port=11111, timeout = 60):
    """Creates a connection to a server. Example usage::

    > Connect("amber") # Connect to a single server at default port
    > Connect("amber", 12345) # Connect to a single server at port 12345
    > Connect("amber", 11111, "vis_cluster", 11111) # connect to data server, render server pair
    > Connect("amber", timeout=30) # Connect to a single server at default port with a 30s timeout instead of default 60s
    > Connect("amber", timeout=-1) # Connect to a single server at default port with no timeout instead of default 60s
    > Connect("amber", timeout=0)  # Connect to a single server at default port without retrying instead of retrying for the default 60s"""
    Disconnect(globals(), False)
    connection = servermanager.Connect(ds_host, ds_port, rs_host, rs_port, timeout)
    if not (connection is None):
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

# -----------------------------------------------------------------------------

def ResetSession():
    """Reset the session to its initial state."""
    connection = servermanager.ResetSession()
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
def CreateView(view_xml_name, detachedFromLayout=None, **params):
    """Creates and returns the specified proxy view based on its name/label.
    Also set params keywords arguments as view properties.

    `detachedFromLayout` has been deprecated in ParaView 5.7 as it is no longer
    needed. All views are created detached by default.
    """
    if detachedFromLayout is not None:
        warnings.warn("`detachedFromLayout` is deprecated in ParaView 5.7", DeprecationWarning)

    view = servermanager._create_view(view_xml_name)
    if not view:
        raise RuntimeError ("Failed to create requested view", view_xml_name)

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

    if paraview.compatibility.GetVersion() <= 5.6:
        # older versions automatically assigned view to a
        # layout.
        controller.AssignViewToLayout(view)

    # setup an interactor if current process support interaction if an
    # interactor hasn't already been set. This overcomes the problem where VTK
    # segfaults if the interactor is created after the window was created.
    view.MakeRenderWindowInteractor(True)
    return view

# -----------------------------------------------------------------------------

def CreateRenderView(detachedFromLayout=False, **params):
    """"Create standard 3D render view.
    See CreateView for arguments documentation"""
    return CreateView("RenderView", detachedFromLayout, **params)

# -----------------------------------------------------------------------------

def CreateXYPlotView(detachedFromLayout=False, **params):
    """Create XY plot Chart view.
    See CreateView for arguments documentation"""
    return CreateView("XYChartView", detachedFromLayout, **params)

# -----------------------------------------------------------------------------

def CreateXYPointPlotView(detachedFromLayout=False, **params):
    """Create XY plot point Chart view.
    See CreateView for arguments documentation"""
    return CreateView("XYPointChartView", detachedFromLayout, **params)

# -----------------------------------------------------------------------------


def CreateBarChartView(detachedFromLayout=False, **params):
    """"Create Bar Chart view.
    See CreateView for arguments documentation"""
    return CreateView("XYBarChartView", detachedFromLayout, **params)

# -----------------------------------------------------------------------------

def CreateComparativeRenderView(detachedFromLayout=False, **params):
    """"Create Comparative view.
    See CreateView for arguments documentation"""
    return CreateView("ComparativeRenderView", detachedFromLayout, **params)

# -----------------------------------------------------------------------------

def CreateComparativeXYPlotView(detachedFromLayout=False, **params):
    """"Create comparative XY plot Chart view.
    See CreateView for arguments documentation"""
    return CreateView("ComparativeXYPlotView", detachedFromLayout, **params)

# -----------------------------------------------------------------------------

def CreateComparativeBarChartView(detachedFromLayout=False, **params):
    """"Create comparative Bar Chart view.
    See CreateView for arguments documentation"""
    return CreateView("ComparativeBarChartView", detachedFromLayout, **params)

# -----------------------------------------------------------------------------

def CreateParallelCoordinatesChartView(detachedFromLayout=False, **params):
    """"Create Parallele coordinate Chart view.
    See CreateView for arguments documentation"""
    return CreateView("ParallelCoordinatesChartView", detachedFromLayout, **params)

# -----------------------------------------------------------------------------

def Create2DRenderView(detachedFromLayout=False, **params):
    """"Create the standard 3D render view with the 2D interaction mode turned ON.
    See CreateView for arguments documentation"""
    return CreateView("2DRenderView", detachedFromLayout, **params)

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
    if not view:
        raise AttributeError ("view cannot be None")
    # setup an interactor if current process support interaction if an
    # interactor hasn't already been set. This overcomes the problem where VTK
    # segfaults if the interactor is created after the window was created.
    view.MakeRenderWindowInteractor(True)
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
def Interact(view=None):
    """Call this method to start interacting with a view. This method will
    block till the interaction is done. This method will simply return
    if the local process cannot support interactions."""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError ("view argument cannot be None")
    if not view.MakeRenderWindowInteractor(False):
        raise RuntimeError ("Configuration doesn't support interaction.")
    paraview.print_debug_info("Staring interaction. Use 'q' to quit.")

    # Views like ComparativeRenderView require that Render() is called before
    # the Interaction is begun. Hence we call a Render() before start the
    # interactor loop. This also avoids the case where there are pending updates
    # and thus the interaction will be begun on stale datasets.
    Render(view)
    view.GetInteractor().Start()

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

def CreateLayout(name=None):
    """Create a new layout with no active view."""
    layout = servermanager.misc.ViewLayout(registrationGroup="layouts")
    if name:
        RenameLayout(name, layout)
    return layout

# -----------------------------------------------------------------------------

def RemoveLayout(proxy=None):
    """Remove the provided layout, if none is provided,
    remove the layout containing the active view.
    If it is the last layout it will create a new
    one with the same name as the removed one."""
    pxm = servermanager.ProxyManager()
    if not proxy:
        proxy = GetLayout()
    name = pxm.GetProxyName('layouts', proxy)
    pxm.UnRegisterProxy('layouts', name, proxy)
    if len(GetLayouts()) == 0:
      CreateLayout(name)

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
        raise RuntimeError ("No active view was found.")
    lproxy = servermanager.vtkSMViewLayoutProxy.FindLayout(view.SMProxy)
    return servermanager._getPyProxy(lproxy)

def GetLayoutByName(name):
    """Return the first layout with the given name, if any."""
    layouts = GetLayouts()
    for key in layouts.keys():
      if key[0] == name:
        return layouts.get(key)
    return None

def GetViewsInLayout(layout=None):
    """Returns a list of views in the given layout. If not layout is specified,
    the layout for the active view is used, if possible."""
    layout = layout if layout else GetLayout()
    if not layout:
        raise RuntimeError ("Layout couldn't be determined. Please specify a valid layout.")
    views = GetViews()
    return [x for x in views if layout.GetViewLocation(x) != -1]

def AssignViewToLayout(view=None, layout=None, hint=0):
    """Assigns the view provided (or active view if None) to the
    layout provided. If layout is None, then either the active layout or an
    existing layout on the same server will be used. If no layout exists, then
    a new layout will be created. Returns True on success.

    It is an error to assign the same view to multiple layouts.
    """
    view = view if view else GetActiveView()
    if not view:
        raise RuntimeError("No active view was found.")

    layout = layout if layout else GetLayout()
    controller = servermanager.ParaViewPipelineController()
    return controller.AssignViewToLayout(view, layout, hint)

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

def LoadState(filename, connection=None, **extraArgs):
    RemoveViewsAndLayouts()

    pxm = servermanager.ProxyManager()
    proxy = pxm.NewProxy('options', 'LoadStateOptions')

    if ((proxy is not None) & proxy.PrepareToLoad(filename)):
        if (proxy.HasDataFiles() and (extraArgs is not None)):
            pyproxy = servermanager._getPyProxy(proxy)
            SetProperties(pyproxy, **extraArgs)

        proxy.Load()

    # Try to set the new view active
    if len(GetRenderViews()) > 0:
        SetActiveView(GetRenderViews()[0])

# -----------------------------------------------------------------------------

def SaveState(filename):
    servermanager.SaveState(filename)

#==============================================================================
# Representation methods
#==============================================================================

def GetRepresentation(proxy=None, view=None):
    """"Given a pipeline object and view, returns the corresponding representation object.
    If pipeline object and view are not specified, active objects are used."""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError ("view argument cannot be None.")
    if not proxy:
        proxy = active_objects.source
    if not proxy:
        raise ValueError ("proxy argument cannot be None.")
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
    if proxy.GetNumberOfOutputPorts() == 0:
        raise RuntimeError('Cannot show a sink i.e. algorithm with no output.')
    if proxy == None:
        raise RuntimeError ("Show() needs a proxy argument or that an active source is set.")
    if not view:
        # it here's now active view, controller.Show() will create a new preferred view.
        # if possible.
        view = active_objects.view
    controller = servermanager.ParaViewPipelineController()
    rep = controller.Show(proxy, proxy.Port, view)
    if rep == None:
        raise RuntimeError ("Could not create a representation object for proxy %s" % proxy.GetXMLLabel())
    for param in params.keys():
        setattr(rep, param, params[param])
    return rep

# -----------------------------------------------------------------------------
def ShowAll(view=None):
    """Show all pipeline sources in the given view.
    If view is not specified, active view is used."""
    if not view:
        view = active_objects.view
    controller = servermanager.ParaViewPipelineController()
    controller.ShowAll(view)

# -----------------------------------------------------------------------------
def Hide(proxy=None, view=None):
    """Turns the visibility of a given pipeline object off in the given view.
    If pipeline object and/or view are not specified, active objects are used."""
    if not proxy:
      proxy = active_objects.source
    if not view:
        view = active_objects.view
    if not proxy:
        raise ValueError ("proxy argument cannot be None when no active source is present.")
    controller = servermanager.ParaViewPipelineController()
    controller.Hide(proxy, proxy.Port, view)

# -----------------------------------------------------------------------------
def HideAll(view=None):
    """Hide all pipeline sources in the given view.
    If view is not specified, active view is used."""
    if not view:
        view = active_objects.view
    controller = servermanager.ParaViewPipelineController()
    controller.HideAll(view)

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
def ColorBy(rep=None, value=None, separate=False):
    """Set scalar color. This will automatically setup the color maps and others
    necessary state for the representations. 'rep' must be the display
    properties proxy i.e. the value returned by GetDisplayProperties() function.
    If none is provided the display properties for the active source will be
    used, if possible. Set separate to True in order to use a separate color
    map for this representation"""
    rep = rep if rep else GetDisplayProperties()
    if not rep:
        raise ValueError ("No display properties can be determined.")

    rep.UseSeparateColorMap = separate
    association = rep.ColorArrayName.GetAssociation()
    arrayname = rep.ColorArrayName.GetArrayName()
    component = None
    if value == None:
        rep.SetScalarColoring(None, servermanager.GetAssociationFromString(association))
        return
    if not isinstance(value, tuple) and not isinstance(value, list):
        value = (value,)
    if len(value) == 1:
        arrayname = value[0]
    elif len(value) >= 2:
        association = value[0]
        arrayname = value[1]
    if len(value) == 3:
        # component name provided
        componentName = value[2]
        if componentName == "Magnitude":
          component = -1
        else:
          if association == "POINTS":
            array = rep.Input.PointData.GetArray(arrayname)
          if association == "CELLS":
            array = rep.Input.CellData.GetArray(arrayname)
          if array:
            # looking for corresponding component name
            for i in range(0, array.GetNumberOfComponents()):
              if componentName == array.GetComponentName(i):
                component = i
                break
              # none have been found, try to use the name as an int
              if i ==  array.GetNumberOfComponents() - 1:
                try:
                  component = int(componentName)
                except ValueError:
                  pass
    if component is None:
      rep.SetScalarColoring(arrayname, servermanager.GetAssociationFromString(association))
    else:
      rep.SetScalarColoring(arrayname, servermanager.GetAssociationFromString(association), component)
    rep.RescaleTransferFunctionToDataRange()

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
    properties = proxy.ListProperties()
    for param in params.keys():
        pyproxy = servermanager._getPyProxy(proxy)
        pyproxy.__setattr__(param, params[param])

# -----------------------------------------------------------------------------

def GetProperty(*arguments, **keywords):
    """Get one property of the given pipeline object. If keywords are used,
    you can set the proxy and the name of the property that you want to get
    as shown in the following example::

        GetProperty({proxy=sphere, name="Radius"})

    If arguments are used, then you have two cases:

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
        raise RuntimeError ("Expecting at least a property name as input. Otherwise keyword could be used to set 'proxy' and property 'name'")
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

# -----------------------------------------------------------------------------
def LoadPalette(paletteName):
    """Load a color palette to override the default foreground and background
    colors used by ParaView views.  The current global palette's colors are set
    to the colors in the loaded palette."""
    pxm = servermanager.ProxyManager()
    palette = pxm.GetProxy("global_properties", "ColorPalette")
    prototype = pxm.GetPrototypeProxy("palettes", paletteName)

    if palette is None or prototype is None:
        return

    palette.Copy(prototype)
    palette.UpdateVTKObjects()

#==============================================================================
# ServerManager methods
#==============================================================================

def RenameProxy(proxy, group, newName):
    """Renames the given proxy."""
    pxm = servermanager.ProxyManager()
    oldName = pxm.GetProxyName(group, proxy)
    if oldName and newName != oldName:
      pxm.RegisterProxy(group, newName, proxy)
      pxm.UnRegisterProxy(group, oldName, proxy)

def RenameSource(newName, proxy=None):
    """Renames the given source.  If the given proxy is not registered
    in the sources group this method will have no effect.  If no source is
    provided, the active source is used."""
    if not proxy:
        proxy = GetActiveSource()
    RenameProxy(proxy, "sources", newName)

def RenameView(newName, proxy=None):
    """Renames the given view.  If the given proxy is not registered
    in the views group this method will have no effect.  If no view is
    provided, the active view is used."""
    if not proxy:
        proxy = GetActiveView()
    RenameProxy(proxy, "views", newName)

def RenameLayout(newName, proxy=None):
    """Renames the given layout.  If the given proxy is not registered
    in the layout group this method will have no effect.  If no layout is
    provided, the active layout is used."""
    if not proxy:
        proxy = GetLayout()
    RenameProxy(proxy, "layouts", newName)

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
        raise RuntimeError ("Failed to create/locate the specified view")
    return view

def FindViewOrCreate(name, viewtype):
    """
    Returns the view, if a view with the given name exists and is of the
    the given type, otherwise creates a new view of the requested type."""
    view = FindView(name)
    if view is None or view.GetXMLName() != viewtype:
        view = CreateView(viewtype)
    if not view:
        raise RuntimeError ("Failed to create/locate the specified view")
    return view

def LocateView(displayProperties=None):
    """
    Given a displayProperties object i.e. the object returned by
    GetDisplayProperties() or Show() functions, this function will locate a view
    to which the displayProperties object corresponds."""
    if displayProperties is None:
        displayProperties = GetDisplayProperties()
    if displayProperties is None:
        raise ValueError ("'displayProperties' must be set")
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
        raise RuntimeError ("Could not locate proxy to 'Delete'")
    controller = servermanager.ParaViewPipelineController()
    controller.UnRegisterProxy(proxy)

#==============================================================================
# Active Source / View / Camera / AnimationScene
#==============================================================================

def GetActiveView():
    """Returns the active view."""
    return active_objects.view

# -----------------------------------------------------------------------------

def SetActiveView(view):
    """Sets the active view."""
    active_objects.view = view

# -----------------------------------------------------------------------------

def GetActiveSource():
    """Returns the active source."""
    return active_objects.source

# -----------------------------------------------------------------------------

def SetActiveSource(source):
    """Sets the active source."""
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
        raise RuntimeError (msg)
    if not reader_factor.CanReadFile(first_file, session):
        msg = "File not readable. No reader found for '%s' " % first_file
        raise RuntimeError (msg)
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
def ReloadFiles(proxy=None):
    """Forces the `proxy` to reload the data files. If no `proxy` is provided,
    active source is used."""
    if not proxy:
        proxy = GetActiveSource()
    helper = servermanager.vtkSMReaderReloadHelper()
    return helper.ReloadFiles(proxy.SMProxy)

def ExtendFileSeries(proxy=None):
    """For a reader `proxy` that supports reading files series, detect any new files
    added to the series and update the reader's filename property.
    If no `proxy` is provided, active source is used."""
    if not proxy:
        proxy = GetActiveSource()
    helper = servermanager.vtkSMReaderReloadHelper()
    return helper.ExtendFileSeries(proxy.SMProxy)

# -----------------------------------------------------------------------------
def ImportCinema(filename, view=None):
    """Import a cinema database. This can potentially create multiple
    sources/filters for visualizable objects in the Cinema database.
    Returns True on success. If view is provided, then the cinema sources
    are shown in that view as indicated in the database.
    """
    try:
        from paraview.modules.vtkPVCinemaReader import vtkSMCinemaDatabaseImporter
    except ImportError:
        # cinema not supported in current configuration
        return False
    session = servermanager.ActiveConnection.Session
    importer = vtkSMCinemaDatabaseImporter()
    return importer.ImportCinema(filename, session, view)

# -----------------------------------------------------------------------------
def CreateWriter(filename, proxy=None, **extraArgs):
    """Creates a writer that can write the data produced by the source proxy in
       the given file format (identified by the extension). If no source is
       provided, then the active source is used. This doesn't actually write the
       data, it simply creates the writer and returns it."""
    if not filename:
       raise RuntimeError ("filename must be specified")
    session = servermanager.ActiveConnection.Session
    writer_factory = servermanager.vtkSMProxyManager.GetProxyManager().GetWriterFactory()
    if writer_factory.GetNumberOfRegisteredPrototypes() == 0:
        writer_factory.UpdateAvailableWriters()
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError ("Could not locate source to write")
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
        raise RuntimeError ("Could not create writer for specified file or data type")
    writer.UpdateVTKObjects()
    writer.UpdatePipeline()
    del writer

# -----------------------------------------------------------------------------

def WriteImage(filename, view=None, **params):
    """::deprecated:: 4.2
    Use :func:`SaveScreenshot` instead.
    """
    if not view:
        view = active_objects.view
    writer = None
    if 'Writer' in params:
        writer = params['Writer']
    mag = 1
    if 'Magnification' in params:
        mag = int(params['Magnification'])
    if not writer:
        writer = _find_writer(filename)
    view.WriteImage(filename, writer, mag)

# -----------------------------------------------------------------------------
def _SaveScreenshotLegacy(filename,
    view=None, layout=None, magnification=None, quality=None, **params):
    if view is not None and layout is not None:
        raise ValueError ("both view and layout cannot be specified")

    viewOrLayout = view if view else layout
    viewOrLayout = viewOrLayout if viewOrLayout else GetActiveView()
    if not viewOrLayout:
        raise ValueError ("view or layout needs to be specified")
    try:
        magnification = int(magnification) if int(magnification) > 0 else 1
    except TypeError:
        magnification = 1
    try:
        quality = int(quality)
    except TypeError:
        quality = -1

    # convert magnification to image resolution.
    if viewOrLayout.IsA("vtkSMViewProxy"):
        size = viewOrLayout.ViewSize
    else:
        assert(viewOrLayout.IsA("vtkSMViewLayoutProxy"))
        exts = [0] * 4
        viewOrLayout.GetLayoutExtent(exts)
        size = [exts[1]-exts[0]+1, exts[3]-exts[2]+1]

    imageResolution = (size[0]*magnification, size[1]*magnification)

    # convert quality to ImageQuality
    imageQuality = quality

    # now, call the new API
    return SaveScreenshot(filename, viewOrLayout,
            ImageResolution=imageResolution,
            ImageQuality=imageQuality)

def SaveScreenshot(filename, viewOrLayout=None, **params):
    """Save screenshot for a view or layout (collection of views) to an image.

    `SaveScreenshot` is used to save the rendering results to an image.

    **Parameters**

        filename (str)
          Name of the image file to save to. The filename extension is used to
          determine the type of image file generated. Supported extensions are
          `png`, `jpg`, `tif`, `bmp`, and `ppm`.

        viewOrLayout (``proxy``, optional):
          The view or layout to save image from, defaults to None. If None, then
          the active view is used, if available. To save image from a single
          view, this must be set to a view, to save an image from all views in a
          layout, pass the layout.

    **Keyword Parameters (optional)**

        ImageResolution (tuple(int, int))
          A 2-tuple to specify the output image resolution in pixels as
          `(width, height)`. If not specified, the view (or layout) size is
          used.

        FontScaling (str)
          Specify whether to scale fonts proportionally (`"Scale fonts
          proportionally"`) or not (`"Do not scale fonts"`). Defaults to
          `"Scale fonts proportionally"`.

        SeparatorWidth (int)
          When saving multiple views in a layout, specify the width (in
          approximate pixels) for a separator between views in the generated
          image.

        SeparatorColor (tuple(float, float, float))
          Specify the color for separator between views, if applicable.

        OverrideColorPalette (:obj:str, optional)
          Name of the color palette to use, if any. If none specified, current
          color palette remains unchanged.

        StereoMode (str)
          Stereo mode to use, if any. Available values are `"No stereo"`,
          `"Red-Blue"`, `"Interlaced"`, `"Left Eye Only"`, `"Right Eye Only"`,
          `"Dresden"`, `"Anaglyph"`, `"Checkerboard"`,
          `"Side-by-Side Horizontal"`, and the default `"No change"`.

        TransparentBackground (int)
          Set to 1 (or True) to save an image with background set to alpha=0, if
          supported by the output image format.

    In addition, several format-specific keyword parameters can be specified.
    The format is chosen based on the file extension.

    For JPEG (`*.jpg`), the following parameters are available (optional)

        Quality (int) [0, 100]
          Specify the JPEG compression quality. `O` is low quality (maximum compression)
          and `100` is high quality (least compression).

        Progressive (int):
          Set to 1 (or True) to save progressive JPEG.

    For PNG (`*.png`), the following parameters are available (optional)

        CompressionLevel (int) [0, 9]
          Specify the *zlib* compression level. `0` is no compression, while `9` is
          maximum compression.

    **Legacy Parameters**

        Prior to ParaView version 5.4, the following parameters were available
        and are still supported. However, they cannot be used together with
        other keyword parameters documented earlier.

        view (proxy)
          Single view to save image from.

        layout (proxy)
          Layout to save image from.

        magnification (int)
          Magnification factor to use to save the output image. The current view
          (or layout) size is scaled by the magnification factor provided.

        quality (int)
          Output image quality, a number in the range [0, 100].

        ImageQuality (int)
            For ParaView 5.4, the following parameters were available, however
            it is ignored starting with ParaView 5.5. Instead, it is recommended
            to use format-specific quality parameters based on the file format being used.
    """
    # Let's handle backwards compatibility.
    # Previous API for this method took the following arguments:
    # SaveScreenshot(filename, view=None, layout=None, magnification=None, quality=None)
    # If we notice any of the old arguments, call legacy method.
    if "view" in params or "layout" in params or \
            "magnification" in params or \
            "quality" in params:
                # since in previous variant, view was a positional param,
                # we handle that too.
                if "view" in params:
                    view = params.get("view")
                    del params["view"]
                else:
                    view = viewOrLayout
                return _SaveScreenshotLegacy(filename, view=view, **params)

    # use active view if no view or layout is specified.
    viewOrLayout = viewOrLayout if viewOrLayout else GetActiveView()

    if not viewOrLayout:
        raise ValueError("A view or layout must be specified.")

    controller = servermanager.ParaViewPipelineController()
    options = servermanager.misc.SaveScreenshot()
    controller.PreInitializeProxy(options)

    options.Layout = viewOrLayout if viewOrLayout.IsA("vtkSMViewLayoutProxy") else None
    options.View = viewOrLayout if viewOrLayout.IsA("vtkSMViewProxy") else None
    options.SaveAllViews = True if viewOrLayout.IsA("vtkSMViewLayoutProxy") else False

    # this will choose the correct format.
    options.UpdateDefaultsAndVisibilities(filename)

    controller.PostInitializeProxy(options)

    # explicitly process format properties.
    formatProxy = options.Format
    formatProperties = formatProxy.ListProperties()
    for prop in formatProperties:
        if prop in params:
            formatProxy.SetPropertyWithName(prop, params[prop])
            del params[prop]

    if "ImageQuality" in params:
        import warnings
        warnings.warn("'ImageQuality' is deprecated and will be ignored.", DeprecationWarning)
        del params["ImageQuality"]

    SetProperties(options, **params)
    return options.WriteImage(filename)

# -----------------------------------------------------------------------------
def SaveAnimation(filename, viewOrLayout=None, scene=None, **params):
    """Save animation as a movie file or series of images.

    `SaveAnimation` is used to save an animation as a movie file (avi or ogv) or
    a series of images.

    **Parameters**

        filename (str)
          Name of the output file. The extension is used to determine the type
          of the output. Supported extensions are `png`, `jpg`, `tif`, `bmp`,
          and `ppm`. Based on platform (and build) configuration, `avi` and
          `ogv` may be supported as well.

        viewOrLayout (``proxy``, optional)
          The view or layout to save image from, defaults to None. If None, then
          the active view is used, if available. To save image from a single
          view, this must be set to a view, to save an image from all views in a
          layout, pass the layout.

        scene (``proxy``, optional)
          Animation scene to save. If None, then the active scene returned by
          `GetAnimationScene` is used.

    **Keyword Parameters (optional)**

        `SaveAnimation` supports all keyword parameters supported by
        `SaveScreenshot`. In addition, the following parameters are supported:

        FrameRate (int):
          Frame rate in frames per second for the output. This only affects the
          output when generated movies (`avi` or `ogv`), and not when saving the
          animation out as a series of images.

        FrameWindow (tuple(int,int))
          To save a part of the animation, provide the range in frames or
          timesteps index.

    In addition, several format-specific keyword parameters can be specified.
    The format is chosen based on the file extension.

    For Image-based file-formats that save series of images e.g. PNG, JPEG,
    following parameters are available.

        SuffixFormat (string):
          Format string used to convert the frame number to file name suffix.

    FFMPEG avi file format supports following parameters.

        Compression (int)
          Set to 1 or True to enable compression.

        Quality:
          When compression is 1 (or True), this specifies the compression
          quality. `0` is worst quality (smallest file size) and `2` is best
          quality (largest file size).

    VideoForWindows (VFW) avi file format supports following parameters.

        Quality:
          This specifies the compression quality. `0` is worst quality
          (smallest file size) and `2` is best quality (largest file size).

    OGG/Theora file format supports following parameters.

        Quality:
          This specifies the compression quality. `0` is worst quality
          (smallest file size) and `2` is best quality (largest file size).

        UseSubsampling:
          When set to 1 (or True), the video will be encoded using 4:2:0
          subsampling for the color channels.

    **Obsolete Parameters**

        DisconnectAndSave (int):
          This mode is no longer supported as of ParaView 5.5, and will be
          ignored.

        ImageQuality (int)
            For ParaView 5.4, the following parameters were available, however
            it is ignored starting with ParaView 5.5. Instead, it is recommended
            to use format-specific quality parameters based on the file format being used.
    """
    # use active view if no view or layout is specified.
    viewOrLayout = viewOrLayout if viewOrLayout else GetActiveView()

    if not viewOrLayout:
        raise ValueError("A view or layout must be specified.")

    scene = scene if scene else GetAnimationScene()
    if not scene:
        raise RuntimeError("Missing animation scene.")

    if "DisconnectAndSave" in params:
        import warnings
        warnings.warn("'DisconnectAndSave' is deprecated and will be ignored.", DeprecationWarning)
        del params["DisconnectAndSave"]

    controller = servermanager.ParaViewPipelineController()
    options = servermanager.misc.SaveAnimation()
    controller.PreInitializeProxy(options)

    options.AnimationScene = scene
    options.Layout = viewOrLayout if viewOrLayout.IsA("vtkSMViewLayoutProxy") else None
    options.View = viewOrLayout if viewOrLayout.IsA("vtkSMViewProxy") else None
    options.SaveAllViews = True if viewOrLayout.IsA("vtkSMViewLayoutProxy") else False

    # this will choose the correct format.
    options.UpdateDefaultsAndVisibilities(filename)

    controller.PostInitializeProxy(options)

    # explicitly process format properties.
    formatProxy = options.Format
    formatProperties = formatProxy.ListProperties()
    for prop in formatProperties:
        if prop in params:
            # see comment at vtkSMSaveAnimationProxy.cxx:327
            # certain 'prop' (such as FrameRate) are present
            # in both SaveAnimation and formatProxy (FFMPEG with
            # panel_visibility="never"). In this case save it only
            # in SaveAnimation
            if formatProxy.GetProperty(prop).GetPanelVisibility() != "never":
                formatProxy.SetPropertyWithName(prop, params[prop])
                del params[prop]

    if "ImageQuality" in params:
        import warnings
        warnings.warn("'ImageQuality' is deprecated and will be ignored.", DeprecationWarning)
        del params["ImageQuality"]

    SetProperties(options, **params)
    return options.WriteAnimation(filename)

def WriteAnimation(filename, **params):
    """
    ::deprecated:: 5.3
    Use :func:`SaveAnimation` instead.

    This function can still be used to save an animation, but using
    :func: `SaveAnimation` is strongly recommended as it provides more
    flexibility.

    The following parameters are currently supported.

    **Parameters**

        filename (str)
          Name of the output file.

    **Keyword Parameters (optional)**

        Magnification (int):
          Magnification factor for the saved animation.

        Quality (int)
          int in range [0,2].

        FrameRate (int)
          Frame rate.

    The following parameters are no longer supported and are ignored:
    Subsampling, BackgroundColor, FrameRate, StartFileCount, PlaybackTimeWindow
    """
    newparams = {}

    # this method simply tries to provide legacy behavior.
    scene = GetAnimationScene()
    newparams["scene"] = scene

    # previously, scene saved all views and only worked well if there was 1
    # layout, so do that.
    layout = GetLayout()
    newparams["viewOrLayout"] = layout

    if "Magnification" in params:
        magnification = params["Magnification"]
        exts = [0] * 4
        layout.GetLayoutExtent(exts)
        size = [exts[1]-exts[0]+1, exts[3]-exts[2]+1]
        imageResolution = (size[0]*magnification, size[1]*magnification)
        newparams["ImageResolution"] = imageResolution

    if "Quality" in params:
        # convert quality (0=worst, 2=best) to imageQuality (0 = worst, 100 = best)
        quality = int(params["Quality"])
        imageQuality = int(100 * quality/2.0)
        newparams["ImageQuality"] = imageQuality

    if "FrameRate" in params:
        newparams["FrameRate"] = int(params["FrameRate"])
    return SaveAnimation(filename, **newparams)

def WriteAnimationGeometry(filename, view=None):
    """Save the animation geometry from a specific view to a file specified.
    The animation geometry is written out as a PVD file. If no view is
    specified, the active view will be used of possible."""
    view = view if view else GetActiveView()
    if not view:
        raise ValueError ("Please specify the view to use")
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
        raise ValueError ("'view' argument cannot be None with no active is present.")
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.UpdateScalarBars(view.SMProxy, tfmgr.HIDE_UNUSED_SCALAR_BARS)

def HideScalarBarIfNotNeeded(lut, view=None):
    """Hides the given scalar bar if it is not used by any of the displayed data."""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError ("'view' argument cannot be None with no active present.")
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.HideScalarBarIfNotNeeded(lut.SMProxy, view.SMProxy)

def UpdateScalarBars(view=None):
    """Hides all unused scalar bar and shows used scalar bars. A scalar bar is used
    if some data is shown in that view that is coloring using the transfer function
    shown by the scalar bar."""
    if not view:
        view = active_objects.view
    if not view:
        raise ValueError ("'view' argument cannot be None with no active is present.")
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.UpdateScalarBars(view.SMProxy, tfmgr.HIDE_UNUSED_SCALAR_BARS | tfmgr.SHOW_USED_SCALAR_BARS)

def UpdateScalarBarsComponentTitle(ctf, representation=None):
    """Update all scalar bars using the provided lookup table. The representation is used to recover
    the array from which the component title was obtained. If None is provided the representation
    of the active source in the active view is used."""
    if not representation:
        view = active_objects.view
        proxy = active_objects.source
        if not view:
            raise ValueError ("'representation' argument cannot be None with no active view.")
        if not proxy:
            raise ValueError ("'representation' argument cannot be None with no active source.")
        representation = GetRepresentation(view, proxy)
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    return tfmgr.UpdateScalarBarsComponentTitle(ctf.SMProxy, representation.SMProxy)

def GetScalarBar(ctf, view=None):
    """Returns the scalar bar for color transfer function in the given view.
    If view is None, the active view will be used, if possible.
    This will either return an existing scalar bar or create a new one."""
    view = view if view else active_objects.view
    if not view:
        raise ValueError ("'view' argument cannot be None when no active view is present")
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    sb = servermanager._getPyProxy(\
        tfmgr.GetScalarBarRepresentation(ctf.SMProxy, view.SMProxy))
    return sb

# -----------------------------------------------------------------------------
def GetColorTransferFunction(arrayname, representation=None, separate=False, **params):
    """Get the color transfer function used to mapping a data array with the
    given name to colors. Representation is used to modify the array name
    when using a separate color transfer function. separate can be used to recover
    the separate color transfer function even if it is not used currently by the representation.
    This may create a new color transfer function if none exists, or return an existing one"""
    if representation:
      if separate or representation.UseSeparateColorMap:
        arrayname = ("%s%s_%s" % ("Separate_", representation.SMProxy.GetGlobalIDAsString(), arrayname))
    if not servermanager.ActiveConnection:
        raise RuntimeError ("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    lut = servermanager._getPyProxy(\
            tfmgr.GetColorTransferFunction(arrayname, session.GetSessionProxyManager()))
    SetProperties(lut, **params)
    return lut

def GetOpacityTransferFunction(arrayname, representation=None, separate=False, **params):
    """Get the opacity transfer function used to mapping a data array with the
    given name to opacity. Representation is used to modify the array name
    when using a separate opacity transfer function. separate can be used to recover
    the separate opacity transfer function even if it is not used currently by the representation.
    This may create a new opacity transfer function if none exists, or return an existing one"""
    if representation:
      if separate or representation.UseSeparateColorMap:
        arrayname = ("%s%s_%s" % ("Separate_", representation.SMProxy.GetGlobalIDAsString(), arrayname))
    if not servermanager.ActiveConnection:
        raise RuntimeError ("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    otf = servermanager._getPyProxy(\
            tfmgr.GetOpacityTransferFunction(arrayname, session.GetSessionProxyManager()))
    SetProperties(otf, **params)
    return otf

# -----------------------------------------------------------------------------
def ImportPresets(filename):
    """Import presets from a file. The file can be in the legacy color map xml
    format or in the new JSON format. Returns True on success."""
    presets = servermanager.vtkSMTransferFunctionPresets()
    return presets.ImportPresets(filename)

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

# -----------------------------------------------------------------------------
def AssignLookupTable(arrayInfo, lutName, rangeOveride=[]):
    """Assign a lookup table to an array by lookup table name.

    `arrayInfo` is the information object for the array. The array name and its
    range is determined using the info object provided.

    `lutName` is the name for the transfer function preset.

    `rangeOveride` is provided is the range to use instead of the range of the
    array determined using the `arrayInfo`.

    Example usage::

      track = GetAnimationTrack("Center", 0, sphere) or
      arrayInfo = source.PointData["Temperature"]
      AssignLookupTable(arrayInfo, "Cool to Warm")

    """
    presets = servermanager.vtkSMTransferFunctionPresets()
    if not presets.HasPreset(lutName):
        raise RuntimeError("no preset with name `%s` present", lutName)

    lut = GetColorTransferFunction(arrayInfo.Name)
    if not lut.ApplyPreset(lutName):
        return False

    if rangeOveride:
        lut.RescaleTransferFunction(rangeOveride)
    return True

# -----------------------------------------------------------------------------
def GetLookupTableNames():
    """Returns a list containing the currently available transfer function
    presets."""
    presets = servermanager.vtkSMTransferFunctionPresets()
    return [presets.GetPresetName(index) for index in range(presets.GetNumberOfPresets())]

# -----------------------------------------------------------------------------

def LoadLookupTable(fileName):
    """Load transfer function preset from a file.
    Both JSON (new) and XML (legacy) preset formats are supported.
    If the filename ends with a .xml, it's assumed to be a legacy color map XML
    and will be converted to the new format before processing.
    """
    presets = servermanager.vtkSMTransferFunctionPresets()
    return presets.ImportPresets(fileName)

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

def GetTimeKeeper():
    """Returns the time-keeper for the active session. Timekeeper is often used
    to manage time step information known to the ParaView application."""
    if not servermanager.ActiveConnection:
        raise RuntimeError ("Missing active session")
    session = servermanager.ActiveConnection.Session
    controller = servermanager.ParaViewPipelineController()
    return controller.FindTimeKeeper(session)

def GetAnimationScene():
    """Returns the application-wide animation scene. ParaView has only one
    global animation scene. This method provides access to that. Users are
    free to create additional animation scenes directly, but those scenes
    won't be shown in the ParaView GUI."""
    if not servermanager.ActiveConnection:
        raise RuntimeError ("Missing active session")
    session = servermanager.ActiveConnection.Session
    controller = servermanager.ParaViewPipelineController()
    return controller.GetAnimationScene(session)

# -----------------------------------------------------------------------------

def AnimateReader(reader=None, view=None):
    """This is a utility function that, given a reader and a view
    animates over all time steps of the reader."""
    if not reader:
        reader = active_objects.source
    if not view:
        view = active_objects.view

    return servermanager.AnimateReader(reader, view)

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
        raise TypeError ("proxy must be a servermanager.Proxy instance")
    if isinstance(propertyname_or_property, str):
        propertyname = propertyname_or_property
    elif isinstance(propertyname_or_property, servermanager.Property):
        prop = propertyname_or_property
        propertyname = prop.Name
        proxy = prop.Proxy
    else:
        raise TypeError ("propertyname_or_property must be a string or servermanager.Property")

    # To handle the case where the property is actually a "display" property, in
    # which case we are actually animating the "RepresentationAnimationHelper"
    # associated with the source.
    if propertyname in ["Visibility", "Opacity"]:
        proxy = _GetRepresentationAnimationHelper(proxy)
    if not proxy or not proxy.GetProperty(propertyname):
        raise AttributeError ("Failed to locate property %s" % propertyname)

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
    specified, active view will be used. If no existing camera animation track
    is found, a new one will be created."""
    if not view:
        view = GetActiveView()
    if not view:
        raise ValueError ("No view specified")
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
        raise RuntimeError ("Missing animation scene")
    controller = servermanager.ParaViewPipelineController()
    return controller.GetTimeAnimationTrack(scene)

#==============================================================================
# Plugin Management
#==============================================================================

def LoadXML(xmlstring, ns=None):
    """Given a server manager XML as a string, parse and process it.
    If you loaded the simple module with ``from paraview.simple import *``,
    make sure to pass ``globals()`` as the second arguments::

        LoadXML(xmlstring, globals())

    Otherwise, the new functions will not appear in the global namespace."""
    if not ns:
        ns = globals()
    servermanager.LoadXML(xmlstring)
    _add_functions(ns)

# -----------------------------------------------------------------------------

def LoadPlugin(filename, remote=True, ns=None):
    """Loads a ParaView plugin and updates this module with new constructors
    if any. The remote argument (default to ``True``) is to specify whether
    the plugin will be loaded on client (``remote=False``) or on server
    (``remote=True``).
    If you loaded the simple module with ``from paraview.simple import *``,
    make sure to pass ``globals()`` as an argument::

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
        raise RuntimeError ("Cannot load a plugin without a session.")
    plm = servermanager.vtkSMProxyManager.GetProxyManager().GetPluginManager()
    if remote:
        session = servermanager.ActiveConnection.Session
        info = plm.GetRemoteInformation(session)
    else:
        info = plm.GetLocalInformation()
    for cc in range(0, info.GetNumberOfPlugins()):
        if info.GetPluginName(cc) == pluginname:
            return LoadPlugin(info.GetPluginFileName(cc), remote, ns)
    raise RuntimeError ("Plugin '%s' not found" % pluginname)

#==============================================================================
# Custom Filters Management
#==============================================================================
def LoadCustomFilters(filename, ns=None):
    """Loads a custom filter XML file and updates this module with new
    constructors if any. If you loaded the simple module with
    ``from paraview.simple import *``, make sure to pass ``globals()`` as an
    argument.
    """
    servermanager.ProxyManager().SMProxyManager.LoadCustomProxyDefinitions(filename)
    if not ns:
        ns = globals()
    _add_functions(ns)

#==============================================================================
# Selection Management
#==============================================================================
def _select(seltype, query=None, proxy=None):
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError ("No active source was found.")

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
        raise RuntimeError ("No active source was found.")
    proxy.SMProxy.SetSelectionInput(proxy.Port, None, 0)


#==============================================================================
# Dynamic lights.
#==============================================================================

def CreateLight():
    """Makes a new vtkLight, unattached to a view."""
    pxm = servermanager.ProxyManager()
    lightproxy = pxm.NewProxy("additional_lights", "Light")

    controller = servermanager.ParaViewPipelineController()
    controller.SMController.RegisterLightProxy(lightproxy, None)

    return servermanager._getPyProxy(lightproxy)


def AddLight(view=None):
    """Makes a new vtkLight and adds it to the designated or active view."""
    view = view if view else GetActiveView()
    if not view:
        raise ValueError ("No 'view' was provided and no active view was found.")
    if view.IsA("vtkSMRenderViewProxy") is False:
        return

    lightproxy = CreateLight()

    nowlights = [l for l in view.AdditionalLights]
    nowlights.append(lightproxy)
    view.AdditionalLights = nowlights
    # This is not the same as returning lightProxy
    return GetLight(len(view.AdditionalLights) - 1, view)

def RemoveLight(light):
    """Removes an existing vtkLight from its view."""
    if not light:
        raise ValueError ("No 'light' was provided.")
    view = GetViewForLight(light)
    if view:
        if (not view.IsA("vtkSMRenderViewProxy")) or (len(view.AdditionalLights) < 1):
            raise RuntimeError ("View retrieved inconsistent with owning a 'light'.")

        nowlights = [l for l in view.AdditionalLights if l != light]
        view.AdditionalLights = nowlights

    controller = servermanager.ParaViewPipelineController()
    controller.SMController.UnRegisterProxy(light.SMProxy)

def GetLight(number, view=None):
    """Get a handle on a previously added light"""
    if not view:
        view = active_objects.view
    numlights = len(view.AdditionalLights)
    if numlights < 1 or number < 0 or number >= numlights:
        return
    return view.AdditionalLights[number]

def GetViewForLight(proxy):
    """Given a light proxy, find which view it belongs to"""
    # search current views for this light.
    for cc in range(proxy.GetNumberOfConsumers()):
        consumer = proxy.GetConsumerProxy(cc)
        consumer = consumer.GetTrueParentProxy()
        if consumer.IsA("vtkSMRenderViewProxy") and proxy in consumer.AdditionalLights:
            return consumer
    return None

#==============================================================================
# Materials.
#==============================================================================

def GetMaterialLibrary():
    """Returns the material library for the active session. """
    if not servermanager.ActiveConnection:
        raise RuntimeError ("Missing active session")
    session = servermanager.ActiveConnection.Session
    controller = servermanager.ParaViewPipelineController()
    return controller.FindMaterialLibrary(session)

#==============================================================================
# Miscellaneous functions.
#==============================================================================
def Show3DWidgets(proxy=None):
    """If possible in the current environment, this method will
    request the application to show the 3D widget(s) for proxy"""
    proxy = proxy if proxy else GetActiveSource()
    if not proxy:
        raise ValueError ("No 'proxy' was provided and no active source was found.")
    _Invoke3DWidgetUserEvent(proxy, "ShowWidget")

def Hide3DWidgets(proxy=None):
    """If possible in the current environment, this method will
    request the application to hide the 3D widget(s) for proxy"""
    proxy = proxy if proxy else GetActiveSource()
    if not proxy:
        raise ValueError ("No 'proxy' was provided and no active source was found.")
    _Invoke3DWidgetUserEvent(proxy, "HideWidget")

def _Invoke3DWidgetUserEvent(proxy, event):
    """Internal method used by Show3DWidgets/Hide3DWidgets"""
    if proxy:
        proxy.InvokeEvent('UserEvent', event)
        # Since in 5.0 and earlier, Show3DWidgets/Hide3DWidgets was called with the
        # proxy being the filter proxy (eg. Clip) and not the proxy that has the
        # widget i.e. (Clip.ClipType), we explicitly handle it by iterating of
        # proxy list properties and then invoking the event on their value proxies
        # too.
        for smproperty in proxy:
            if smproperty.FindDomain("vtkSMProxyListDomain"):
                _Invoke3DWidgetUserEvent(smproperty.GetData(), event)

def ExportView(filename, view=None, **params):
    """Export a view to the specified output file."""
    view = view if view else GetActiveView()
    if not view:
        raise ValueError ("No 'view' was provided and no active view was found.")
    if not filename:
        raise ValueError ("No filename specified")

    # ensure that the view is up-to-date.
    view.StillRender()
    helper = servermanager.vtkSMViewExportHelper()
    proxy = helper.CreateExporter(filename, view.SMProxy)
    if not proxy:
        raise RuntimeError ("Failed to create exporter for ", filename)
    proxy.UnRegister(None)
    proxy = servermanager._getPyProxy(proxy)
    SetProperties(proxy, **params)
    proxy.Write()
    del proxy
    del helper

def ResetProperty(propertyName, proxy=None, restoreFromSettings=True):
    if proxy == None:
        proxy = GetActiveSource()

    propertyToReset = proxy.SMProxy.GetProperty(propertyName)

    if propertyToReset != None:
        propertyToReset.ResetToDefault()

        if restoreFromSettings:
            settings = paraview.servermanager.vtkSMSettings.GetInstance()
            settings.GetPropertySetting(propertyToReset)

        proxy.SMProxy.UpdateVTKObjects()

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
    print (avail)
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
    print ('Number of point arrays:', len(pdi))
    for i in range(len(pdi)):
        ai = pdi[i]
        print ("----------------")
        print ("Array:", i, " ", ai.Name, ":")
        numComps = ai.GetNumberOfComponents()
        print ("Number of components:", numComps)
        for j in range(numComps):
            print ("Range:", ai.GetRange(j))
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
      raise RuntimeError ("'connection' cannot be empty.")
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeSession(connection.Session)

def _create_func(key, module, skipRegisteration=False):
    "Internal function."

    def CreateObject(*input, **params):
        """This function creates a new proxy. For pipeline objects that accept inputs,
        all non-keyword arguments are assumed to be inputs. All keyword arguments are
        assumed to be property,value pairs and are passed to the new proxy."""

        # Create a controller instance.
        controller = servermanager.ParaViewPipelineController()

        # Instantiate the actual object from the given module.
        px = paraview._backwardscompatibilityhelper.GetProxy(module, key)

        # preinitialize the proxy.
        controller.PreInitializeProxy(px)

        # Make sure non-keyword arguments are valid
        for inp in input:
            if inp != None and not isinstance(inp, servermanager.Proxy):
                if px.GetProperty("Input") != None:
                    raise RuntimeError ("Expecting a proxy as input.")
                else:
                    raise RuntimeError ("This function does not accept non-keyword arguments.")

        # Assign inputs
        inputName = servermanager.vtkSMCoreUtilities.GetInputPropertyName(px.SMProxy, 0)

        if px.GetProperty(inputName) != None:
            if len(input) > 0:
                px.SetPropertyWithName(inputName, input)
            else:
                # If no input is specified, try the active pipeline object
                if px.GetProperty(inputName).GetRepeatable() and active_objects.get_selected_sources():
                    px.SetPropertyWithName(inputName, active_objects.get_selected_sources())
                elif active_objects.source:
                    px.SetPropertyWithName(inputName, active_objects.source)
        else:
            if len(input) > 0:
                raise RuntimeError ("This function does not expect an input.")

        registrationName = None
        for nameParam in ['registrationName', 'guiName']:
          if nameParam in params:
              registrationName = params[nameParam]
              del params[nameParam]

        # Pass all the named arguments as property,value pairs
        SetProperties(px, **params)

        # post initialize
        controller.PostInitializeProxy(px)

        if not skipRegisteration:
            # Register the proxy with the proxy manager (assuming we are only using
            # these functions for pipeline proxies or animation proxies.
            if isinstance(px, servermanager.SourceProxy):
                controller.RegisterPipelineProxy(px, registrationName)
            elif px.GetXMLGroup() == "animation":
               controller.RegisterAnimationProxy(px)
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
    res += ' '.join(ts)
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
def _get_proxymodules_to_import(connection):
    """
    used in _add_functions, _get_generated_proxies, and _remove_functions to get
    modules to import proxies from.
    """
    if connection and connection.Modules:
        modules = connection.Modules
        return [modules.filters, modules.sources, modules.writers, modules.animation]
    else:
        return []

def _add_functions(g):
    if not servermanager.ActiveConnection:
        return

    activeModule = servermanager.ActiveConnection.Modules
    for m in _get_proxymodules_to_import(servermanager.ActiveConnection):
        # Skip registering proxies in certain modules (currently only writers)
        skipRegisteration = m is activeModule.writers
        dt = m.__dict__
        for key in dt.keys():
            cl = dt[key]
            if not isinstance(cl, str):
                if not key in g and _func_name_valid(key):
                    #print "add %s function" % key
                    g[key] = _create_func(key, m, skipRegisteration)
                    exec ("g[key].__doc__ = _create_doc(m.%s.__doc__, g[key].__doc__)" % key)

# -----------------------------------------------------------------------------

def _get_generated_proxies():
    proxies = []
    for m in _get_proxymodules_to_import(servermanager.ActiveConnection):
        dt = m.__dict__
        for key in dt.keys():
            cl = dt[key]
            if not isinstance(cl, str):
                if _func_name_valid(key):
                    proxies.append(key)
    return proxies
# -----------------------------------------------------------------------------

def _remove_functions(g):
    for m in _get_proxymodules_to_import(servermanager.ActiveConnection):
        dt = m.__dict__
        for key in dt.keys():
            cl = dt[key]
            if not isinstance(cl, str) and key in g:
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
        raise RuntimeError ("Filename has no extension, please specify a write")

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
        raise RuntimeError ("Cannot infer filetype from extension:", extension)

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
            if servermanager.vtkSMProxyManager:
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
            raise RuntimeError ("Try to set an active object with invalid active connection.")
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

if not paraview.options.satelite:
    active_session_observer = _active_session_observer()

    if not servermanager.ActiveConnection:
        Connect()
    else:
        _add_functions(globals())

    active_objects = _active_objects()
