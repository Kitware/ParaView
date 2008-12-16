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

def Connect(ds_host=None, ds_port=11111, rs_host=None, rs_port=11111):
    """Creates a connection to a server. Example usage:
    > Connect("amber") # Connect to a single server at default port
    > Connect("amber", 12345) # Connect to a single server at port 12345
    > Connect("amber", 11111, "vis_cluster", 11111) # connect to data server, render server pair
    This also create a default render view."""
    servermanager.ProxyManager().UnRegisterProxies()
    active_objects.view = None
    active_objects.source = None
    import gc
    gc.collect()
    servermanager.Disconnect()
    cid = servermanager.Connect(ds_host, ds_port, rs_host, rs_port)
    CreateRenderView()
    return cid

def CreateRenderView():

    active_objects.view = servermanager.CreateRenderView()
    servermanager.ProxyManager().RegisterProxy("views", \
      "my_view%d" % _funcs_internals.view_counter, active_objects.view)
    _funcs_internals.view_counter += 1
    return active_objects.view

def GetRenderView():
    return servermanager.GetRenderView()

def GetRenderViews():
    return servermanager.GetRenderViews()

def GetRepresentation(proxy=None, view=None):
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
    return GetRepresentation(proxy, view)
    
def Show(proxy=None, view=None, **params):
    rep = GetDisplayProperties(proxy, view)
    for param in params.keys():
        setattr(rep, param, params[param])
    rep.Visibility = 1
    return rep

def Hide(proxy=None, view=None):
    rep = GetDisplayProperties(proxy, view)
    rep.Visibility = 0

def Render(view=None):
    if not view:
        view = active_objects.view
    view.StillRender()
    if _funcs_internals.first_render:
        view.ResetCamera()
        view.StillRender()
        _funcs_internals.first_render = False
    return view
        
def ResetCamera(view=None):
    if not view:
        view = active_objects.view
    view.ResetCamera()
    Render(view)

def SetProperties(proxy=None, **params):
    if not proxy:
        proxy = active_objects.source
    for param in params.keys():
        if not hasattr(proxy, param):
            raise AttributeError("object has no property %s" % param)
        setattr(proxy, param, params[param])

def SetDisplayProperties(proxy=None, view=None, **params):
    rep = GetDisplayProperties(proxy, view)
    SetProperties(rep, **params)

def SetViewProperties(view=None, **params):
    if not view:
        view = active_objects.view
    SetProperties(view, **params)

def FindSource(name):
    return servermanager.ProxyManager().GetProxy("sources", name)

def GetSources():
    return servermanager.ProxyManager().GetProxiesInGroup("sources")

def GetRepresentations():
    return servermanager.ProxyManager().GetProxiesInGroup("representations")

def UpdatePipeline(time=None, proxy=None):
    if not proxy:
        proxy = active_objects.source
    proxy.UpdatePipeline(time)

# TODO: Change this to take the array name and number of components. Register 
# the lt under the name ncomp.array_name
def MakeBlueToRedLT(min, max):
    lt = servermanager.rendering.PVLookupTable()
    servermanager.Register(lt)
    # Add to RGB points. These are tuples of 4 values. First one is
    # the scalar values, the other 3 the RGB values. 
    lt.RGBPoints = [min, 0, 0, 1, max, 1, 0, 0]
    lt.ColorSpace = "HSV"
    return lt
    
def _find_writer(filename):
    extension = None
    parts = filename.rsplit('.')
    if len(parts) > 1:
        extension = parts[-1]
    else:
        raise RuntimeError, "Filename has no extension, please specify a write"
        
    if extension == 'png':
        return 'vtkPNGWriter'
    elif extension == 'bmp':
        return 'vtkBMPWriter'
    elif extension == 'ppm':
        return vtkPNMWriter
    elif extension == 'tif' or extension == 'tiff':
        return 'vtkTIFFWriter'
    elif extension == 'jpg' or extension == 'jpeg':
        return 'vtkJPEGWriter'
    else:
        raise RuntimeError, "Cannot infer filetype from extension:", extension
    
def WriteImage(filename, view=None, **params):
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
    
def _create_func(key, module):
    def myfunc(*input, **params):
        px = module.__dict__[key]()
        if len(input) == 1:
            px.Input = input[0]
        else:
            px.Input = active_objects.source
        for param in params.keys():
            setattr(px, param, params[param])
        servermanager.Register(px)
        active_objects.source = px
        return px
    return myfunc

def _func_name_valid(name):
    valid = True
    for c in key:
        if c == '(' or c ==')':
            valid = False
            break
    return valid
    
for m in [servermanager.filters, servermanager.sources]:
    dt = m.__dict__
    for key in dt.keys():
        cl = dt[key]
        if not isinstance(cl, str):
            if _func_name_valid(key):
                exec "%s = _create_func(key, m)" % key
del dt, m

def GetActiveView():
    return active_objects.view
    
def GetActiveSource():
    return active_objects.source
    
def GetActiveCamera():
    return GetActiveView().GetActiveCamera()
    
class active_objects:
    view = None
    source = None

class _funcs_internals:
    first_render = True
    view_counter = 0
    rep_counter = 0

FIELD_ASSOCIATION_POINTS = 0
FIELD_ASSOCIATION_CELLS = 1
FIELD_ASSOCIATION_VERTICES = 4
FIELD_ASSOCIATION_EDGES = 5
FIELD_ASSOCIATION_ROWS = 6

if not servermanager.ActiveConnection:
    Connect()

def demo1():
    ss = Sphere(Radius=2, ThetaResolution=32)
    shr = Shrink(Input=ss)
    cs = Cone()
    app = AppendDatasets()
    app.Input = [shr, cs]
    Show(app)
    Render()

def demo2(fname="/Users/berk/Work/ParaView/ParaViewData/Data/disk_out_ref.ex2"):
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
            print "Range:", ai.Range(j)
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

