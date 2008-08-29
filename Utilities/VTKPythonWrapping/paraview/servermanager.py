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
One can always use the server manager API directly. However, this module
provides an interface easier to use from Python by wrapping several VTK
classes around Python classes.

Note that, upon load, this module will create several sub-modules: sources,
filters and rendering. These modules can be used to instantiate specific
proxy types. For a list, try "dir(servermanager.sources)"

A simple example:
  from paraview.servermanager import *
  
  # Creates a new built-in connection and makes it the active connection.
  Connect()

  # Creates a new render view on the active connection.
  renModule = CreateRenderView()

  # Create a new sphere proxy on the active connection and register it
  # in the sources group.
  sphere = sources.SphereSource(registrationGroup="sources", ThetaResolution=16, PhiResolution=32)

  # Create a representation for the sphere proxy and adds it to the render
  # module.
  display = CreateRepresentation(sphere, renModule)

  renModule.StillRender()
"""
import re, os, new, exceptions, sys, vtk

if os.name == "posix":
    from libvtkPVServerCommonPython import *
    from libvtkPVServerManagerPython import *
else:
    from vtkPVServerCommonPython import *
    from vtkPVServerManagerPython import *

class Proxy(object):
    """
    Proxy wrapper. Makes it easier to set/get properties on a proxy.
    Instead of:
     proxy.GetProperty("Foo").SetElement(0, 1)
     proxy.GetProperty("Foo").SetElement(0, 2)
    you can do:
     proxy.Foo = (1,2)
    or
     proxy.Foo.SetData((1,2))
    or
     proxy.Foo[0:2] = (1,2)
    Instead of:
      proxy.GetPropery("Foo").GetElement(0)
    you can do:
      proxy.Foo.GetData()[0]
    or
      proxy.Foo[0]
    For proxy properties, you can use append:
     proxy.GetProperty("Bar").AddProxy(foo)
    you can do:
     proxy.Bar.append(foo)
    Properties support most of the list API. See VectorProperty and
    ProxyProperty documentation for details.

    This class also provides an iterator which can be used to iterate
    over all properties.
    eg:
      proxy = Proxy(proxy=smproxy)
      for property in proxy:
          print property

    Please note that some of the methods accessible through the Proxy
    class are not listed by help() because the Proxy objects forward
    unresolved attributes to the underlying object. To get the full list,
    see also dir(proxy.SMProxy). See also the doxygen based documentation
    of the vtkSMProxy C++ class.
    """
    def __init__(self, **args):
        """ Default constructor. It can be used to initialize properties
        by passing keyword arguments where the key is the name of the
        property. In addition registrationGroup and registrationName (optional)
        can be specified (as keyword arguments) to automatically register
        the proxy with the proxy manager. """
        self.Observed = None
        self.ObserverTag = -1
        self.__Properties = {}
        self.SMProxy = None

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
            pxm.RegisterProxy(registrationGroup, registrationName, self.SMProxy)
        for key in args.keys():
            self.SetPropertyWithName(key, args[key])
        self.UpdateVTKObjects()

    def __del__(self):
        """Destructor. Cleans up all observers as well as remove
        the proxy for the _pyproxies dictionary"""
        # Make sure that we remove observers we added
        if self.Observed:
            observed = self.Observed
            tag = self.ObserverTag
            self.Observed = None
            self.ObserverTag = -1
            observed.RemoveObserver(tag)
        if self.SMProxy and self.SMProxy in _pyproxies:
            del _pyproxies[self.SMProxy]

    def InitializeFromProxy(self, aProxy):
        """Constructor. Assigns proxy to self.SMProxy, updates the server
        object as well as register the proxy in _pyproxies dictionary."""
        import weakref
        self.SMProxy = aProxy
        self.SMProxy.UpdateVTKObjects()
        _pyproxies[self.SMProxy] = weakref.ref(self)

    def Initialize(self):
        "Overridden by the subclass created automatically"
        pass
    
    def __eq__(self, other):
        "Returns true if the underlying SMProxies are the same."
        if isinstance(other, Proxy):
            return self.SMProxy == other.SMProxy
        return self.SMProxy == other

    def __ne__(self, other):
        "Returns false if the underlying SMProxies are the same."
        return not self.__eq__(other)

    def __iter__(self):
        "Creates an iterator for the properties."
        return PropertyIterator(self)

    def SetPropertyWithName(self, pname, arg):
        """Generic method for setting the value of a property."""
        prop = self.GetProperty(pname)
        if prop is None:
            raise exceptions.RuntimeError, "Property %s does not exist. Please check the property name for typos." % pname
        prop.SetData(arg)
         
    def GetProperty(self, name):
        """Given a property name, returns the property object."""
        if name in self.__Properties and self.__Properties[name]():
            return self.__Properties[name]()
        smproperty = self.SMProxy.GetProperty(name)
        if smproperty:
            property = None
            if smproperty.IsA("vtkSMVectorProperty"):
                property = VectorProperty(self, smproperty)
            elif smproperty.IsA("vtkSMInputProperty"):
                property = InputProperty(self, smproperty)
            elif smproperty.IsA("vtkSMProxyProperty"):
                property = ProxyProperty(self, smproperty)
            else:
                property = Property(self, smproperty)
            if property is not None:
                import weakref
                self.__Properties[name] = weakref.ref(property)
            return property
        return None
            
    def ListProperties(self):
        """Returns a list of all property names on this proxy."""
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
          return _getPyProxy(retVal)
      elif type(retVal) is type(self.SMProxy) and retVal.IsA("vtkSMProperty"):
          property = retVal
          if property.IsA("vtkSMVectorProperty"):
              return VectorProperty(self, property)
          elif property.IsA("vtkSMInputProperty"):
              return InputProperty(self, property)
          elif property.IsA("vtkSMProxyProperty"):
              return ProxyProperty(self, property)
          else:
              return None
      else:
          return retVal

    def __GetActiveCamera(self):
        """ This method handles GetActiveCamera specially. It adds
        an observer to the camera such that everything it is modified
        the render view updated"""
        import weakref
        c = self.SMProxy.GetActiveCamera()
        if not c.HasObserver("ModifiedEvent"):
            self.ObserverTag =c.AddObserver("ModifiedEvent", \
                              _makeUpdateCameraMethod(weakref.ref(self)))
            self.Observed = c
        return c
        
    def __getattr__(self, name):
        """With the exception of a few overloaded methods,
        returns the SMProxy method"""
        if not self.SMProxy:
            return getattr(self, name)
        # Handle GetActiveCamera specially.
        if name == "GetActiveCamera" and \
           hasattr(self.SMProxy, "GetActiveCamera"):
            return self.__GetActiveCamera
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

class SourceProxy(Proxy):
    """Source proxy wrapper. This class adds a few methods to Proxy
    that are specific to source proxies."""
    
    def UpdatePipeline(self):
        """This method updates the server-side VTK pipeline and the associated
        data information."""
        self.SMProxy.UpdatePipeline()
        # Fetch the new information. This is also here to cause a receive
        # on the client side so that progress works properly.
        self.SMProxy.GetDataInformation()

    def GetDataInformation(self, idx=0):
        """This method returns a DataInformation wrapper around a
        vtkPVDataInformation"""
        if self.SMProxy:
            return DataInformation( \
                self.SMProxy.GetDataInformation(idx), \
                self.SMProxy, idx)
                
class Property(object):
    """Python wrapper around a vtkSMProperty with a simple interface.
    In addition to all method provided by vtkSMProperty (obtained by
    forwarding unknown attributes requests to the underlying SMProxy),
    Property and sub-class provide a list API.

    Please note that some of the methods accessible through the Property
    class are not listed by help() because the Property objects forward
    unresolved attributes to the underlying object. To get the full list,
    see also dir(proxy.SMProperty). See also the doxygen based documentation
    of the vtkSMProperty C++ class.
    """
    def __init__(self, proxy, smproperty):
        """Default constructor. Stores a reference to the proxy."""
        import weakref
        self.SMProperty = smproperty
        self.Proxy = proxy

    def __repr__(self):
        """Returns a string representation containing property name
        and value"""
        repr = "Property name= "
        name = self.Proxy.GetPropertyName(self.SMProperty)
        if name:
            repr += name
        else:
            repr += "Unknown"
        if not type(self) is Property:
            repr += " value = "
            if self.GetData() is not None:
                repr += self.GetData().__repr__()
            else:
                repr += "None"
        return repr

    def __call__(self):
        if type(self) is Property:
            self.Proxy.SMProxy.InvokeCommand(self._FindPropertyName())
        else:
            raise exceptions.RuntimeError, "Cannot invoke this property"
        
    def _FindPropertyName(self):
        "Returns the name of this property."
        return self.Proxy.GetPropertyName(self.SMProperty)

    def _UpdateProperty(self):
        "Pushes the value of this property to the server."
        # For now, we are updating all properties. This is due to an
        # issue with the representations. Their VTK objects are not
        # created until Input is set; therefore, updating a property
        # has no effect. Updating all properties everytime one is
        # updated has the effect of pushing values set before Input
        # when Input is updated.
        # self.Proxy.SMProxy.UpdateProperty(self._FindPropertyName())
        self.Proxy.SMProxy.UpdateVTKObjects()

class VectorProperty(Property):
    """Python wrapper for vtkSMVectorProperty and sub-classes.
    In addition to vtkSMVectorProperty methods, this class provides
    a list interface, allowing things like:
    val = property[2]
    property[1:3] = (1, 2)
    """
    def ConvertValue(self, value):
       """Converts value to type suitable for vtSM.Property::SetElement()"""
       if self.SMProperty.IsA("vtkSMIntVectorProperty") and \
         self.SMProperty.GetDomain("enum") and type(value) == str:
           domain = self.SMProperty.GetDomain("enum")
           if domain.HasEntryText(value):
              return domain.GetEntryValueForText(value)
       return value

    
    def __len__(self):
        """Returns the number of elements."""
        return self.SMProperty.GetNumberOfElements()

    def __getitem__(self, idx):
        """Given an index, return an element. Raises an IndexError
        exception if the argument is out of bounds."""
        if idx >= len(self):
            raise exceptions.IndexError
        return self.SMProperty.GetElement(idx)

    def __setitem__(self, idx, value):
        """Given an index and a value, sets an element."""
        self.SMProperty.SetElement(idx, self.ConvertValue(value))
        self._UpdateProperty()

    def __getslice__(self, min, max):
        """Returns the range [min, max) of elements. Raises an IndexError
        exception if an argument is out of bounds."""
        if min < 0 or min > len(self) or max < 0 or max > len(self):
            raise exceptions.IndexError
        retval = []
        for i in range(min, max):
            retval.append(self.SMProperty.GetElement(i))
        return retval

    def __setslice__(self, min, max, values):
        "Given a list or tuple of values, sets a slice of values [min, max)."
        if min < 0 or min > len(self) or max < 0 or max > len(self):
            raise exceptions.IndexError
        for i in range(min, max):
            self.SMProperty.SetElement(i, self.ConvertValue(values[i-min]))
        self._UpdateProperty()

    def __getattr__(self, name):
        "Unknown attribute requests get forwarded to SMProperty."
        return getattr(self.SMProperty, name)

    def GetData(self):
        "Returns all elements as either a list or a single value."
        property = self.SMProperty
        if property.GetRepeatable() or \
           property.GetRepeatCommand() or  \
           property.GetNumberOfElements() > 1:
            return self.__getslice__(0, len(self))
        elif property.GetNumberOfElements() == 1:
            return property.GetElement(0)

    def SetData(self, values):
        """Allows setting of all values at once. Requires a single value,
        a tuple or list."""
        if not isinstance(values, tuple) and \
           not isinstance(values, list):
            values = (values,)
        for i in range(len(values)):
            self.SMProperty.SetElement(i, self.ConvertValue(values[i]))
        self._UpdateProperty()

    def Clear(self):
        "Removes all elements."
        self.SMProperty().SetNumberOfElements(0)
        self._UpdateProperty()

class ProxyProperty(Property):
    """Python wrapper for vtkSMProxyProperty.
    In addition to vtkSMProxyProperty methods, this class provides
    a list interface, allowing things like:
    proxy = property[2]
    property[1:3] = (px1, px2)
    property.append(proxy)
    del property[1]
    """
    def __len__(self):
        """Returns the number of elements."""
        return self.SMProperty.GetNumberOfProxies()

    def __getitem__(self, idx):
        """Given an index, return an element. Raises an IndexError
        exception if the argument is out of bounds."""
        if idx >= len(self):
            raise exceptions.IndexError
        return _getPyProxy(self.SMProperty.GetProxy(idx))

    def __setitem__(self, idx, value):
        """Given an index and a value, sets an element."""
        self.SMProperty.SetProxy(idx, value.SMProxy)
        self._UpdateProperty()

    def __delitem__(self, idx):
        """Removes the element idx"""
        proxy = self[idx].SMProxy
        self.SMProperty.RemoveProxy(proxy)
        self._UpdateProperty()

    def __getslice__(self, min, max):
        """Returns the range [min, max) of elements. Raises an IndexError
        exception if an argument is out of bounds."""
        if min < 0 or min > len(self) or max < 0 or max > len(self):
            raise exceptions.IndexError
        retval = []
        for i in range(min, max):
            proxy = _getPyProxy(self.SMProperty.GetProxy(i))
            retval.append(proxy)
        return retval

    def __setslice__(self, min, max, values):
        "Given a list or tuple of values, sets a slice of values [min, max)."
        if min < 0 or min > len(self) or max < 0 or max > len(self):
            raise exceptions.IndexError
        for i in range(min, max):
            self.SMProperty.SetProxy(i, values[i-min].SMProxy)
        self._UpdateProperty()

    def __delslice__(self, min, max):
        """Removes elements [min, max)"""
        proxies = []
        for i in range(min, max):
            proxies.append(self[i].SMProxy)
        for i in proxies:
            self.SMProperty.RemoveProxy(i)
        self._UpdateProperty()

    def __getattr__(self, name):
        "Unknown attribute requests get forwarded to SMProperty."
        return getattr(self.SMProperty, name)

    def append(self, proxy):
        "Appends the given proxy to the property values."
        self.SMProperty.AddProxy(proxy.SMProxy)
        self._UpdateProperty()
        
    def GetData(self):
        "Returns all elements as either a list or a single value."
        property = self.SMProperty
        if property.GetRepeatable() or property.GetNumberOfProxies() > 1:
            return self.__getslice__(0, len(self))
        else:
            if property.GetNumberOfProxies() > 0:
                return _getPyProxy(property.GetProxy(0))
        return None

    def SetData(self, values):
        """Allows setting of all values at once. Requires a single value,
        a tuple or list."""
        if not isinstance(values, tuple) and \
           not isinstance(values, list):
            values = (values,)
        self.SMProperty.RemoveAllProxies()
        for value in values:
            if isinstance(value, Proxy):
                value_proxy = value.SMProxy
            else:
                value_proxy = value
            self.SMProperty.AddProxy(value_proxy)
        self._UpdateProperty()

    def Clear(self):
        "Removes all elements."
        self.SMProperty.RemoveAllProxies()
        self._UpdateProperty()

class InputProperty(ProxyProperty):
    """Python wrapper for vtkSMInputProperty.
    In addition to vtkSMInputProperty methods, this class provides
    a list interface, allowing things like:
    outputport = property[2]
    property[0] = proxy
     or
    property[0] = OuputPort(proxy, 1)
    property.append(OutputPort(proxy, 0))
    del property[1]
    """

    def __getitem__(self, idx):
        """Given an index, return an element as an OutputPort object.
        Raises an IndexError exception if the argument is out of bounds."""
        if idx >= len(self):
            raise exceptions.IndexError
        return OutputPort(_getPyProxy(self.SMProperty.GetProxy(idx)),\
                          self.SMProperty.GetOutputPortForConnection(idx))

    def __getOutputPort(self, value):
        portidx = 0
        if isinstance(value, OutputPort):
            portidx = value.Port
            value = value.Proxy
        if isinstance(value, Proxy):
            value_proxy = value.SMProxy
        else:
            value_proxy = value
        return OutputPort(value_proxy, portidx)
        
    def __setitem__(self, idx, value):
        """Given an index and a value, sets an element. Accepts Proxy or
        OutputPort objects."""
        op = self.__getOutputPort(value)
        self.SMProperty.SetInputConnection(idx, op.Proxy, op.Port)
        self._UpdateProperty()

    def __getslice__(self, min, max):
        """Returns the range [min, max) of elements as a list of OutputPort
        objects. Raises an IndexError exception if an argument is out of
        bounds."""
        if min < 0 or min > len(self) or max < 0 or max > len(self):
            raise exceptions.IndexError
        retval = []
        for i in range(min, max):
            port = OutputPort(_getPyProxy(self.SMProperty.GetProxy(i)),\
                              self.SMProperty.GetOutputPortForConnection(i))
            retval.append(port)
        return retval

    def __setslice__(self, min, max, values):
        """Given a list or tuple of values, sets a slice of values [min, max).
        Accepts Proxy or OutputPort objects."""
        if min < 0 or min > len(self) or max < 0 or max > len(self):
            raise exceptions.IndexError
        for i in range(min, max):
            op = self.__getOutputPort(value[i-min])
            self.SMProperty.SetInputConnection(i, op.Proxy, op.Port)
        self._UpdateProperty()

    def append(self, value):
        """Appends the given proxy to the property values.
        Accepts Proxy or OutputPort objects."""
        op = self.__getOutputPort(value)
        self.SMProperty.AddInputConnection(op.Proxy, op.Port)
        self._UpdateProperty()
        
    def GetData(self):
        """Returns all elements as either a list of OutputPort objects or
        a single OutputPort object."""
        property = self.SMProperty
        if property.GetRepeatable() or property.GetNumberOfProxies() > 1:
            return self.__getslice__(0, len(self))
        else:
            if property.GetNumberOfProxies() > 0:
                return OutputPort(_getPyProxy(property.GetProxy(0)),\
                                  self.SMProperty.GetOutputPortForConnection(0))
        return None

    def SetData(self, values):
        """Allows setting of all values at once. Requires a single value,
        a tuple or list. Accepts Proxy or OutputPort objects."""
        if not isinstance(values, tuple) and \
           not isinstance(values, list):
            values = (values,)
        self.SMProperty.RemoveAllProxies()
        for value in values:
            op = self.__getOutputPort(value)
            self.SMProperty.AddInputConnection(op.Proxy, op.Port)
        self._UpdateProperty()
        
class DataInformation(object):
    """Python wrapper around a vtkPVDataInformation. In addition to
    proving all methods of a vtkPVDataInformation, it provides a
    few convenience methods.

    Please note that some of the methods accessible through the DataInformation
    class are not listed by help() because the DataInformation objects forward
    unresolved attributes to the underlying object. To get the full list,
    see also dir(proxy.DataInformation).
    See also the doxygen based documentation of the vtkPVDataInformation C++
    class.
    """
    def __init__(self, dataInformation, proxy, idx):
        """Default constructor. Requires a vtkPVDataInformation, a source proxy
        and an output port id."""
        self.DataInformation = dataInformation
        self.Proxy = proxy
        self.Idx = idx

    def Update(self):
        """****Deprecated**** There is no reason anymore to use this method
        explicitly, it is called automatically when one gets any value from the
        data information object.
        Update the data information if necessary. Note that this
        does not cause execution of the underlying object. In certain
        cases, you may have to call UpdatePipeline() on the proxy."""
        if self.Proxy:
            self.Proxy.GetDataInformation(self.Idx)
            
    def GetDataSetType(self):
        """Returns the dataset type as defined in vtkDataObjectTypes."""
        self.Update()
        if not self.DataInformation:
            raise exceptions.RuntimeError, "No data information is available"
        if self.DataInformation.GetCompositeDataSetType() > -1:
            return self.DataInformation.GetCompositeDataSetType()
        return self.DataInformation.GetDataSetType()

    def GetDataSetTypeAsString(self):
        """Returns the dataset type as a user-friendly string. This is
        not the same as the enumaration used by VTK"""
        return vtk.vtkDataObjectTypes.GetClassNameFromTypeId(self.GetDataSetType())

    def __getattr__(self, name):
        """Forwards unknown attribute requests to the underlying
        vtkPVInformation."""
        if not self.DataInformation:
            return getattr(self, name)
        self.Update()
        return getattr(self.DataInformation, name)
    
class OutputPort(object):
    """Helper class to store a reference to an output port. Used to
    set pipeline connections."""
    
    def __init__(self, proxy, outputPort=0):
        """Default constructor. It takes a Proxy and an optional port
        index."""
        self.Proxy = proxy
        self.Port = outputPort

    def __repr__(self):
        """Returns a user-friendly representation string."""
        import exceptions
        try:
            return self.Proxy.__repr__() + ":%d" % self.Port
        except exceptions.AttributeError:
            return object.__repr__(self)
        
class ProxyManager(object):
    """Python wrapper for vtkSMProxyManager. Note that the underlying
    vtkSMProxyManager is a singleton. All instances of this class will
    refer to the same object. In addition to all methods provided by
    vtkSMProxyManager (all unknown attribute requests are forwarded
    to the vtkSMProxyManager), this class provides several convenience
    methods.

    When running scripts from the python shell in the ParaView application,
    registering proxies with the proxy manager is the ony mechanism to
    notify the graphical user interface (GUI) that a proxy
    exists. Therefore, unless a proxy is registered, it will not show up in
    the user interface. Also, the proxy manager is the only way to get
    access to proxies created using the GUI. Proxies created using the GUI
    are automatically registered under an appropriate group (sources,
    filters, representations and views). To get access to these objects,
    you can use proxyManager.GetProxy(group, name). The name is the same
    as the name shown in the pipeline browser.

    Please note that some of the methods accessible through the ProxyManager
    class are not listed by help() because the ProxyManager objects forwards
    unresolved attributes to the underlying object. To get the full list,
    see also dir(proxy.SMProxyManager). See also the doxygen based documentation
    of the vtkSMProxyManager C++ class.
    """

    def __init__(self):
        """Constructor. Assigned self.SMProxyManager to
        vtkSMObject.GetPropertyManager()."""
        self.SMProxyManager = vtkSMObject.GetProxyManager()

    def RegisterProxy(self, group, name, aProxy):
        """Registers a proxy (either SMProxy or proxy) with the
        server manager"""
        if isinstance(aProxy, Proxy):
            self.SMProxyManager.RegisterProxy(group, name, aProxy.SMProxy)
        else:
            self.SMProxyManager.RegisterProxy(group, name, aProxy)

    def NewProxy(self, group, name):
        """Creates a new proxy of given group and name and returns an SMProxy.
        Note that this is a server manager object. You should normally create
        proxies using the class objects. For example:
        obj = servermanager.sources.SphereSource()"""
        if not self.SMProxyManager:
            return None
        aProxy = self.SMProxyManager.NewProxy(group, name)
        if not aProxy:
            return None
        aProxy.UnRegister(None)
        return aProxy

    def GetProxy(self, group, name):
        """Returns a Proxy registered under a group and name"""
        if not self.SMProxyManager:
            return None
        aProxy = self.SMProxyManager.GetProxy(group, name)
        if not aProxy:
            return None
        return _getPyProxy(aProxy)

    def GetPrototypeProxy(self, group, name):
        """Returns a prototype proxy given a group and name. This is an
        SMProxy. This is a low-level method. You should not normally
        have to call it."""
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
        iter = self.NewConnectionIterator(connection) 
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
        iter = self.NewGroupIterator(groupname) 
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
        """Returns all proxies registered under the given group with the
        given name. Note that it is possible to register more than one
        proxy with the same name in the same group. Because the proxies
        are different, there is no conflict. Use this method instead of
        GetProxy() if you know that there are more than one proxy registered
        with this name."""
        if not self.SMProxyManager:
            return []
        collection = vtk.vtkCollection()
        result = []
        self.SMProxyManager.GetProxies(groupname, proxyname, collection)
        for i in range(0, collection.GetNumberOfItems()):
            aProxy = _getPyProxy(collection.GetItemAsObject(i))
            if aProxy:
                result.append(aProxy)
                
        return result
        
    def __iter__(self):
        """Returns a new ProxyIterator."""        
        iter = ProxyIterator()
        if ActiveConnection:
            iter.SetConnectionID(ActiveConnection.ID)
        iter.Begin()
        return iter

    def NewGroupIterator(self, group_name, connection=None):
        """Returns a ProxyIterator for a group. The resulting object
        can be used to traverse the proxies that are in the given
        group."""
        iter = self.__iter__()
        if not connection:
            connection = ActiveConnection
        if connection:
            iter.SetConnectionID(connection.ID)
        iter.SetModeToOneGroup()
        iter.Begin(group_name)
        return iter

    def NewConnectionIterator(self, connection=None):
        """Returns a ProxyIterator for a given connection. This can be
        used to travers ALL proxies managed by the proxy manager."""
        iter = self.__iter__()
        if not connection:
            connection = ActiveConnection
        if connection:
            iter.SetConnectionID(connection.ID)
        iter.Begin()
        return iter

    def NewDefinitionIterator(self, groupname=None):
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

    def __ConvertArgumentsAndCall(self, *args):
      newArgs = []
      for arg in args:
          if issubclass(type(arg), Proxy) or isinstance(arg, Proxy):
              newArgs.append(arg.SMProxy)
          else:
              newArgs.append(arg)
      func = getattr(self.SMProxyManager, self.__LastAttrName)
      retVal = func(*newArgs)
      if type(retVal) is type(self.SMProxyManager) and retVal.IsA("vtkSMProxy"):
          return _getPyProxy(retVal)
      elif type(retVal) is type(self.SMProxyManager) and retVal.IsA("vtkSMProperty"):
          property = retVal
          if property.IsA("vtkSMVectorProperty"):
              return VectorProperty(self, property)
          elif property.IsA("vtkSMInputProperty"):
              return InputProperty(self, property)
          elif property.IsA("vtkSMProxyProperty"):
              return ProxyProperty(self, property)
          else:
              return None
      else:
          return retVal

    def __getattr__(self, name):
        """Returns attribute from the ProxyManager"""
        try:
            pmAttr = getattr(self.SMProxyManager, name)
            self.__LastAttrName = name
            return self.__ConvertArgumentsAndCall
        except:
            pass
        return getattr(self.SMProxyManager, name)
        

class PropertyIterator(object):
    """Wrapper for a vtkSMPropertyIterator class to satisfy
       the python iterator protocol. Note that the list of
       properties can also be obtained from the class object's
       dictionary.
       See the doxygen documentation for vtkSMPropertyIterator C++
       class for details.
       """
    
    def __init__(self, aProxy):
        self.SMIterator = aProxy.NewPropertyIterator()
	if self.SMIterator:
            self.SMIterator.UnRegister(None)
            self.SMIterator.Begin()
        self.Key = None
        self.Proxy = aProxy

    def __iter__(self):
        return self

    def next(self):
	if not self.SMIterator:
            raise StopIteration

        if self.SMIterator.IsAtEnd():
            self.Key = None
            raise StopIteration
        self.Key = self.SMIterator.GetKey()
        self.SMIterator.Next()
        return self.Proxy.GetProperty(self.Key)

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
        return self.Proxy.GetProperty(self.Key)

    def __getattr__(self, name):
        """returns attributes from the vtkSMProxyIterator."""
        return getattr(self.SMIterator, name)

class ProxyDefinitionIterator(object):
    """Wrapper for a vtkSMProxyDefinitionIterator class to satisfy
       the python iterator protocol.
       See the doxygen documentation of the vtkSMProxyDefinitionIterator
       C++ class for more information."""
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
     python iterator protocol.
     See the doxygen documentation of vtkSMProxyIterator C++ class for
     more information.
     """
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
        self.AProxy = _getPyProxy(self.SMIterator.GetProxy())
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
      This is a python representation for a connection.
    """
    def __init__(self, connectionId):
        """Default constructor. Creates a Connection with the given
        ID, all other data members initialized to None."""
        self.ID = connectionId
        self.Hostname = ""
        self.Port = 0
        self.RSHostname = None
        self.RSPort = None
        return

    def __eq__(self, other):
        "Returns true if the connection ids are the same."
        return self.ID == other.ID
    
    def SetHost(self, ds_host, ds_port, rs_host=None, rs_port=None):
        """Set the hostname of a given connection. Used by Connect()."""
        self.Hostname = ds_host 
        self.Port = ds_port
        self.RSHostname = rs_host
        self.RSPort = rs_port
        return

    def __repr__(self):
        """User friendly string representation"""
        if not self.RSHostname:
            return "Connection (%s:%d)" % (self.Hostname, self.Port)
        return "Connection data(%s:%d), render(%s:%d)" % \
            (self.Hostname, self.Port, self.RSHostname, self.RSPort)

    def IsRemote(self):
        """Returns True if the connection to a remote server, False if
        it is local (built-in)"""
        pm = vtkProcessModule.GetProcessModule()
        if pm.IsRemote(self.ID):
            return True
        return False

    def GetNumberOfDataPartitions(self):
        """Returns the number of partitions on the data server for this
           connection"""
        pm = vtkProcessModule.GetProcessModule()
        return pm.GetNumberOfPartitions(self.ID);


## These are methods to create a new connection.
## One can connect to a server, (data-server,render-server)
## or simply create a built-in connection.
## Note: these are internal methods. Use Connect() instead.
def _connectServer(host, port, rc=False):
    """Connect to a host:port. Returns the connection object if successfully
    connected with the server. Internal method, use Connect() instead."""
    pm =  vtkProcessModule.GetProcessModule()
    if not rc:
        cid = pm.ConnectToRemote(host, port)
        if not cid:
            return None
        conn = Connection(cid)
    else:
        pm.AcceptConnectionsOnPort(port)
        print "Waiting for connection..."
        while True:
            cid = pm.MonitorConnections(10)
            if cid > 0:
                conn = Connection(cid)
                break
        pm.StopAcceptingAllConnections()
    conn.SetHost(host, port)
    return conn 

def _connectDsRs(ds_host, ds_port, rs_host, rs_port):
    """Connect to a dataserver at (ds_host:ds_port) and to a render server
    at (rs_host:rs_port). 
    Returns the connection object if successfully connected 
    with the server. Internal method, use Connect() instead."""
    pm =  vtkProcessModule.GetProcessModule()
    cid = pm.ConnectToRemote(ds_host, ds_port, rs_host, rs_port)
    if not cid:
        return None
    conn = Connection(cid)
    conn.SetHost(ds_host, ds_port, rs_host, rs_port)
    return conn 

def _connectSelf():
    """Creates a new self connection.Internal method, use Connect() instead."""
    pm =  vtkProcessModule.GetProcessModule()
    pmOptions = pm.GetOptions()
    if pmOptions.GetProcessType() == 0x40: # PVBATCH
        return Connection(vtkProcessModuleConnectionManager.GetRootServerConnectionID())        
    cid = pm.ConnectToSelf()
    if not cid:
        return None
    conn = Connection(cid)
    conn.SetHost("builtin", cid)
    return conn

def SaveState(filename):
    """Given a state filename, saves the state of objects registered
    with the proxy manager."""
    pm = ProxyManager()
    pm.SaveState(filename)

def LoadState(filename, connection=None):
    """Given a state filename and an optional connection, loads the server
    manager state."""
    if not connection:
        connection = ActiveConnection
    if not connection:
        raise exceptions.RuntimeError, "Cannot load state without a connection"
    loader = vtkSMPQStateLoader()
    pm = ProxyManager()
    pm.LoadState(filename, ActiveConnection.ID, loader)
    views = GetRenderViews()
    for view in views:
        # Make sure that the client window size matches the
        # ViewSize property. In paraview, the GUI takes care
        # of this.
        if view.GetClassName() == "vtkSMIceTDesktopRenderViewProxy":
            view.GetRenderWindow().SetSize(view.ViewSize[0], \
                                           view.ViewSize[1])
    
def Connect(ds_host=None, ds_port=11111, rs_host=None, rs_port=11111):
    """
    Use this function call to create a new connection. On success,
    it returns a Connection object that abstracts the connection.
    Otherwise, it returns None.
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
    global ActiveConnection
    global fromGUI
    if fromGUI:
        raise exceptions.RuntimeError, "Cannot create a connection through python. Use the GUI to setup the connection."
    if ds_host == None:
        connectionId = _connectSelf()
    elif rs_host == None:
        connectionId = _connectServer(ds_host, ds_port)
    else:
        connectionId = _connectDsRs(ds_host, ds_port, rs_host, rs_port)
    if not ActiveConnection:
        ActiveConnection = connectionId
    return connectionId

def ReverseConnect(port=11111):
    """
    Use this function call to create a new connection. On success,
    it returns a Connection object that abstracts the connection.
    Otherwise, it returns None.
    In reverse connection mode, the client waits for a connection
    from the server (client has to be started first). The server
    then connects to the client (run pvserver with -rc and -ch
    option).
    The optional port specified the port to listen to.
    """
    global ActiveConnection
    global fromGUI
    if fromGUI:
        raise exceptions.RuntimeError, "Cannot create a connection through python. Use the GUI to setup the connection."
    connectionId = _connectServer("Reverse connection", port, True)
    if not ActiveConnection:
        ActiveConnection = connectionId
    return connectionId

def Disconnect(connection=None):
    """Disconnects the connection. Make sure to clear the proxy manager
    first."""
    global ActiveConnection
    global fromGUI
    if fromGUI:
        raise exceptions.RuntimeError, "Cannot disconnect through python. Use the GUI to disconnect."
    if not connection or connection == ActiveConnection:
        connection = ActiveConnection
        ActiveConnection = None
    pm =  vtkProcessModule.GetProcessModule()
    pm.Disconnect(connection.ID)
    return

def CreateProxy(xml_group, xml_name, connection=None):
    """Creates a proxy. If connection is set, the proxy's connection ID is
    set accordingly. If connection is None, ActiveConnection is used, if
    present. You should not have to use method normally. Instantiate the
    appropriate class from the appropriate module, for example:
    sph = servermanager.sources.SphereSource()"""

    pxm = ProxyManager()
    aProxy = pxm.NewProxy(xml_group, xml_name)
    if not aProxy:
        return None
    if not connection:
        connection = ActiveConnection
    if connection:
        aProxy.SetConnectionID(connection.ID)
    return aProxy

def GetRenderView(connection=None):
    """Return the render view in use.  If more than one render view is in
    use, return the first one."""

    if not connection:
        connection = ActiveConnection
    render_module = None
    for aProxy in ProxyManager().NewConnectionIterator(connection):
        if aProxy.IsA("vtkSMRenderViewProxy"):
            render_module = aProxy
            break
    return render_module

def GetRenderViews(connection=None):
    """Returns the set of all render views."""
    
    if not connection:
        connection = ActiveConnection
    render_modules = []
    for aProxy in ProxyManager().NewConnectionIterator(connection):
        if aProxy.IsA("vtkSMRenderViewProxy"):
            render_modules.append(aProxy)
    return render_modules

def CreateRenderView(connection=None, **extraArgs):
    """Creates a render window on the particular connection. If connection
    is not specified, then the active connection is used, if available.

    This method can also be used to initialize properties by passing
    keyword arguments where the key is the name of the property.In addition
    registrationGroup and registrationName (optional) can be specified (as
    keyword arguments) to automatically register the proxy with the proxy
    manager."""

    if not connection:
        connection = ActiveConnection
    if not connection:
        raise exceptions.RuntimeError, "Cannot create render window without connection."
    pxm = ProxyManager()
    prototype = pxm.GetPrototypeProxy("views", "RenderView")

    proxy_xml_name = prototype.GetSuggestedViewType(connection.ID)
    ren_module = None
    if proxy_xml_name:
        ren_module = CreateProxy("views", proxy_xml_name, connection)
    if not ren_module:
        return None
    extraArgs['proxy'] = ren_module
    proxy = rendering.__dict__[ren_module.GetXMLName()](**extraArgs)
    return proxy

def CreateRepresentation(aProxy, view, **extraArgs):
    """Creates a representation for the proxy and adds it to the render
    module.

    This method can also be used to initialize properties by passing
    keyword arguments where the key is the name of the property.In addition
    registrationGroup and registrationName (optional) can be specified (as
    keyword arguments) to automatically register the proxy with the proxy
    manager.
    
    This method tries to create the best possible representation for the given
    proxy in the given view. Additionally, the user can specify proxyName
    (optional) to create a representation of a particular type."""

    global rendering
    if not aProxy:
        raise exceptions.RuntimeError, "proxy argument cannot be None."
    if not view:
        raise exceptions.RuntimeError, "render module argument cannot be None."
    if "proxyName" in extraArgs:
      display = CreateProxy("representations", extraArgs['proxyName'], None)
      del extraArgs['proxyName']
    else:
      display = view.SMProxy.CreateDefaultRepresentation(aProxy.SMProxy, 0)
      if display:
        display.UnRegister(None)
    if not display:
        return None
    display.SetConnectionID(aProxy.GetConnectionID())
    extraArgs['proxy'] = display
    proxy = rendering.__dict__[display.GetXMLName()](**extraArgs)
    proxy.Input = aProxy
    proxy.UpdateVTKObjects()
    view.Representations.append(proxy)
    return proxy

def LoadPlugin(filename, connection=None):
    """ Given a filename and a connection (optional, otherwise uses
    ActiveConnection), loads a plugin. It then updates the sources,
    filters and rendering modules."""
    
    if not connection:
        connection = ActiveConnection
    if not connection:
        raise exceptions.RuntimeError, "Cannot load a plugin without a connection."
    
    pxm=ProxyManager()
    pld = pxm.NewProxy("misc", "PluginLoader")
    pld.GetProperty("FileName").SetElement(0, filename)
    pld.UpdateVTKObjects()
    pld.UpdatePropertyInformation()
    # Make sure that the plugin was loaded successfully
    if pld.GetProperty("Loaded").GetElement(0):
        # Get the XML, parse it and load it
        xmlstring = pld.GetProperty("ServerManagerXML").GetElement(0)
        if xmlstring:
            parser = vtkSMXMLParser()
            parser.Parse(xmlstring)
            parser.ProcessConfiguration(vtkSMObject.GetProxyManager())
            # Update the modules
            updateModules()
            

def Fetch(input, arg1=None, arg2=None):
    """ 
    A convenience method that moves data from the server to the client, 
    optionally performing some operation on the data as it moves.
    The input argument is the name of the (proxy for a) source or filter
    whose output is needed on the client.
    
    You can use Fetch to do three things:

    If arg1 is None (the default) then all of the data is brought to the client.
    In parallel runs an appropriate append Filter merges the
    data on each processor into one data object. The filter chosen will be 
    vtkAppendPolyData for vtkPolyData, vtkAppendRectilinearGrid for 
    vtkRectilinearGrid, vtkMultiBlockDataGroupFilter for vtkCompositeData, 
    and vtkAppendFilter for anything else.
    
    If arg1 is an integer then one particular processor's output is brought to
    the client. In serial runs the arg is ignored. If you have a filter that
    computes results in parallel and brings them to the root node, then set 
    arg to be 0.
    
    If arg1 and arg2 are a algorithms, for example vtkMinMax, the algorithm
    will be applied to the data to obtain some result. Here arg1 will be
    applied pre-gather and arg2 will be applied post-gather. In parallel
    runs the algorithm will be run on each processor to make intermediate
    results and then again on the root processor over all of the
    intermediate results to create a global result.  """

    import types

    #create the pipeline that reduces and transmits the data
    gvd = rendering.ClientDeliveryRepresentationBase()
    gvd.AddInput(input, "DONTCARE") 
  
    if arg1 == None:
        print "getting appended"

        cdinfo = input.GetDataInformation().GetCompositeDataInformation()
        if cdinfo.GetDataIsComposite():
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

        
    elif type(arg1) is types.IntType:          
        print "getting node %d" % arg1
        gvd.SetReductionType(3)  
        gvd.SetPreGatherHelper(None)
        gvd.SetPostGatherHelper(None)
        gvd.SetPassThrough(arg1)

    else:
        print "applying operation"
        gvd.SetReductionType(6) # CUSTOM   
        gvd.SetPreGatherHelper(arg1)
        gvd.SetPostGatherHelper(arg2)
        gvd.SetPassThrough(-1)

    #go!
    gvd.UpdateVTKObjects()
    gvd.Update()   
    return gvd.GetOutput()

def AnimateReader(reader, view, filename=None):
    """This is a utility function that, given a reader and a view
    animates over all time steps of the reader. If the optional
    filename is provided, a movie is created (type depends on the
    extension of the filename."""
    # Create an animation scene
    scene = animation.AnimationScene();
    # with 1 view
    scene.ViewModules = [view];
    # Update the reader to get the time information
    reader.UpdatePipelineInformation()
    scene.TimeSteps = reader.TimestepValues.GetData()
    # Animate from 1st time step to last
    scene.StartTime = reader.TimestepValues.GetData()[0]
    scene.EndTime = reader.TimestepValues.GetData()[-1];

    # Each frame will correspond to a time step
    scene.PlayMode = 2; #Snap To Timesteps

    # Create a special animation cue for time.
    cue = animation.TimeAnimationCue();
    cue.AnimatedProxy = view;
    cue.AnimatedPropertyName = "ViewTime";
    scene.Cues = [cue];

    if filename:
        writer = vtkSMAnimationSceneImageWriter();
        writer.SetFileName(filename);
        writer.SetFrameRate(1);
        writer.SetAnimationScene(scene.SMProxy);
        
        # Now save the animation.
        if not writer.Save():
            raise exceptions.RuntimeError, "Saving of animation failed!"
    else:
        scene.Play()

def ToggleProgressPrinting():
    """Turn on/off printing of progress (by default, it is on). You can
    always turn progress off and add your own observer to the process
    module to handle progress in a custom way. See _printProgress for
    an example event observer."""
    global progressObserverTag

    if fromGUI:
        raise exceptions.RuntimeError, "Printing progress in the GUI is not supported."
    if progressObserverTag:
        vtkProcessModule.GetProcessModule().RemoveObserver(progressObserverTag)
        progressObserverTag = None
    else:
        progressObserverTag = vtkProcessModule.GetProcessModule().AddObserver(\
             "ProgressEvent", _printProgress)
    
def Finalize():
    """Although not required, this can be called at exit to cleanup."""
    global progressObserverTag
    # Make sure to remove the observer
    if progressObserverTag:
        ToggleProgressPrinting()
    vtkInitializationHelper.Finalize()
    
# Internal methods

def _getPyProxy(smproxy):
    """Returns a python wrapper for a server manager proxy. This method
    first checks if there is already such an object by looking in the
    _pyproxies group and returns it if found. Otherwise, it creates a
    new one. Proxies register themselves in _pyproxies upon creation."""
    if not smproxy:
        return None
    if smproxy in _pyproxies:
        retVal = _pyproxies[smproxy]()
        if retVal:
            return retVal
    
    classForProxy = _findClassForProxy(smproxy.GetXMLName())
    if classForProxy:
        retVal = classForProxy(proxy=smproxy)
    else:
        retVal = Proxy(proxy=smproxy)
    return retVal
    
def _makeUpdateCameraMethod(rv):
    """ This internal method is used to create observer methods """
    rvref = rv
    rvref().BlockUpdateCamera = False
    def UpdateCamera(obj, string):
        if not rvref().BlockUpdateCamera:
          # used to avoid some nasty recursion that occurs when interacting in
          # the GUI.
          rvref().BlockUpdateCamera = True
          rvref().SynchronizeCameraProperties()
          rvref().BlockUpdateCamera = False
    return UpdateCamera

def _createInitialize(group, name):
    """Internal method to create an Initialize() method for the sub-classes
    of Proxy"""
    pgroup = group
    pname = name
    def aInitialize(self, connection=None):
        if not connection:
            connection = ActiveConnection
        if not connection:
            raise exceptions.RuntimeError,\
                  'Cannot create a proxy without a connection.'
        self.InitializeFromProxy(\
            CreateProxy(pgroup, pname, connection))
    return aInitialize

def _createGetProperty(pName):
    """Internal method to create a GetXXX() method where XXX == pName."""
    propName = pName
    def getProperty(self):
        return self.GetProperty(propName)
    return getProperty

def _createSetProperty(pName):
    """Internal method to create a SetXXX() method where XXX == pName."""
    propName = pName
    def setProperty(self, value):
        return self.SetPropertyWithName(propName, value)
    return setProperty

def _findClassForProxy(xmlName):
    """Given the xmlName for a proxy, returns a Proxy class. Note
    that if there are duplicates, the first one is returned."""
    global sources, filters
    if xmlName in sources.__dict__:
        return sources.__dict__[xmlName]
    elif xmlName in filters.__dict__:
        return filters.__dict__[xmlName]
    elif xmlName in rendering.__dict__:
        return rendering.__dict__[xmlName]
    elif xmlName in animation.__dict__:
        return animation.__dict__[xmlName]
    else:
        return None

def _printProgress(caller, event):
    """The default event handler for progress. Prints algorithm
    name and 1 '.' per 10% progress."""
    global currentAlgorithm, currentProgress
    
    pm = vtkProcessModule.GetProcessModule()
    progress = pm.GetLastProgress() / 10
    alg = pm.GetLastProgressName()
    if alg != currentAlgorithm and alg:
        if currentAlgorithm:
            while currentProgress <= 10:
                import sys
                sys.stdout.write(".")
                currentProgress += 1
            print "]"
            currentProgress = 0
        print alg, ": [ ",
        currentAlgorithm = alg
    while currentProgress <= progress:
        import sys
        sys.stdout.write(".")
        #sys.stdout.write("%d " % pm.GetLastProgress())
        currentProgress += 1
    if progress == 10:
        print "]"
        currentAlgorithm = None
        currentProgress = 0
            
def updateModules():
    """Called when a plugin is loaded, this method updates
    the proxy class object in all known modules."""
    global sources, filters, writers, rendering, animation, implicit_functions

    createModule("sources", sources)
    createModule("filters", filters)
    createModule("writers", writers)
    createModule("representations", rendering)
    createModule("views", rendering)
    createModule("lookup_tables", rendering)
    createModule("textures", rendering)
    createModule("animation", animation)
    createModule('animation_keyframes', animation)
    createModule('implicit_functions', implicit_functions)
    
def _createModules():
    """Called when the module is loaded, this creates sub-
    modules for all know proxy groups."""
    global sources, filters, writers, rendering, animation, implicit_functions

    sources = createModule('sources')
    filters = createModule('filters')
    writers = createModule('writers')
    rendering = createModule('representations')
    createModule('views', rendering)
    createModule("lookup_tables", rendering)
    createModule("textures", rendering)
    animation = createModule('animation')
    createModule('animation_keyframes', animation)
    implicit_functions = createModule('implicit_functions')
    
def createModule(groupName, mdl=None):
    """Populates a module with proxy classes defined in the given group.
    If mdl is not specified, it also creates the module"""
    pxm = vtkSMObject.GetProxyManager()
    # Use prototypes to find all proxy types.
    pxm.InstantiateGroupPrototypes(groupName)

    debug = False
    if not mdl:
        debug = True
        mdl = new.module(groupName)
    numProxies = pxm.GetNumberOfXMLProxies(groupName)
    for i in range(numProxies):
        pname = pxm.GetXMLProxyName(groupName, i)
        if pname in mdl.__dict__:
            continue
        cdict = {}
        # Create an Initialize() method for this sub-class.
        cdict['Initialize'] = _createInitialize(groupName, pname)
        proto = pxm.GetPrototypeProxy(groupName, pname)
        iter = PropertyIterator(proto)
        # Add all properties as python properties.
        for prop in iter:
            propName = iter.GetKey()
            propDoc = None
            if prop.GetDocumentation():
                propDoc = prop.GetDocumentation().GetDescription()
            cdict[propName] = property(_createGetProperty(propName),
                                       _createSetProperty(propName),
                                       None,
                                       propDoc)
        # Add the documentation as the class __doc__
        if proto.GetDocumentation() and \
           proto.GetDocumentation().GetDescription():
            doc = proto.GetDocumentation().GetDescription()
        else:
            doc = Proxy.__doc__
        cdict['__doc__'] = doc
        # Create the new type
        if proto.IsA("vtkSMSourceProxy"):
            cobj = type(pname, (SourceProxy,), cdict)
        else:
            cobj = type(pname, (Proxy,), cdict)            
        # Add it to the modules dictionary
        mdl.__dict__[pname] = cobj
    return mdl


def __determineGroup(proxy):
    """Internal method"""
    if not proxy:
        return None
    xmlgroup = proxy.GetXMLGroup()
    xmlname = proxy.GetXMLName()
    if xmlgroup == "sources":
        return "sources"
    elif xmlgroup == "filters":
        return "sources"
    elif xmlgroup == "views":
        return "views"
    elif xmlgroup == "representations":
        if xmlname == "ScalarBarWidgetRepresentation":
            return "scalar_bars"
        return "representations";
    elif xmlgroup == "lookup_tables":
        return "lookup_tables"
    return None

__nameCounter = 0
def __determineName(proxy, group):
    global __nameCounter
    __nameCounter += 1
    return "%s%d" % (proxy.GetXMLName(), __nameCounter)

def __getName(proxy, group):
    pxm = ProxyManager()
    if isinstance(proxy, Proxy):
        proxy = proxy.SMProxy
    return pxm.GetProxyName(group, proxy)

def Register(proxy, **extraArgs):
    """Registers a proxy with the proxy manager. If no 'registrationGroup' is
    specified, then the group is inferred from the type of the proxy.
    'registrationName' may be specified to register with a particular name
    otherwise a default name will be created."""
    # TODO: handle duplicate registration
    if "registrationGroup" in extraArgs:
        registrationGroup = extraArgs["registrationGroup"]
    else:
        registrationGroup = __determineGroup(proxy)
        
    if "registrationName" in extraArgs:
        registrationName = extraArgs["registrationName"]
    else:
        registrationName = __determineName(proxy, registrationGroup)
    if registrationGroup and registrationName:
        pxm = ProxyManager()
        pxm.RegisterProxy(registrationGroup, registrationName, proxy)
    else:
        raise exceptions.RuntimeError, "Registeration error."
    return (registrationGroup, registrationName);

def UnRegister(proxy, **extraArgs):
    """UnRegisters proxies registered using Register()."""
    if "registrationGroup" in extraArgs:
        registrationGroup = extraArgs["registrationGroup"]
    else:
        registrationGroup = __determineGroup(proxy)
        
    if "registrationName" in extraArgs:
        registrationName = extraArgs["registrationName"]
    else:
        registrationName = __getName(proxy, registrationGroup)

    if registrationGroup and registrationName:
        pxm = ProxyManager()
        pxm.UnRegisterProxy(registrationGroup, registrationName, proxy)
    else:
        raise exceptions.RuntimeError, "UnRegisteration error."
    return (registrationGroup, registrationName);

def demo1():
    """This simple demonstration creates a sphere, renders it and delivers
    it to the client using Fetch. It returns a tuple of (data, render
    view)"""
    if not ActiveConnection:
        Connect()
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

    return (data, rv)

def demo2(fname="/Users/berk/Work/ParaViewData/Data/disk_out_ref.ex2"):
    """This method demonstrates the user of a reader, representation and
    view. It also demonstrates how meta-data can be obtained using proxies.
    Make sure to pass the full path to an exodus file. Also note that certain
    parameters are hard-coded for disk_out_ref.ex2 which can be found
    in ParaViewData. This method returns the render view."""
    if not ActiveConnection:
        Connect()
    # Create the exodus reader and specify a file name
    reader = sources.ExodusIIReader(FileName=fname)
    # Update information to force the reader to read the meta-data
    # including the list of variables available
    reader.UpdatePipelineInformation()
    # Get the list of point arrays. This is a list of pairs; each pair
    # contains the name of the array and whether that array is to be
    # loaded
    arrayList = reader.PointResultArrayInfo
    print arrayList.GetData()
    # Assign the list to the property that determines which arrays are read.
    # See the documentation for the exodus reader for details.
    reader.PointResultArrayStatus = arrayList.GetData()
    arraystatus = reader.PointResultArrayStatus
    # Flip the read flag of all arrays
    for idx in range(1, len(arraystatus), 2):
        arraystatus[idx] = "1"
    print arraystatus.GetData()
    # Next create a default render view appropriate for the connection type.
    rv = CreateRenderView()
    # Create the matching representation
    rep = CreateRepresentation(reader, rv)
    rep.Representation = 1 # Wireframe
    # Black background is not pretty
    rv.Background = [0.4, 0.4, 0.6]
    rv.StillRender()
    # Reset the camera to include the whole thing
    rv.ResetCamera()
    rv.StillRender()
    # Change the elevation of the camera. See VTK documentation of vtkCamera
    # for camera parameters.
    c = rv.GetActiveCamera()
    c.Elevation(45)
    rv.StillRender()
    # Now that the reader execute, let's get some information about it's
    # output.
    di = reader.GetDataInformation()
    pdi = di.GetPointDataInformation()
    # This prints a list of all read point data arrays as well as their
    # value ranges.
    print 'Number of point arrays:', pdi.GetNumberOfArrays()
    for i in range(pdi.GetNumberOfArrays()):
        ai = pdi.GetArrayInformation(i)
        print "----------------"
        print "Array:", i, ai.GetName(), ":"
        numComps = ai.GetNumberOfComponents()
        print "Number of components:", numComps
        for j in range(numComps):
            print "Range:", ai.GetComponentRange(j)
    # White is boring. Let's color the geometry using a variable.
    # First create a lookup table. This object controls how scalar
    # values are mapped to colors. See VTK documentation for
    # details.
    lt = rendering.PVLookupTable()
    # Assign it to the representation
    rep.LookupTable = lt
    # Color by point array called Pres
    rep.ColorAttributeType = 0 # point data
    rep.ColorArrayName = "Pres"
    # Add to RGB points. These are tuples of 4 values. First one is
    # the scalar values, the other 3 the RGB values. This list has
    # 2 points: Pres: 0.00678, color: blue, Pres: 0.0288, color: red
    lt.RGBPoints = [0.00678, 0, 0, 1, 0.0288, 1, 0, 0]
    lt.ColorSpace = 1 # HSV
    rv.StillRender()
    return rv

def demo3():
    """This method demonstrates the use of servermanager with numpy as
    well as pylab for plotting. It creates an artificial data sources,
    probes it with a line, delivers the result to the client using Fetch
    and plots it using pylab. This demo requires numpy and pylab installed.
    It returns a tuple of (data, render view)."""
    import paraview.numeric
    import pylab
    
    if not ActiveConnection:
        Connect()
    # Create a synthetic data source
    source = sources.RTAnalyticSource()
    # Let's get some information about the data. First, for the
    # source to execute
    source.UpdatePipeline()

    di = source.GetDataInformation()
    print "Data type:", di.GetPrettyDataTypeString()
    print "Extent:", di.GetExtent()
    print "Array name:", \
          di.GetPointDataInformation().GetArrayInformation(0).GetName()

    rv = CreateRenderView()

    rep1 = CreateRepresentation(source, rv)
    rep1.Representation = 3 # outline

    # Let's apply a contour filter
    cf = filters.Contour(Input=source, ContourValues=[200])
    # This is probably one of the most difficult properties to set.
    # Look at the documentation of vtkAlgorithm::SetInputArrayToProcess()
    # for details.
    cf.SelectInputScalars = ['0', '0', '0', '0', 'RTData']

    rep2 = CreateRepresentation(cf, rv)
    
    rv.Background = (0.4, 0.4, 0.6)
    # Reset the camera to include the whole thing
    rv.StillRender()
    rv.ResetCamera()
    rv.StillRender()

    # Now, let's probe the data
    probe = filters.Probe(Input=source)
    # with a line
    line = sources.LineSource(Resolution=60)
    # that spans the dataset
    bounds = di.GetBounds()
    print "Bounds: ", bounds
    line.Point1 = bounds[0:6:2]
    line.Point2 = bounds[1:6:2]

    probe.Source = line

    # Render with the line
    rep3 = CreateRepresentation(line, rv)
    rv.StillRender()

    # Now deliver it to the client. Remember, this is for small data.
    data = Fetch(probe)
    # Convert it to a numpy array
    data = paraview.numeric.getarray(data.GetPointData().GetArray(0))
    # Plot it using matplotlib
    pylab.plot(data)
    pylab.show()
    
    return (data, rv)
    
def demo4(fname="/Users/berk/Work/ParaViewData/Data/can.ex2"):
    """This method demonstrates the user of AnimateReader for
    creating animations."""
    if not ActiveConnection:
        Connect()
    reader = sources.ExodusIIReader(FileName=fname)
    view = CreateRenderView()
    repr = CreateRepresentation(reader, view)
    view.StillRender();
    view.ResetCamera();
    view.StillRender();
    c = view.GetActiveCamera()
    c.Elevation(95)
    AnimateReader(reader, view)


def demo5():
    """ Simple sphere animation"""
    if not ActiveConnection:
        Connect()
    sphere = sources.SphereSource();
    view = CreateRenderView();
    repr = CreateRepresentation(sphere, view);

    view.StillRender();
    view.ResetCamera();
    view.StillRender();

    # Create an animation scene
    scene = animation.AnimationScene();
    # Add 1 view
    scene.ViewModules = [view];

    # Create a cue to animate the StartTheta property
    cue = animation.KeyFrameAnimationCue();
    cue.AnimatedProxy = sphere;
    cue.AnimatedPropertyName = "StartTheta";
    # Add it to the scene's cues
    scene.Cues = [cue];

    # Create 2 keyframes for the StartTheta track
    keyf0 = animation.CompositeKeyFrame();
    keyf0.Type = 2 ;# Set keyframe interpolation type to Ramp.
    # At time = 0, value = 0
    keyf0.KeyTime = 0;
    keyf0.KeyValues= [0];

    keyf1 = animation.CompositeKeyFrame();
    # At time = 1.0, value = 200
    keyf1.KeyTime = 1.0;
    keyf1.KeyValues= [200];

    # Add keyframes.
    cue.KeyFrames = [keyf0, keyf1];

    scene.Play()
    return scene;

# Users can set the active connection which will be used by API
# to create proxies etc when no connection argument is passed.
# Connect() automatically sets this if it is not already set.
ActiveConnection = None

# Needs to be called when paraview module is loaded from python instead
# of pvpython, pvbatch or GUI.
if not vtkSMObject.GetProxyManager():
    vtkInitializationHelper.Initialize(sys.executable)

# Initialize progress printing. Can be turned off by calling
# ToggleProgressPrinting() again.
progressObserverTag = None
currentAlgorithm = False
currentProgress = 0
fromGUI = False
ToggleProgressPrinting()

_pyproxies = {}

# Create needed sub-modules
_createModules()

