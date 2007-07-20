#==============================================================================
#
#  Program:   ParaView
#  Module:    __init__.py
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
r"""paraview.py is a Python module for using paraview server manager in Python.
One can always use the server manager directly. However, this module
provides server utilty methods that assist in creating connections, proxies, 
as well as introspection.

A simple example:
  import paraview
  # Creates a new built-in connection and makes it the active connection.
  paraview.ActiveConnection = paraview.Connect()

  # Creates a new render view on the active connection.
  renModule = paraview.CreateRenderView()

  # Create a new sphere proxy on the active connection.
  sphere = paraview.CreateProxy("sources", "SphereSource")

  # Create a representation for the sphere proxy and adds it to the render module.
  display = paraview.CreateRepresentation(sphere, renModule)

  renModule.ResetCamera()
  renModule.StillRender()
"""
import re
import os
if os.name == "posix":
    from libvtkPVServerCommonPython import *
    from libvtkPVServerManagerPython import *
    from libvtkCommonPython import *
else:
    from vtkPVServerCommonPython import *
    from vtkPVServerManagerPython import *
    from vtkCommonPython import *

class pyProxy(object):
    """
    Proxy wrapper. Makes it easier to set properties on a proxy.
    Instead of:
     proxy.GetProperty("Foo").SetElement(0, 1)
     proxy.GetProperty("Foo").SetElement(0, 2)
    you can do:
     proxy.SetFoo(1,2)
    Instead of:
      proxy.GetPropery("Foo").GetElements()
    you can do:
      proxy.GetFoo()
      it returns a list of all values of the property.
    For proxy properties, you can use AddTo or RemoveFrom so instead of:
     proxy.GetProperty("Bar").AddProxy(foo)
    you can do:
     proxy.AddToBar(foo)
    Likewise, instead of 
     proxy.GetProperty("Bar").RemoveProxy(foo)
    you can do:
     proxy.RemoveFromBar(foo)

    This class also adds a few new methods:

    All other methods are passed through to the SMProxy.
    * ListMethods - can be used to obtain a list of property names
        that are supported by this proxy.

    This class also provides an iterator which can be used to iterate
    over all properties.
    eg:
      proxy = pyProxy(smproxy)
      for property in proxy:
          print property
    """
    def __eq__(self, other):
        if isinstance(other, pyProxy):
            return self.SMProxy == other.SMProxy
        return self.SMProxy == other

    def __ne__(self, other):
        return not self.__eq__(other)

    def __init__(self, proxy):
        "Constructor. Assigns proxy to self.SMProxy"
        self.SMProxy = proxy
        doc = proxy.GetDocumentation().GetDescription()
        if doc:
            self.__doc__ = doc

    def __iter__(self):
        return pyPropertyIterator(self)

    def __AddToProperty(self, *args):
        """Generic method for adding a proxy to a proxy property.
        Should not be directly called"""
        if not self.__LastAttrName:
            self.__LastAttrName = None
            raise "Cannot find property name"
            return
        property = self.SMProxy.GetProperty(self.__LastAttrName)
        if not property:
            self.__LastAttrName = None
            print "Property %s not found. Cannot set" % self.__LastAttrName
            return
        if property.IsA("vtkSMProxyProperty"):
            for proxy in args:
                if isinstance(proxy, pyProxy):
                    property.AddProxy(proxy.SMProxy)
                else:
                    property.AddProxy(proxy)
        else:
            raise "AddTo works only with proxy properties"
        self.__LastAttrName = None
        return

    def __RemoveFromProperty(self, *args):
        """Generic method for removing a proxy from a proxy property.
        Should not be directly called."""
        if not self.__LastAttrName:
            self.__LastAttrName = None
            raise "Cannot find property name"
            return
        property = self.SMProxy.GetProperty(self.__LastAttrName)
        if not property:
            self.__LastAttrName = None
            print "Property %s not found. Cannot set" % self.__LastAttrName
            return
        if property.IsA("vtkSMProxyProperty"):
            for proxy in args:
                if isinstance(proxy, pyProxy):
                    property.RemoveProxy(proxy.SMProxy)
                else:
                    property.RemoveProxy(proxy)
        else:
            raise "RemoveFrom works only with proxy properties"
        self.__LastAttrName = None
        return


    def __SetProperty(self, *args):
        """Generic method for setting the value of a property.
        Should not be directly called"""
        if not self.__LastAttrName:
            self.__LastAttrName = None
            raise "Cannot find property name"
            return
        property = self.SMProxy.GetProperty(self.__LastAttrName)
        if not property:
            self.__LastAttrName = None
            print "Property %s not found. Cannot set" % self.__LastAttrName
            return
        if property.IsA("vtkSMProxyProperty"):
            for i in range(len(args)):
                if isinstance(args[i], pyProxy):
                    value_proxy = args[i].SMProxy
                else:
                    value_proxy = args[i]
                if i < property.GetNumberOfProxies() :
                    property.SetProxy(i, value_proxy)
                else:
                    property.AddProxy(value_proxy)
        else:
            for i in range(len(args)):
                property.SetElement(i, args[i])
        self.__LastAttrName = None

    def __GetProperty(self):
        """Generic method for getting the value of a property.
           Should not be directly called."""
        if not self.__LastAttrName:
            raise "Cannot find property name."
            return
        property = self.SMProxy.GetProperty(self.__LastAttrName)
        if not property:
            self.__LastAttrName = None
            print "Property %s not found. Cannot set" % self.__LastAttrName
            return
        self.__LastAttrName = None
        if property.IsA("vtkSMProxyProperty"):
            list = []
            for i in range(0, property.GetNumberOfProxies()):
                proxy = property.GetProxy(i)
                if proxy:
                    list.append(pyProxy(proxy))
                else:
                    list.append(None)
            return list
        else:
            list = []
            for i in range(0, property.GetNumberOfElements()):
                list.append(property.GetElement(i))
            return list 
        return []
      
    def __AddProxy(self, name, proxy):
        "Overload CompoundProxy's AddProxy()"
        if isinstance(proxy, pyProxy):
            self.SMProxy.AddProxy(name, proxy.SMProxy)
        else:
            self.SMProxy.AddProxy(name, proxy)
        return

    def __SaveDefinition(self, root=None):
        "Overload for CompoundProxy's SaveDefinition."
        defn = self.SMProxy.SaveDefinition(root)
        if defn:
            defn.UnRegister(None)
        return defn

    def ListProperties(self):
        """Returns a list of all properties on this proxy."""
        property_list = []
        iter = self.__iter__()
        for property in iter:
            property_list.append(iter.GetKey())
        return property_list

    def __ConvertArgumentsAndCall(self, *args):
      newArgs = []
      for arg in args:
          if type(arg) is type(self):
              newArgs.append(arg.SMProxy)
          else:
              newArgs.append(arg)
      func = getattr(self.SMProxy, self.__LastAttrName)
      retVal = func(*newArgs)
      if type(retVal) is type(self.SMProxy) and retVal.IsA("vtkSMProxy"):
          return pyProxy(retVal)
      else:
          return retVal

    def __getattr__(self, name):
        """With the exception of a few overloaded methods,
        returns the SMProxy method"""
        if not self.SMProxy:
            return getattr(self, name)
        # First check if this is a property
        if re.compile("^Set").match(name) and self.SMProxy.GetProperty(name[3:]):
            self.__LastAttrName = name[3:]
            return self.__SetProperty
        if re.compile("^Get").match(name) and self.SMProxy.GetProperty(name[3:]):
            self.__LastAttrName = name[3:]
            return self.__GetProperty
        if re.compile("^AddTo").match(name) and self.SMProxy.GetProperty(name[5:]):
            self.__LastAttrName = name[5:]
            return self.__AddToProperty
        if re.compile("^RemoveFrom").match(name) and self.SMProxy.GetProperty(name[10:]):
            self.__LastAttrName = name[10:]
            return self.__RemoveFromProperty
        if name == "SaveDefinition" and hasattr(self.SMProxy, "SaveDefinition"):
            return self.__SaveDefinition
        # If not a property, see if SMProxy has the method
        try:
            proxyAttr = getattr(self.SMProxy, name)
            self.__LastAttrName = name
            return self.__ConvertArgumentsAndCall
        except:
            pass
        return getattr(self.SMProxy, name)
        
class pyProxyManager(object):
    "Proxy manager wrapper"

    def __init__(self):
        """Constructor. Assigned self.SMProxyManager to
        vtkSMObject.GetPropertyManager(). Make sure to initialize
        server manager before creating a pyProxyManager"""
        self.SMProxyManager = vtkSMObject.GetProxyManager()

    def RegisterProxy(self, group, name, proxy):
        """Registers a proxy (either SMProxy or pyProxy) with the
        server manager"""
        if isinstance(proxy, pyProxy):
            self.SMProxyManager.RegisterProxy(group, name, proxy.SMProxy)
        else:
            self.SMProxyManager.RegisterProxy(group, name, proxy)

    def NewProxy(self, group, name):
        """Creates a new proxy of given group and name and returns a pyProxy
        wrapper"""
        if not self.SMProxyManager:
            return None
        proxy = self.SMProxyManager.NewProxy(group, name)
        if not proxy:
            return None
        proxy.UnRegister(None)
        return pyProxy(proxy)

    def NewCompoundProxy(self, name):
        """Create a new compound proxy with the given name and returns q pyProxy
        wrapper"""
        if not self.SMProxyManager:
            return None
        proxy = self.SMProxyManager.NewCompoundProxy(name)
        if not proxy:
            return None
        proxy.UnRegister(None)
        return pyProxy(proxy)

    def GetProxy(self, group, name):
        """Returns a pyProxy wrapper for a proxy"""
        if not self.SMProxyManager:
            return None
        proxy = self.SMProxyManager.GetProxy(group, name)
        if not proxy:
            return None
        return pyProxy(proxy)

    def GetPrototypeProxy(self, group, name):
        """Returns a pyProxy wrapper for a proxy"""
        if not self.SMProxyManager:
            return None
        proxy = self.SMProxyManager.GetPrototypeProxy(group, name)
        if not proxy:
            return None
        return pyProxy(proxy)

    def GetProxiesOnConnection(self, connection):
        """Returns a map of proxies registered with the proxy manager
           on the particular connection."""
        proxy_groups = {}
        iter = self.connection_iter(connection) 
        for proxy in iter:
            if not proxy_groups.has_key(iter.GetGroup()):
                proxy_groups[iter.GetGroup()] = {}
            group = proxy_groups[iter.GetGroup()]
            group[iter.GetKey()] = proxy;
        return proxy_groups

    def GetProxiesInGroup(self, groupname, connection=None):
        """Returns a map of proxies in a particular group. 
         If connection is not None, then only those proxies
         in the group that are on the particular connection
         are returned.
        """
        proxies = {}
        iter = self.group_iter(groupname) 
        for proxy in iter:
            proxies[iter.GetKey()] = proxy;
        return proxies

    def UnRegisterProxy(self, groupname, proxyname, proxy):
        """Unregisters a proxy."""
        if not self.SMProxyManager:
            return 
        if proxy != None and isinstance(proxy,pyProxy):
            proxy = proxy.SMProxy
        if not proxy:
            self.SMProxyManager.UnRegisterProxy(groupname, proxyname, proxy)

    def GetProxies(self, groupname, proxyname):
        """Returns all proxies registered under the given group with the given name."""
        if not self.SMProxyManager:
            return []
        collection = vtkCollection()
        result = []
        self.SMProxyManager.GetProxies(groupname, proxyname, collection)
        for i in range(0, collection.GetNumberOfItems()):
            proxy = collection.GetItemAsObject(i)
            if proxy:
                proxy = pyProxy(proxy)
            result.append(proxy)
        return result
        
    def __getattr__(self, name):
        """Returns attribute from the SMProxyManager"""
        return getattr(self.SMProxyManager, name)

    def __iter__(self):
        return pyProxyIterator()

    def group_iter(self, group_name, connection=None):
        iter = self.__iter__()
        if connection:
            iter.SetConnectionID(connection.ID)
        iter.SetModeToOneGroup()
        iter.Begin(group_name)
        return iter

    def connection_iter(self, connection):
        iter = self.__iter__()
        if connection:
            iter.SetConnectionID(connection.ID)
        iter.Begin()
        return iter

    def definition_iter(self, groupname=None):
        """Returns an iterator that can be used to iterate over
           all groups and types of proxies that the proxy manager
           can create."""
        iter = pyProxyDefinitionIterator()
        if groupname != None:
            iter.SetModeToOneGroup()
            iter.Begin(groupname)
        return iter

    def ListProperties(self, groupname, proxyname):
        """Returns a list of all property names for a
           proxy of the given type."""
        proxy = self.GetPrototypeProxy(groupname, proxyname)
        if proxy:
            return proxy.ListProperties()
        

class pyPropertyIterator(object):
    """Wrapper for a vtkSMPropertyIterator class to satisfy
       the python iterator protocol."""
    def __init__(self, proxy):
        self.SMIterator = proxy.NewPropertyIterator()
	if self.SMIterator:
            self.SMIterator.UnRegister(None)
            self.SMIterator.Begin()
        self.Key = None
        self.Property = None
        self.Proxy = None

    def __iter__(self):
        return self

    def next(self):
	if not self.SMIterator:
            raise StopIteration

        if self.SMIterator.IsAtEnd():
            self.Key = None
            self.Property = None
            self.Proxy = None
            raise StopIteration
        self.Proxy = self.SMIterator.GetProxy()
        self.Key = self.SMIterator.GetKey()
        self.Property = self.SMIterator.GetProperty()
        self.SMIterator.Next()
        return self.Property

    def GetProxy(self):
        """Returns the proxy for the property last returned by the call to
        'next()'"""
        return self.Proxy

    def GetKey(self):
        """Returns the key for the property last returned by the call to
        'next()' """
        return self.Key

    def GetProperty(self):
        """Returns the property last returned by the call to 'next()' """
        return self.Property

    def __getattr__(self, name):
        """returns attributes from the vtkSMProxyIterator."""
        return getattr(self.SMIterator, name)

class pyProxyDefinitionIterator(object):
    """Wrapper for a vtkSMProxyDefinitionIterator class to satisfy
       the python iterator protocol."""
    def __init__(self):
        self.SMIterator = vtkSMProxyDefinitionIterator()
        self.Group = None
        self.Key = None

    def __iter__(self):
        return self

    def next(self):
        if self.SMIterator.IsAtEnd():
            self.Group = None
            self.Key = None
            raise StopIteration
        self.Group = self.SMIterator.GetGroup()
        self.Key = self.SMIterator.GetKey()
        self.SMIterator.Next()
        return {"group": self.Group, "key":self.Key }

    def GetKey(self):
        """Returns the key for the proxy definition last returned by the call
        to 'next()' """
        return self.Key

    def GetGroup(self):
        """Returns the group for the proxy definition last returned by the
        call to 'next()' """
        return self.Group

    def __getattr__(self, name):
        """returns attributes from the vtkSMProxyDefinitionIterator."""
        return getattr(self.SMIterator, name)


class pyProxyIterator(object):
    """Wrapper for a vtkSMProxyIterator class to satisfy the
     python iterator protocol."""
    def __init__(self):
        self.SMIterator = vtkSMProxyIterator()
        self.SMIterator.Begin()
        self.Proxy = None
        self.Group = None
        self.Key = None

    def __iter__(self):
        return self

    def next(self):
        if self.SMIterator.IsAtEnd():
            self.Proxy = None
            self.Group = None
            self.Key = None
            raise StopIteration
            return None
        self.Proxy = self.SMIterator.GetProxy()
        if self.Proxy:
            self.Proxy = pyProxy(self.Proxy)
        self.Group = self.SMIterator.GetGroup()
        self.Key = self.SMIterator.GetKey()
        self.SMIterator.Next()
        return self.Proxy

    def GetProxy(self):
        """Returns the proxy last returned by the call to 'next()'"""
        return self.Proxy

    def GetKey(self):
        """Returns the key for the proxy last returned by the call to
        'next()' """
        return self.Key

    def GetGroup(self):
        """Returns the group for the proxy last returned by the call to
        'next()' """
        return self.Group

    def __getattr__(self, name):
        """returns attributes from the vtkSMProxyIterator."""
        return getattr(self.SMIterator, name)

class pyConnection(object):
    """
      This is a representation for a connection on in the python client.
      Eventually,  this may move to the server manager itself.
    """
    def __init__(self, connectionId):
        self.ID = connectionId
        self.Hostname = ""
        self.Port = 0
        self.RSHostname = None
        self.RSPort = None
        return

    def SetHost(self, ds_host, ds_port, rs_host=None, rs_port=None):
        self.Hostname = ds_host 
        self.Port = ds_port
        self.RSHostname = rs_host
        self.RSPort = rs_port
        return

    def __repr__(self):
        if not self.RSHostname:
            return "Connection (%s:%d)" % (self.Hostname, self.Port)
        return "Connection data(%s:%d), render(%s:%d)" % \
            (self.Hostname, self.Port, self.RSHostname, self.RSPort)

    def IsRemote(self):
        pm = vtkProcessModule.GetProcessModule()
        if pm.IsRemote(self.ID):
            return True
        return False


# Users can set the active connection which will be used by API
# to create proxies etc when no connection argument is passed.
ActiveConnection = None

## These are method to create a new connection.
## One can connect to a server, (data-server,render-server)
## or simply create a built-in connection.
def connect_server(host, port):
    """Connect to a host:port. Returns the connection object if successfully
    connected with the server."""
    pm =  vtkProcessModule.GetProcessModule()
    cid = pm.ConnectToRemote(host, port)
    if not cid:
        return None
    conn = pyConnection(cid)
    conn.SetHost(host, port)
    return conn 

def connect_ds_rs(ds_host, ds_port, rs_host, rs_port):
    """Connect to a dataserver at (ds_host:ds_port) and to a render server
    at (rs_host:rs_port). 
    Returns the connection object if successfully connected 
    with the server."""
    pm =  vtkProcessModule.GetProcessModule()
    cid = pm.ConnectToRemote(ds_host, ds_port, rs_host, rs_port)
    if not cid:
        return None
    conn = pyConnection(cid)
    conn.SetHost(ds_host, ds_port, rs_host, rs_port)
    return conn 

def connect_self():
    """Creates a new self connection."""
    pm =  vtkProcessModule.GetProcessModule()
    cid = pm.ConnectToSelf()
    if not cid:
        return None
    conn = pyConnection(cid)
    conn.SetHost("builtin", cid)
    return conn

def Connect(ds_host=None, ds_port=11111, rs_host=None, rs_port=11111):
    """
    Use this function call to create a new connection. On success,
    it returns a pyConnection object that abstracts the connection.
    There are several ways in which this function can be called:
    * When called with no arguments, it creates a new connection
      to the built-in server on the client itself.
    * When called with ds_host and ds_port arguments, it
      attempts to connect to a server(data and render server on the same server)
      on the indicated host:port.
    * When called with ds_host, ds_port, rs_host, rs_port, it
      creates a new connection to the data server on ds_host:ds_port and to the
      render server on rs_host: rs_port.
    """
    if ds_host == None:
        return connect_self()
    if rs_host == None:
        return connect_server(ds_host, ds_port)
    return connect_ds_rs(ds_host, ds_port, rs_host, rs_port)

def Disconnect(connection):
    """Disconnects the connection."""
    pm =  vtkProcessModule.GetProcessModule()
    pm.Disconnect(connection.ID)
    return

def CreateProxy(xml_group, xml_name, register_group=None, register_name=None, connection=None):
    """Creates a proxy. If register_group is non-None, then the created
       proxy is registered under that group. If connection is set, the proxy's
       connection ID is set accordingly. If connection is None, ActiveConnection
       is used, is present. If register_group is non-None, but register_name is None,
       then the proxy's self id is used to create a new name.
    """
    global ActiveConnection
    pxm = pyProxyManager()
    proxy = pxm.NewProxy(xml_group, xml_name)
    if not proxy:
        return None
    if not connection:
        connection = ActiveConnection
    if connection:
        proxy.SMProxy.SetConnectionID(connection.ID)
    if register_group:
        if not register_name:
            register_name = proxy.GetSelfIDAsString()
        pxm.RegisterProxy(register_group, register_name, proxy.SMProxy)
    return proxy

def RegisterProxy(proxy, name, group = "sources"):
    pyProxyManager().RegisterProxy(group, name, proxy.SMProxy)

def GetRenderView():
    """Return the render view in use.  If more than one render view is in use, return the first one."""
    render_module = None
    for proxy in pyProxyManager().connection_iter(ActiveConnection):
        if proxy.IsA("vtkSMRenderViewProxy"):
            render_module = proxy
            break
    return render_module

def GetRenderViews():
    """Returns the set of all render views."""
    render_modules = []
    for proxy in pyProxyManager().connection_iter(ActiveConnection):
        if proxy.IsA("vtkSMRenderViewProxy"):
            render_modules.append(proxy)
    return render_modules

def CreateRenderView(connection=None):
    """Creates a render window on the particular connection. If connection is not specified,
    then the active connection is used, if available."""
    global ActiveConnection
    if not connection:
        connection = ActiveConnection
    if not connection:
        raise "Cannot create render window without connection."
    pxm = pyProxyManager()
    proxy_xml_name = None
    if connection.IsRemote():
        proxy_xml_name = "IceTDesktopRenderView"
    else:
        proxy_xml_name = "RenderView"
    ren_module = pxm.NewProxy("newviews", proxy_xml_name)
    if not ren_module:
        return None
    pxm.RegisterProxy("render_modules", ren_module.GetSelfIDAsString(), ren_module)
    ren_module.UpdateVTKObjects()
    return ren_module

def CreateRepresentation(proxy, renModule):
    """Creates a representation for the proxy and adds it to the render module."""
    if not proxy:
        raise "proxy argument cannot be None."
    if not renModule:
        raise "render module argument cannot be None."
    display = renModule.CreateDefaultRepresentation(proxy)
    if not display:
        return None
    display.UnRegister(None)
    pxm = pyProxyManager()
    pxm.RegisterProxy("displays", display.GetSelfIDAsString(), display)
    display.SetInput(proxy)
    display.UpdateVTKObjects()
    renModule.AddToRepresentations(display)
    renModule.UpdateVTKObjects()
    return display

def Fetch(input, arg=None):
    """ 
    A convenience method that moves data from the server to the client, 
    optionally performing some operation on the data as it moves.
    The input argument is the name of the (proxy for a) source or filter
    whose output is needed on the client.
    
    You can use Fetch to do three things:

    If arg is None (the default) then all of the data is brought to the client.
    In parallel runs an appropriate append Filter merges the
    data on each processor into one data object. The filter chosen will be 
    vtkAppendPolyData for vtkPolyData, vtkAppendRectilinearGrid for 
    vtkRectilinearGrid, vtkMultiGroupDataGroupFilter for vtkCompositeData, 
    and vtkAppendFilter for anything else.
    
    If arg is an integer then one particular processor's output is brought to
    the client. In serial runs the arg is ignored. If you have a filter that
    computes results in parallel and brings them to the root node, then set 
    arg to be 0.
    
    If arg is an algorithm, for example vtkMinMax, the algorithm will be 
    applied to the data to obtain some result. In parallel runs the algorithm 
    will be run on each processor to make intermediate results and then again 
    on the root processor over all of the intermediate results to create a 
    global result.
    """

    import types

    #create the pipeline that reduces and transmits the data
    gvd = CreateProxy("representations", "ClientDeliveryRepresentationBase")
    gvd.AddInput(input, "DONTCARE") 
  
    if arg == None:
        print "getting appended"

        cdinfo = input.GetDataInformation().GetCompositeDataInformation()
        if (cdinfo.GetDataIsComposite() or cdinfo.GetDataIsHierarchical()):
            print "use composite data append"
            gvd.SetReductionType(5)        

        elif input.GetDataInformation().GetDataClassName() == "vtkPolyData":
            print "use append poly data filter"
            gvd.SetReductionType(1)        

        elif input.GetDataInformation().GetDataClassName() == "vtkRectilinearGrid":
            print "use append rectilinear grid filter"
            gvd.SetReductionType(4)

        elif input.GetDataInformation().IsA("vtkDataSet"):
            print "use unstructured append filter"
            gvd.SetReductionType(2)

        
    elif type(arg) is types.IntType:          
        print "getting node %d" % arg
        gvd.SetReductionType(3)   
        gvd.SetPreGatherHelper(None)
        gvd.SetPostGatherHelper(None)
        gvd.SetPassThrough(arg)

    else:
        print "applying operation"
        gvd.SetReductionType(3)   
        gvd.SetPreGatherHelper(arg)
        gvd.SetPostGatherHelper(arg)
        gvd.SetPassThrough(-1)

    #go!
    gvd.UpdateVTKObjects()
    gvd.Update()   
    return gvd.GetOutput()

def IntegrateCell(dataset, cellId):
    """
    This functions uses vtkCellIntegrator's Integrate method that calculates
    the length/area/volume of a 1D/2D/3D cell. The calculation is exact for
    lines, polylines, triangles, triangle strips, pixels, voxels, convex
    polygons, quads and tetrahedra. All other 3D cells are triangulated
    during volume calculation. In such cases, the result may not be exact.
    """
    
    return vtkCellIntegrator.Integrate(dataset, cellId)
