import re
import os
if os.name == "posix":
  from libvtkPVServerCommonPython import *
  from libvtkPVServerManagerPython import *
else:
  from vtkPVServerCommonPython import *
  from vtkPVServerManagerPython import *

class pyProxy:
    """
    Proxy wrapper. Makes it easier to set properties on a proxy.
    Instead of:
     proxy.GetProperty("Foo").SetElement(0, 1)
     proxy.GetProperty("Foo").SetElement(0, 2)
    you can do:
     proxy.SetFoo(1,2)
    For proxy properties, you can use AddTo so instead of:
     proxy.GetProoperty("Bar").AddProxy(foo)
    you can do:
     proxy.AddToBar(foo)
    All other methods are passed through to the SMProxy.
    """
    def __init__(self, proxy):
        "Constructor. Assigns proxy to self.SMProxy"
        self.SMProxy = proxy
        doc = proxy.GetDocumentation().GetDescription()
        if doc:
          self.__doc__ = doc

    def __iter__(self):
        return pyPropertyIterator(self)

    def __AddToProperty__(self, *args):
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

    def __SetProperty__(self, *args):
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

    def __GetProperty__(self):
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
      
    def __CreateDisplayProxy__(self):
        "Overload RenderModule's CreateDisplayProxy() to return a pyProxy"
        return pyProxy(self.SMProxy.CreateDisplayProxy())

    def __AddProxy__(self, name, proxy):
        "Overload CompoundProxy's AddProxy()"
        if isinstance(proxy, pyProxy):
            self.SMProxy.AddProxy(name, proxy.SMProxy)
        else:
            self.SMProxy.AddProxy(name, proxy)
        return

    def ListProperties(self):
        """Returns a list of all properties on this proxy."""
        property_list = []
        iter = self.__iter__()
        for property in iter:
          property_list.append(iter.GetKey())
        return property_list

    def __getattr__(self, name):
        """With the exception of a few overloaded methods,
        returns the SMProxy method"""
        if not self.SMProxy:
          return getattr(self, name)
        if re.compile("^Set").match(name) and self.SMProxy.GetProperty(name[3:]):
            self.__LastAttrName = name[3:]
            return self.__SetProperty__
        if re.compile("^Get").match(name) and self.SMProxy.GetProperty(name[3:]):
            self.__LastAttrName = name[3:]
            return self.__GetProperty__
        if re.compile("^AddTo").match(name) and self.SMProxy.GetProperty(name[5:]):
            self.__LastAttrName = name[5:]
            return self.__AddToProperty__
        if name == "CreateDisplayProxy" and hasattr(self.SMProxy, "CreateDisplayProxy"):
            return self.__CreateDisplayProxy__
        if name == "AddProxy" and hasattr(self.SMProxy, "AddProxy"):
            return self.__AddProxy__
        return getattr(self.SMProxy, name)
        
class pyProxyManager:
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

    def GetProxy(self, group, name):
        """Returns a pyProxy wrapper for a proxy"""
        if not self.SMProxyManager:
            return None
        proxy = self.SMProxyManager.GetProxy(group, name)
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

class pyPropertyIterator:
    """Wrapper for a vtkSMPropertyIterator class to satisfy
       the python iterator protocol."""
    def __init__(self, proxy):
        self.SMIterator = proxy.NewPropertyIterator()
        self.SMIterator.UnRegister(None)
        self.SMIterator.Begin()
        self.Key = None
        self.Property = None
        self.Proxy = None

    def __iter__(self):
        return self

    def next(self):
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
        """Returns the proxy for the property last returned by the call to 'next()'"""
        return self.Proxy

    def GetKey(self):
        """Returns the key for the property last returned by the call to 'next()' """
        return self.Key

    def GetProperty(self):
        """Returns the property last returned by the call to 'next()' """
        return self.Property

    def __getattr__(self, name):
        """returns attributes from the vtkSMProxyIterator."""
        return getattr(self.SMIterator, name)

class pyProxyIterator:
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
        """Returns the key for the proxy last returned by the call to 'next()' """
        return self.Key

    def GetGroup(self):
        """Returns the group for the proxy last returned by the call to 'next()' """
        return self.Group

    def __getattr__(self, name):
        """returns attributes from the vtkSMProxyIterator."""
        return getattr(self.SMIterator, name)

class pyConnection:
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

    def isRemote(self):
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
    """Connect to a host:port. Returns the connection object if successfully connected 
    with the server."""
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

def connect(ds_host=None, ds_port=None, rs_host=None, rs_port=None):
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

def disconnect(connection):
    """Disconnects the connection."""
    pm =  vtkProcessModule.GetProcessModule()
    pm.Disconnect(connection.ID)
    return

def createProxy(xml_group, xml_name, register_group=None, register_name=None, connection=None):
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

def createRenderWindow(connection=None):
    """Creates a render window on the particular connection. If connection is not specified,
    then the active connection is used, if available."""
    global ActiveConnection
    if not connection:
        connection = ActiveConnection
    if not connection:
        raise "Cannot create render window without connection."
    pxm = pyProxyManager()
    multi_view_render_module = None
    for proxy in pxm.connection_iter(connection):
        if proxy.IsA("vtkSMMultiViewRenderModuleProxy"):
            multi_view_render_module = proxy
            break
    if not multi_view_render_module:
        multi_view_render_module = \
            createProxy("rendermodules", "MultiViewRenderModule", 
              "render_modules", None, connection)
        if connection.isRemote():
            multi_view_render_module.SetRenderModuleName("IceTDesktopRenderModule")
        else:
            multi_view_render_module.SetRenderModuleName("LODRenderModule")
    if not multi_view_render_module:
        raise "Could not locate a MultiViewRenderModule for the connection."
    ren_module = multi_view_render_module.NewRenderModule();
    if not ren_module:
        return None
    ren_module.UnRegister(None)
    pxm.RegisterProxy("render_modules", ren_module.GetSelfIDAsString(), ren_module)
    ren_module.UpdateVTKObjects()
    return pyProxy(ren_module)

def createDisplay(proxy, renModule):
    """Create a display for the proxy and adds it to the render module."""
    if not proxy:
        raise "proxy argument cannot be None."
    if not renModule:
        raise "render module argument cannot be None."
    display = renModule.CreateDisplayProxy()
    if not display:
        return None
    display.UnRegister(None)
    pxm = pyProxyManager()
    pxm.RegisterProxy("displays", display.GetSelfIDAsString(), display)
    display.SetInput(proxy)
    display.UpdateVTKObjects()
    renModule.AddToDisplays(display)
    renModule.UpdateVTKObjects()
    return display
