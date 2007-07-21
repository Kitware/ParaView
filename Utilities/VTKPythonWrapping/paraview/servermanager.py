#==============================================================================
#
#  Program:   ParaView
#  Module:    servermanager.py
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
r"""servermanager is a module for using paraview server manager in Python.
One can always use the server manager directly. However, this module
provides server utilty methods that assist in creating connections,
proxies, as well as introspection.

A simple example:
  from paraview import servermanager
  
  # Creates a new built-in connection and makes it the active connection.
  servermanager.ActiveConnection = servermanager.Connect()

  # Creates a new render view on the active connection.
  renModule = servermanager.CreateRenderView()

  # Create a new sphere proxy on the active connection and register it
  # in the sources group.
  sphere = servermanager.sources.SphereSource(registrationGroup="sources", ThetaResolution=16, PhiResolution=32)

  # Create a representation for the sphere proxy and adds it to the render
  # module.
  display = servermanager.CreateRepresentation(sphere, renModule)

  renModule.ResetCamera()
  renModule.StillRender()
"""
import re
import os
import new
import exceptions
from vtk import *

if os.name == "posix":
    from libvtkPVServerManagerPython import *
else:
    from vtkPVServerManagerPython import *

class Proxy(object):
    """
    Proxy wrapper. Makes it easier to set properties on a proxy.
    Instead of:
     proxy.GetProperty("Foo").SetElement(0, 1)
     proxy.GetProperty("Foo").SetElement(0, 2)
    you can do:
     proxy.Foo = (1,2)
    Instead of:
      proxy.GetPropery("Foo").GetElements()
    you can do:
      proxy.Foo
      it returns a list of all values of the property.
    For proxy properties, you can use +=:
     proxy.GetProperty("Bar").AddProxy(foo)
    you can do:
     proxy.Bar += [foo]

    This class also provides an iterator which can be used to iterate
    over all properties.
    eg:
      proxy = Proxy(proxy=smproxy)
      for property in proxy:
          print property
    """
    def __init__(self, **args):
        if 'proxy' in args:
            self.InitializeFromProxy(args['proxy'])
            del args['proxy']
        else:
            self.Initialize()
        if 'registrationGroup' in args:
            registrationGroup = args['registrationGroup']
            del args['registrationGroup']
            registrationName = self.SMProxy.GetSelfIDAsString()
            if 'registrationName' in args:
                registrationName = args['registrationName']
                del args['registrationName']
            pxm = ProxyManager()
            pxm.RegisterProxy(registrationGroup, registrationGroup, self.SMProxy)
        for key in args.keys():
            self.SetPropertyWithName(key, args[key])
        self.UpdateVTKObjects()
        
    def InitializeFromProxy(self, aProxy):
        "Constructor. Assigns proxy to self.SMProxy"
        self.SMProxy = aProxy
        doc = aProxy.GetDocumentation().GetDescription()
        if doc:
            self.__doc__ = doc
        self.SMProxy.UpdateVTKObjects()

    def Initialize(self):
        "Overridden by the subclass"
        pass
    
    def __eq__(self, other):
        if isinstance(other, Proxy):
            return self.SMProxy == other.SMProxy
        return self.SMProxy == other

    def __ne__(self, other):
        return not self.__eq__(other)

    def __iter__(self):
        return PropertyIterator(self)

    def GetDataInformation(self, idx=0):
        if self.SMProxy:
            return DataInformation(self.SMProxy.GetDataInformation(idx, False), \
                                   self.SMProxy, idx)
        
    def SetPropertyWithName(self, pname, *args):
        """Generic method for setting the value of a property.
        Should not be directly called"""
        # If we have 1 argument and it is a list, treat it as the list
        # of arguments
        value = None
        if len(args):
            value = args[0]
        if not isinstance(value, tuple) and \
           not isinstance(value, list):
            value = (value,)
        property = self.SMProxy.GetProperty(pname)
        if not property:
            raise exceptions.RuntimeError, "Property %s not found. Cannot set." % pname
        if property.IsA("vtkSMInputProperty"):
            property.RemoveAllProxies()
            for i in range(len(value)):
                arg = value[i]
                idx = 0
                if isinstance(arg, OutputPort):
                    idx = arg.PortIdx
                    arg = arg.Proxy
                if isinstance(arg, Proxy):
                    value_proxy = arg.SMProxy
                else:
                    value_proxy = arg
                property.AddInputConnection(value_proxy, idx)
        elif property.IsA("vtkSMProxyProperty"):
            property.RemoveAllProxies()
            for i in range(len(value)):
                if isinstance(value[i], Proxy):
                    value_proxy = value[i].SMProxy
                else:
                    value_proxy = value[i]
                property.AddProxy(value_proxy)
        else:
            for i in range(len(value)):
                property.SetElement(i, value[i])
        self.SMProxy.UpdateProperty(pname, 0)

    def GetPropertyWithName(self, name):
        """Generic method for getting the value of a property.
           Should not be directly called."""
        property = self.SMProxy.GetProperty(name)
        if not property:
            raise exceptions.RuntimeError, "Property %s not found. Cannot set" % name
        if property.IsA("vtkSMProxyProperty"):
            if property.GetRepeatable() or property.GetNumberOfProxies() > 1:
                list = []
                for i in range(0, property.GetNumberOfProxies()):
                    aProxy = property.GetProxy(i)
                    if aProxy:
                        classForProxy = FindClassForProxy(aProxy.GetXMLName())
                        if classForProxy:
                            list.append(classForProxy(proxy=aProxy))
                        else:
                            list.append(Proxy(proxy=aProxy))
                    else:
                        list.append(None)
                return list
            else:
                if property.GetNumberOfProxies() > 0:
                    aProxy = property.GetProxy(0)
                    if aProxy:
                        classForProxy = FindClassForProxy(aProxy.GetXMLName())
                        if classForProxy:
                            return classForProxy(proxy=aProxy)
                        else:
                            return Proxy(proxy=aProxy)
        else:
            if property.GetRepeatable() or \
               property.GetRepeatCommand() or  \
               property.GetNumberOfElements() > 1:
                list = []
                for i in range(0, property.GetNumberOfElements()):
                    list.append(property.GetElement(i))
                return list
            elif property.GetNumberOfElements() == 1:
                return property.GetElement(0)
        return None
        
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
          if issubclass(type(arg), Proxy) or isinstance(arg, Proxy):
              newArgs.append(arg.SMProxy)
          else:
              newArgs.append(arg)
      func = getattr(self.SMProxy, self.__LastAttrName)
      retVal = func(*newArgs)
      if type(retVal) is type(self.SMProxy) and retVal.IsA("vtkSMProxy"):
          classForProxy = FindClassForProxy(retVal.GetXMLName())
          if classForProxy:
              return classForProxy(proxy=retVal)
          else:
              return Proxy(proxy=retVal)
      else:
          return retVal

    def __getattr__(self, name):
        """With the exception of a few overloaded methods,
        returns the SMProxy method"""
        if not self.SMProxy:
            return getattr(self, name)
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

class DataInformation(object):
    def __init__(self, dataInformation, proxy, idx):
        self.DataInformation = dataInformation
        self.Proxy = proxy
        self.Idx = idx

    def Update(self):
        if self.Proxy:
            self.Proxy.GetDataInformation(self.Idx, False)
            
    def GetDataSetType(self):
        self.Update()
        if not self.DataInformation:
            raise exception.RuntimeError, "No data information is available"
        if self.DataInformation.GetCompositeDataSetType() > -1:
            return self.DataInformation.GetCompositeDataSetType()
        return self.DataInformation.GetDataSetType()

    def GetDataSetTypeAsString(self):
        return vtkDataObjectTypes.GetClassNameFromTypeId(self.GetDataSetType())

    def __getattr__(self, name):
        if not self.DataInformation:
            return getattr(self, name)
        self.Update()
        return getattr(self.DataInformation, name)
    
class OutputPort(object):
    def __init__(self, proxy=None, outputPort=0):
        self.Proxy = proxy
        self.PortIdx = outputPort
        
class ProxyManager(object):
    "Proxy manager wrapper"

    def __init__(self):
        """Constructor. Assigned self.SMProxyManager to
        vtkSMObject.GetPropertyManager(). Make sure to initialize
        server manager before creating a ProxyManager"""
        self.SMProxyManager = vtkSMObject.GetProxyManager()

    def RegisterProxy(self, group, name, aProxy):
        """Registers a proxy (either SMProxy or proxy) with the
        server manager"""
        if isinstance(aProxy, Proxy):
            self.SMProxyManager.RegisterProxy(group, name, aProxy.SMProxy)
        else:
            self.SMProxyManager.RegisterProxy(group, name, aProxy)

    def NewProxy(self, group, name):
        """Creates a new proxy of given group and name and returns a proxy
        wrapper"""
        if not self.SMProxyManager:
            return None
        aProxy = self.SMProxyManager.NewProxy(group, name)
        if not aProxy:
            return None
        aProxy.UnRegister(None)
        return aProxy

    def GetProxy(self, group, name):
        """Returns a proxy wrapper for a proxy"""
        if not self.SMProxyManager:
            return None
        aProxy = self.SMProxyManager.GetProxy(group, name)
        if not aProxy:
            return None
        classForProxy = FindClassForProxy(aProxy.GetXMLName())
        if classForProxy:
            return classForProxy(proxy=aProxy)
        else:
            return Proxy(proxy=aProxy)

    def GetPrototypeProxy(self, group, name):
        """Returns a proxy wrapper for a proxy"""
        if not self.SMProxyManager:
            return None
        aProxy = self.SMProxyManager.GetPrototypeProxy(group, name)
        if not aProxy:
            return None
        return aProxy

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
        for aProxy in iter:
            proxies[iter.GetKey()] = aProxy;
        return proxies

    def UnRegisterProxy(self, groupname, proxyname, aProxy):
        """Unregisters a proxy."""
        if not self.SMProxyManager:
            return 
        if aProxy != None and isinstance(aProxy,Proxy):
            aProxy = aProxy.SMProxy
        if aProxy:
            self.SMProxyManager.UnRegisterProxy(groupname, proxyname, aProxy)

    def GetProxies(self, groupname, proxyname):
        """Returns all proxies registered under the given group with the given name."""
        if not self.SMProxyManager:
            return []
        collection = vtkCollection()
        result = []
        self.SMProxyManager.GetProxies(groupname, proxyname, collection)
        for i in range(0, collection.GetNumberOfItems()):
            aProxy = collection.GetItemAsObject(i)
            if aProxy:
                classForProxy = FindClassForProxy(aProxy.GetXMLName())
                if classForProxy:
                    aProxy = classForProxy(proxy=aProxy)
                else:
                    aProxy = Proxy(proxy=aProxy)
            result.append(aProxy)
        return result
        
    def __getattr__(self, name):
        """Returns attribute from the ProxyManager"""
        return getattr(self.SMProxyManager, name)

    def __iter__(self):
        return ProxyIterator()

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
        iter = ProxyDefinitionIterator()
        if groupname != None:
            iter.SetModeToOneGroup()
            iter.Begin(groupname)
        return iter

    def ListProperties(self, groupname, proxyname):
        """Returns a list of all property names for a
           proxy of the given type."""
        aProxy = self.GetPrototypeProxy(groupname, proxyname)
        if aProxy:
            return aProxy.ListProperties()
        

class PropertyIterator(object):
    """Wrapper for a vtkSMPropertyIterator class to satisfy
       the python iterator protocol."""
    def __init__(self, aProxy):
        self.SMIterator = aProxy.NewPropertyIterator()
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

class ProxyDefinitionIterator(object):
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


class ProxyIterator(object):
    """Wrapper for a vtkSMProxyIterator class to satisfy the
     python iterator protocol."""
    def __init__(self):
        self.SMIterator = vtkSMProxyIterator()
        self.SMIterator.Begin()
        self.AProxy = None
        self.Group = None
        self.Key = None

    def __iter__(self):
        return self

    def next(self):
        if self.SMIterator.IsAtEnd():
            self.AProxy = None
            self.Group = None
            self.Key = None
            raise StopIteration
            return None
        self.AProxy = self.SMIterator.GetProxy()
        if self.AProxy:
            classForProxy = FindClassForProxy(self.AProxy.GetXMLName())
            if classForProxy:
                self.AProxy = classForProxy(proxy=self.AProxy)
            else:
                self.AProxy = Proxy(proxy=self.AProxy)
        self.Group = self.SMIterator.GetGroup()
        self.Key = self.SMIterator.GetKey()
        self.SMIterator.Next()
        return self.AProxy

    def GetProxy(self):
        """Returns the proxy last returned by the call to 'next()'"""
        return self.AProxy

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

class Connection(object):
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
    conn = Connection(cid)
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
    conn = Connection(cid)
    conn.SetHost(ds_host, ds_port, rs_host, rs_port)
    return conn 

def connect_self():
    """Creates a new self connection."""
    pm =  vtkProcessModule.GetProcessModule()
    cid = pm.ConnectToSelf()
    if not cid:
        return None
    conn = Connection(cid)
    conn.SetHost("builtin", cid)
    return conn

def Connect(ds_host=None, ds_port=11111, rs_host=None, rs_port=11111):
    """
    Use this function call to create a new connection. On success,
    it returns a Connection object that abstracts the connection.
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

def Disconnect(connection=None):
    """Disconnects the connection."""
    global ActiveConnection
    if not connection:
        connection = ActiveConnection
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
    pxm = ProxyManager()
    aProxy = pxm.NewProxy(xml_group, xml_name)
    if not aProxy:
        return None
    if not connection:
        connection = ActiveConnection
    if connection:
        aProxy.SetConnectionID(connection.ID)
    if register_group:
        if not register_name:
            register_name = aProxy.GetSelfIDAsString()
        pxm.RegisterProxy(register_group, register_name, aProxy)
    return aProxy

def GetRenderView():
    """Return the render view in use.  If more than one render view is in use, return the first one."""
    render_module = None
    for aProxy in ProxyManager().connection_iter(ActiveConnection):
        if aProxy.IsA("vtkSMRenderViewProxy"):
            render_module = aProxy
            break
    return render_module

def GetRenderViews():
    """Returns the set of all render views."""
    render_modules = []
    for aProxy in ProxyManager().connection_iter(ActiveConnection):
        if aProxy.IsA("vtkSMRenderViewProxy"):
            render_modules.append(aProxy)
    return render_modules

def CreateRenderView(connection=None):
    """Creates a render window on the particular connection. If connection is not specified,
    then the active connection is used, if available."""
    global ActiveConnection
    if not connection:
        connection = ActiveConnection
    if not connection:
        raise exception.RuntimeError, "Cannot create render window without connection."
    pxm = ProxyManager()
    proxy_xml_name = None
    if connection.IsRemote():
        proxy_xml_name = "IceTDesktopRenderView"
    else:
        proxy_xml_name = "RenderView"
    ren_module = CreateProxy("newviews", proxy_xml_name, "view_modules")
    if not ren_module:
        return None
    proxy = rendering.__dict__[ren_module.GetXMLName()](proxy=ren_module)
    return proxy

def CreateRepresentation(aProxy, view):
    """Creates a representation for the proxy and adds it to the render module."""
    global rendering
    if not aProxy:
        raise exceptions.RuntimeError, "proxy argument cannot be None."
    if not view:
        raise exceptions.RuntimeError, "render module argument cannot be None."
    display = view.SMProxy.CreateDefaultRepresentation(aProxy.SMProxy, 0)
    if not display:
        return None
    display.SetConnectionID(aProxy.GetConnectionID())
    display.UnRegister(None)
    pxm = ProxyManager()
    pxm.RegisterProxy("displays", display.GetSelfIDAsString(), display)
    proxy = rendering.__dict__[display.GetXMLName()](proxy=display)
    proxy.Input = aProxy
    proxy.UpdateVTKObjects()
    view.Representations += [proxy]
    return proxy

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
    gvd = rendering.ClientDeliveryRepresentationBase()
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

def __createInitialize(group, name):
    pgroup = group
    pname = name
    def aInitialize(self):
        global ActiveConnection
        if not ActiveConnection:
            raise exceptions.RuntimeError, 'Cannot create a proxy without a connection.'
        self.InitializeFromProxy(CreateProxy(pgroup, pname, pgroup))
    return aInitialize
def __createGetProperty(pName):
    propName = pName
    def getProperty(self):
        return self.GetPropertyWithName(propName)
    return getProperty
def __createSetProperty(pName):
    propName = pName
    def setProperty(self, value):
        return self.SetPropertyWithName(propName, value)
    return setProperty
def FindClassForProxy(xmlName):
    global sources, filters
    if xmlName in sources.__dict__:
        return sources.__dict__[xmlName]
    elif xmlName in filters.__dict__:
        return filters.__dict__[xmlName]
    else:
        return None
    
def __createModule(groupName, mdl=None):
    pxm = vtkSMObject.GetProxyManager()
    pxm.InstantiateGroupPrototypes(groupName)

    if not mdl:
        mdl = new.module(groupName)
    numProxies = pxm.GetNumberOfXMLProxies(groupName)
    for i in range(numProxies):
        pname = pxm.GetXMLProxyName(groupName, i)
        cdict = {}
        cdict['Initialize'] = __createInitialize(groupName, pname)
        proto = pxm.GetPrototypeProxy(groupName, pname)
        iter = PropertyIterator(proto)
        for prop in iter:
            propName = iter.GetKey()
            propDoc = None
            if prop.GetDocumentation():
                propDoc = prop.GetDocumentation().GetDescription()
            cdict[propName] = property(__createGetProperty(propName),
                                       __createSetProperty(propName),
                                       None,
                                       propDoc)
        if proto.GetDocumentation():
            cdict['__doc__'] = proto.GetDocumentation().GetDescription()
        cobj = type(pname, (Proxy,), cdict)
        mdl.__dict__[pname] = cobj
    return mdl

sources = __createModule('sources')
filters = __createModule('filters')
rendering = __createModule('representations')
__createModule('newviews', rendering)

def test():
#ActiveConnection = Connect()
    ss = sources.SphereSource(Radius=2, ThetaResolution=32)
    shr = filters.ShrinkFilter(Input=OutputPort(ss,0))
    cs = sources.ConeSource()
    app = filters.Append()
    app.Input = [shr, cs]
    rv = CreateRenderView()
    rep = CreateRepresentation(app, rv)
    rv.ResetCamera()
    rv.StillRender()
    cf = filters.Contour()
    data = Fetch(ss)
