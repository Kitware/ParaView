# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause

r"""servermanager is a module for using paraview server manager in Python.
One can always use the server manager API directly. However, this module
provides an interface easier to use from Python by wrapping several VTK
classes around Python classes.

Note that, upon load, this module will create several sub-modules: sources,
filters and rendering. These modules can be used to instantiate specific
proxy types. For a list, try "dir(servermanager.sources)"

Usually users should use the paraview.simple module instead as it provide a
more user friendly API.

A simple example::

.. code:: python

    from paraview.servermanager import *

    # Creates a new built-in session and makes it the active session.
    Connect()

    # Creates a new render view on the active session.
    renModule = CreateRenderView()

    # Create a new sphere proxy on the active session and register it
    # in the sources group.
    sphere = sources.SphereSource(registrationGroup="sources", ThetaResolution=16, PhiResolution=32)

    # Create a representation for the sphere proxy and adds it to the render
    # module.
    display = CreateRepresentation(sphere, renModule)

    renModule.StillRender()

"""
from __future__ import print_function
import paraview, re, os, os.path, types, sys

# prefer `vtk` from `paraview` since it doesn't import all
# vtk modules.
from paraview import vtk, print_warning
from paraview import _backwardscompatibilityhelper as _bc
from paraview.util import proxy as proxy_util

from paraview.modules.vtkPVVTKExtensionsCore import *
from paraview.modules.vtkRemotingCore import *
from paraview.modules.vtkRemotingServerManager import *
from paraview.modules.vtkRemotingSettings import *
from paraview.modules.vtkRemotingApplication import *

try:
    from paraview.modules.vtkRemotingViews import *
except ImportError:
    pass

try:
    from paraview.modules.vtkRemotingAnimation import *
except ImportError:
    pass

try:
    from paraview.modules.vtkRemotingExport import *
except ImportError:
    pass

try:
    from paraview.modules.vtkRemotingImport import *
except ImportError:
    pass

try:
    from paraview.modules.vtkRemotingMisc import *
except ImportError:
    pass

try:
    from paraview.modules.vtkRemotingLive import *
except ImportError:
    pass


def _wrap_property(proxy, smproperty):
    """ Internal function.
    Given a server manager property and its domains, returns the
    appropriate python object.
    """
    property = None
    if paraview.compatibility.GetVersion() >= (3, 5) and \
            smproperty.IsA("vtkSMStringVectorProperty"):
        arraySelectionDomain = smproperty.FindDomain("vtkSMArraySelectionDomain")
        chartSeriesSelectionDomain = smproperty.FindDomain("vtkSMChartSeriesSelectionDomain")
        arrayListDomain = smproperty.FindDomain("vtkSMArrayListDomain")
        fileListDomain = smproperty.FindDomain("vtkSMFileListDomain")
        stringListDomain = smproperty.FindDomain("vtkSMStringListDomain")
        if arraySelectionDomain and smproperty.GetRepeatable():
            property = ArrayListProperty(proxy, smproperty)
        elif chartSeriesSelectionDomain and smproperty.GetRepeatable() and \
                chartSeriesSelectionDomain.GetDefaultMode() == 1:
            property = ArrayListProperty(proxy, smproperty)
        elif arrayListDomain and smproperty.GetRepeatable():
            # if it is repeatable, then it is not a single array selection... and if it happens
            # to have 5 elements in the repeatable proxy, avoid an exception by testing this case
            # first.
            property = VectorProperty(proxy, smproperty)
        elif arrayListDomain and smproperty.GetNumberOfElements() == 5:
            property = ArraySelectionProperty(proxy, smproperty)
        elif fileListDomain and fileListDomain.GetIsOptional() == 0:
            # Refer to BUG #9710 to see why optional domains need to be ignored.
            property = FileNameProperty(proxy, smproperty)
        elif stringListDomain:
            property = StringListProperty(proxy, smproperty)
        else:
            property = VectorProperty(proxy, smproperty)
    elif smproperty.IsA("vtkSMVectorProperty"):
        if smproperty.IsA("vtkSMIntVectorProperty") and \
                smproperty.FindDomain("vtkSMEnumerationDomain"):
            property = EnumerationProperty(proxy, smproperty)
        else:
            property = VectorProperty(proxy, smproperty)
    elif smproperty.IsA("vtkSMInputProperty"):
        property = InputProperty(proxy, smproperty)
    elif smproperty.IsA("vtkSMProxyProperty"):
        property = ProxyProperty(proxy, smproperty)
    elif smproperty.IsA("vtkSMDoubleMapProperty"):
        property = DoubleMapProperty(proxy, smproperty)
    else:
        property = Property(proxy, smproperty)
    return property


class ParaViewPipelineController(object):
    """ParaViewPipelineController wraps vtkSMParaViewPipelineController class
    to manage conversion of arguments passed around from Python Proxy objects to
    vtkSMProxy instances are vice-versa."""

    def __init__(self):
        """Constructor. Creates a new instance of
        vtkSMParaViewPipelineController."""
        self.SMController = vtkSMParaViewPipelineController()

    def __ConvertArgumentsAndCall(self, *args):
        newArgs = []
        for arg in args:
            # convert Proxy and ProxyManager to vtkSMProxy and
            # vtkSMSessionProxyManager instances.
            # FIXME: should handle session as well?
            if issubclass(type(arg), Proxy) or isinstance(arg, Proxy):
                newArgs.append(arg.SMProxy)
            elif issubclass(type(arg), ProxyManager) or isinstance(arg, ProxyManager):
                newArgs.append(arg.SMProxyManager)
            else:
                newArgs.append(arg)
        func = getattr(self.SMController, self.__LastAttrName)
        retVal = func(*newArgs)
        if isinstance(retVal, vtkSMProxy):
            # if this is a vtkObject and is a "vtkSMProxy", return a Proxy().
            return _getPyProxy(retVal)
        else:
            return retVal

    def __getattr__(self, name):
        """Returns attribute from the ParaViewPipelineController."""
        try:
            pmAttr = getattr(self.SMController, name)
            self.__LastAttrName = name
            return self.__ConvertArgumentsAndCall
        except:
            pass
        return getattr(self.SMController, name)


class Proxy(object):
    """Proxy for a server side object. A proxy manages the lifetime of
    one or more server manager objects. It also provides an interface
    to set and get the properties of the server side objects. These
    properties are presented as Python properties. For example,
    you can set a property Foo using the following::

       proxy.Foo = (1,2)

    or::

       proxy.Foo.SetData((1,2))

    or::

       proxy.Foo[0:2] = (1,2)

    For more information, see the documentation of the property which
    you can obtain with::

      help(proxy.Foo).

    This class also provides an iterator which can be used to iterate
    over all properties, e.g.::

        proxy = Proxy(proxy=smproxy)
        for property in proxy:
            print (property)

    For advanced users:
    This is a python class that wraps a vtkSMProxy. Makes it easier to
    set/get properties.
    Instead of::

        proxy.GetProperty("Foo").SetElement(0, 1)
        proxy.GetProperty("Foo").SetElement(0, 2)

    you can do::

        proxy.Foo = (1,2)

    or::

        proxy.Foo.SetData((1,2))

    or::

        proxy.Foo[0:2] = (1,2)

    Instead of::

        proxy.GetProperty("Foo").GetElement(0)

    you can do::

        proxy.Foo.GetData()[0]

    or::

        proxy.Foo[0]

    For proxy properties, you can use append::

        proxy.GetProperty("Bar").AddProxy(foo)

    you can do::

        proxy.Bar.append(foo)

    Properties support most of the list API. See ``VectorProperty`` and
    ``ProxyProperty`` documentation for details.

    Please note that some of the methods accessible through the Proxy
    class are not listed by ``help()`` because the ``Proxy`` objects forward
    unresolved attributes to the underlying object. To get the full list,
    see also ``dir(proxy.SMProxy)``. See also the doxygen based documentation
    of the ``vtkSMProxy`` C++ class.
    """

    def __init__(self, **args):
        """ Default constructor. It can be used to initialize properties
        by passing keyword arguments where the key is the name of the
        property. In addition registrationGroup and registrationName (optional)
        can be specified (as keyword arguments) to automatically register
        the proxy with the proxy manager. """
        self.add_attribute('Observed', None)
        self.add_attribute('ObserverTag', -1)
        self.add_attribute('_Proxy__Properties', {})
        self.add_attribute('_Proxy__LastAttrName', None)
        self.add_attribute('SMProxy', None)
        self.add_attribute('IgnoreUnknownSetRequests', False)
        self.add_attribute('pxm', ProxyManager())
        self.add_attribute('_prev_active', None)

        if 'port' in args:
            self.add_attribute('Port', args['port'])
            del args['port']
        else:
            self.add_attribute('Port', 0)

        update = True
        if 'no_update' in args:
            if args['no_update']:
                update = False
            del args['no_update']

        if 'proxy' in args:
            self.InitializeFromProxy(args['proxy'])
            del args['proxy']
        else:
            self.Initialize(None, update)

        if 'registrationGroup' in args:
            registrationGroup = args['registrationGroup']
            del args['registrationGroup']
            registrationName = self.SMProxy.GetGlobalIDAsString()
            if 'registrationName' in args:
                registrationName = args['registrationName']
                del args['registrationName']
            self.pxm.RegisterProxy(registrationGroup, registrationName, self.SMProxy)

        if update:
            self.UpdateVTKObjects()

        proxy_util.set(self, **args)

        # Visit all properties so that they are created
        for prop in self:
            pass

    def add_attribute(self, name, value):
        self.__dict__[name] = value

    def __del__(self):
        """Destructor. Cleans up all observers as well as remove
        the proxy from the _pyproxies dictionary"""
        # Make sure that we remove observers we added
        if self.Observed:
            observed = self.Observed
            tag = self.ObserverTag
            self.Observed = None
            self.ObserverTag = -1
            observed.RemoveObserver(tag)
        if _pyproxies and self.SMProxy and (self.SMProxy, self.Port) in _pyproxies:
            del _pyproxies[(self.SMProxy, self.Port)]

    def InitializeFromProxy(self, aProxy, update=True):
        """Constructor. Assigns proxy to self.SMProxy, updates the server
        object as well as register the proxy in _pyproxies dictionary."""
        import weakref
        self.SMProxy = aProxy
        if update:
            self.SMProxy.UpdateVTKObjects()
        _pyproxies[(self.SMProxy, self.Port)] = weakref.ref(self)

    def Initialize(self):
        "Overridden by the subclass created automatically"
        pass

    def __eq__(self, other):
        "Returns true if the underlying SMProxies are the same."
        if isinstance(other, Proxy):
            try:
                if self.Port != other.Port:
                    return False
            except:
                pass
            return self.SMProxy == other.SMProxy
        return self.SMProxy == other

    def __ne__(self, other):
        "Returns false if the underlying SMProxies are the same."
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.SMProxy)

    def __iter__(self):
        "Creates an iterator for the properties."
        return PropertyIterator(self)

    def SetPropertyWithName(self, pname, arg):
        """Generic method for setting the value of a property."""
        prop = self.GetProperty(pname)
        if prop is None:
            raise RuntimeError("Property %s does not exist. Please check the property name for typos." % pname)
        prop.SetData(arg)

    def GetPropertyValue(self, name):
        """Returns a scalar for properties with 1 elements, the property
        itself for vectors."""
        p = self.GetProperty(name)
        if isinstance(p, VectorProperty) and paraview.compatibility.GetVersion() <= (4, 1) and name == "ColorArrayName":
            # Return ColorArrayName as just the array name for backwards compatibility.
            return p[1]
        if isinstance(p, EnumerationProperty) or \
                isinstance(p, ArraySelectionProperty) or \
                isinstance(p, StringListProperty) or \
                isinstance(p, ArrayListProperty):
            # with domain based property, return the property to provide access to Available method
            return p
        elif isinstance(p, VectorProperty):
            if p.GetNumberOfElements() == 1 and not p.GetRepeatable():
                if p.SMProperty.IsA("vtkSMStringVectorProperty") or not p.GetArgumentIsArray():
                    return p[0]
        elif isinstance(p, InputProperty):
            if not p.GetMultipleInput():
                if len(p) > 0:
                    return p[0]
                else:
                    return None
        elif isinstance(p, ProxyProperty):
            if not p.GetRepeatable():
                if len(p) > 0:
                    return p[0]
                else:
                    return None
        return p

    def GetProperty(self, name):
        """Given a property name, returns the property object."""
        if name in self.__Properties and self.__Properties[name]():
            return self.__Properties[name]()
        smproperty = self.SMProxy.GetProperty(name)
        # Maybe they are looking by the label. Try to match that.
        if not smproperty:
            iter = PropertyIterator(self)
            for prop in iter:
                if name == _make_name_valid(iter.PropertyLabel):
                    smproperty = prop.SMProperty
                    break
        if smproperty:
            property = _wrap_property(self, smproperty)
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
            name = _make_name_valid(iter.PropertyLabel)
            if name:
                property_list.append(name)
        return property_list

    def Set(self, **properties):
        """Update a set of properties using a keyword argument notation"""
        available_props = set(self.ListProperties())
        props_to_set = set(properties.keys())
        if props_to_set <= available_props:
            proxy_util.set(self, **properties)
        else:
            valid_props = {}
            for k, v in properties.items():
                if k not in available_props:
                    print_warning(f"Property \"{k}={v}\" is not available on {self.GetXMLLabel()}")
                else:
                    valid_props[k] = v

            proxy_util.set(self, **valid_props)

    def __ConvertArgumentsAndCall(self, *args):
        """ Internal function.
        Used to call a function on SMProxy. Converts input and
        output values as appropriate.
        """
        newArgs = []
        for arg in args:
            if issubclass(type(arg), Proxy) or isinstance(arg, Proxy):
                newArgs.append(arg.SMProxy)
            else:
                newArgs.append(arg)
        func = getattr(self.SMProxy, self.__LastAttrName)
        retVal = func(*newArgs)
        if isinstance(retVal, vtkSMProxy):
            return _getPyProxy(retVal)
        elif isinstance(retVal, vtkSMProperty):
            return _wrap_property(self, retVal)
        else:
            return retVal

    def __GetActiveCamera(self):
        """ This method handles GetActiveCamera specially.
            We return a decorated vtkCamera object so that whenever
            the Camera is directly modified using the Python API,
            we ensure that the Camera properties on the corresponding
            view proxy are synchronized with the underlying vtkCamera.
        """
        import weakref
        camera = self.SMProxy.GetActiveCamera()
        proxy = weakref.ref(self)

        from functools import wraps
        def _camera_sync(method):
            @wraps(method)
            def newfunc(*args, **kwargs):
                result = method(*args, **kwargs)
                if proxy():
                    proxy().SynchronizeCameraProperties()
                return result

            return newfunc

        # Camera eventually inherit from Object
        class _camera_wrapper(object):
            def __getattribute__(self, s):
                return _camera_sync(camera.__getattribute__(s))

            # Calls to __dir__ bypass the __getattribute__ function, so override it here
            # to delegate it to the vtkCameara class
            def __dir__(self):
                try:
                    return camera.__dir__()
                except:
                    return []

        return _camera_wrapper()

    def __setattr__(self, name, value):
        try:
            setter = getattr(self.__class__, name)
            setter = setter.__set__
        except AttributeError:
            paraview.print_debug_info("No attribute %s" % name)
            # Let the backwards compatibility helper try to handle this
            try:
                _bc.setattr(self, name, value)
            except _bc.Continue:
                pass
            except AttributeError:
                if self.IgnoreUnknownSetRequests:
                    pass
                else:
                    raise AttributeError("Attribute %s does not exist. " % name +
                                         " This class does not allow addition of new attributes to avoid " +
                                         "mistakes due to typos. Use add_attribute() if you really want " +
                                         "to add this attribute.")
        else:
            paraview.print_debug_info("Setting '%s' as '%s'", name, value)
            try:
                setter(self, value)
            except ValueError:
                # Try without spaces
                try:
                    setter(self, _make_name_valid(value))
                except ValueError:
                    # Let the backwards compatibility helper try to handle this
                    try:
                        _bc.setattr_fix_value(self, name, value, setter)
                    except _bc.Continue:
                        pass
                    except ValueError:
                        raise ValueError("%s is not a valid value for attribute %s." % (value, name))

    def __getattr__(self, name):
        """With the exception of a few overloaded methods,
        returns the SMProxy method"""
        if not self.SMProxy:
            raise AttributeError("class %s has no attribute %s" % ("None", name))
            return None
        # Handle GetActiveCamera specially.
        if name == "GetActiveCamera" and \
                hasattr(self.SMProxy, "GetActiveCamera"):
            return self.__GetActiveCamera
        if name == "SaveDefinition" and hasattr(self.SMProxy, "SaveDefinition"):
            return self.__SaveDefinition

        try:
            return _bc.getattr(self, name)
        except _bc.Continue:
            pass
        # If not a property, see if SMProxy has the method
        try:
            proxyAttr = getattr(self.SMProxy, name)
            self.__LastAttrName = name
            return self.__ConvertArgumentsAndCall
        except:
            pass
        return getattr(self.SMProxy, name)

    @property
    def _active_model(self):
        model_name = None
        # /!\ vtkSMRepresentationProxy are vtkSMSourceProxy
        if self.IsA("vtkSMRepresentationProxy"):
            pass # no active group for representation
        elif self.IsA("vtkSMSourceProxy"):
            model_name = "ActiveSources"
        elif self.GetXMLGroup() == "views":
            model_name = "ActiveView"

        if model_name:
            return self.pxm._get_active_model(model_name)

        return None

    def __enter__(self):
        """Activate proxy if possible"""
        active_model = self._active_model
        if active_model:
            self._prev_active = active_model.GetCurrentProxy()
            active_model.SetCurrentProxy(self.SMProxy, active_model.CLEAR_AND_SELECT)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        """Revert previously activated proxy"""
        active_model = self._active_model
        if active_model:
            active_model.SetCurrentProxy(self._prev_active, active_model.CLEAR_AND_SELECT)
        self._prev_active = None

def _extract_array_info(array):
    name = array.GetName()
    type = array.GetDataType() # need to convert that to a string
    components = array.GetNumberOfComponents()
    ranges = []
    for c_idx in range(components):
        data_range = array.GetRange(c_idx)
        default_name = f"component_{c_idx}" if components > 1 else "scalar"
        range_name = array.GetComponentName(c_idx) or default_name
        ranges.append({
            "name": range_name,
            "min": data_range[0],
            "max": data_range[1],
        })
        if components > 1:
            data_range = array.GetRange(-1)
            ranges.append({
                "name": "Magnitude",
                "min": data_range[0],
                "max": data_range[1],
            })
    return name, dict(name=name, type=type, components=components, ranges=ranges)

def _field_data_to_str(fields_info, indent=3):
    lines = [""]
    for k, v in fields_info.items():
        lines.append(f"{' '*indent}{k}: type({v.get('type')}) - tuple({v.get('components')})")
        for r in v.get("ranges"):
            lines.append(f"{' '*(indent+3)}{r.get('name')}: [{r.get('min')}, {r.get('max')}]")

    return '\n'.join(lines)

class SourceInformation:
    def __init__(self, source_proxy):
        data_info = source_proxy.GetDataInformation()

        self.bounds = data_info.DataInformation.GetBounds()
        self.number_of_points = data_info.DataInformation.GetNumberOfPoints()
        self.number_of_cells = data_info.DataInformation.GetNumberOfCells()
        self.data_type = data_info.DataInformation.GetPrettyDataTypeString()
        self.memory = data_info.DataInformation.GetMemorySize()

        # points
        self.point_data = {}
        pd_info = source_proxy.GetPointDataInformation()
        nb_arrays = pd_info.GetNumberOfArrays()
        for array_idx in range(nb_arrays):
            array_info = pd_info.GetArray(array_idx)
            k, v = _extract_array_info(array_info)
            self.point_data[k] = v

        # cells
        self.cell_data = {}
        cd_info = source_proxy.GetCellDataInformation()
        nb_arrays = cd_info.GetNumberOfArrays()
        for array_idx in range(nb_arrays):
            array_info = cd_info.GetArray(array_idx)
            k, v = _extract_array_info(array_info)
            self.cell_data[k] = v

        # fields
        self.field_data = {}
        fd_info = source_proxy.GetFieldDataInformation()
        nb_arrays = fd_info.GetNumberOfArrays()
        for array_idx in range(nb_arrays):
            array_info = fd_info.GetArray(array_idx)
            k, v = _extract_array_info(array_info)
            self.field_data[k] = v

    def __repr__(self):
        return f"""Data Information:
  - bounds: [
     {self.bounds[0]}, {self.bounds[1]},
     {self.bounds[2]}, {self.bounds[3]},
     {self.bounds[4]}, {self.bounds[5]},
    ]
  - number_of_points: {self.number_of_points}
  - number_of_cells: {self.number_of_cells}
  - data_type: {self.data_type}
  - memory: {self.memory}
  - point_data:{_field_data_to_str(self.point_data, 7)}
  - cell_data:{_field_data_to_str(self.cell_data, 7)}
  - field_data:{_field_data_to_str(self.field_data, 7)}
        """


class SourceProxy(Proxy):
    """Proxy for a source object. This class adds a few methods to Proxy
    that are specific to sources. It also provides access to the output
    ports. Output ports can be accessed by name or index:
    > op = source[0]
    or
    > op = source['some name'].
    """

    def Rename(self, new_name):
        """Rename proxy in GUI"""
        proxy_util.rename(self, "sources", new_name)

    def UpdatePipeline(self, time=None):
        """This method updates the server-side VTK pipeline and the associated
        data information. Make sure to update a source to validate the output
        meta-data."""
        if time != None:
            self.SMProxy.UpdatePipeline(time)
        else:
            self.SMProxy.UpdatePipeline()
        # This is here to cause a receive
        # on the client side so that progress works properly.
        if ActiveConnection and ActiveConnection.IsRemote():
            self.SMProxy.GetDataInformation()

    def FileNameChanged(self):
        "Called when the filename of a source proxy is changed."
        self.UpdatePipelineInformation()

    def UpdatePipelineInformation(self):
        """This method updates the meta-data of the server-side VTK pipeline and
        the associated information properties"""
        self.SMProxy.UpdatePipelineInformation()

    def GetDataInformation(self, idx=None):
        """This method returns a DataInformation wrapper around a
        vtkPVDataInformation"""
        if idx == None:
            idx = self.Port
        if self.SMProxy:
            return DataInformation( \
                self.SMProxy.GetDataInformation(idx), \
                self.SMProxy, idx)

    def __getitem__(self, idx):
        """Given a slice, int or string, returns the corresponding
        output port"""
        if isinstance(idx, slice):
            indices = idx.indices(self.SMProxy.GetNumberOfOutputPorts())
            retVal = []
            for i in range(*indices):
                retVal.append(OutputPort(self, i))
            return retVal
        elif isinstance(idx, int):
            if idx >= self.SMProxy.GetNumberOfOutputPorts() or idx < 0:
                raise IndexError
            return OutputPort(self, idx)
        else:
            return OutputPort(self, self.SMProxy.GetOutputPortIndex(idx))

    def GetPointDataInformation(self):
        """Returns the associated point data information."""
        self.UpdatePipeline()
        return FieldDataInformation(self.SMProxy, self.Port, "PointData")

    def GetCellDataInformation(self):
        """Returns the associated cell data information."""
        self.UpdatePipeline()
        return FieldDataInformation(self.SMProxy, self.Port, "CellData")

    def GetFieldDataInformation(self):
        """Returns the associated cell data information."""
        self.UpdatePipeline()
        return FieldDataInformation(self.SMProxy, self.Port, "FieldData")

    def GetInformation(self):
        return SourceInformation(self)

    PointData = property(GetPointDataInformation, None, None, "Returns point data information")
    CellData = property(GetCellDataInformation, None, None, "Returns cell data information")
    FieldData = property(GetFieldDataInformation, None, None, "Returns field data information")
    Information = property(GetInformation, None, None, "Returns a data information summary")


class ExodusIIReaderProxy(SourceProxy):
    """Special class to define convenience functions for array
    selection."""

    if paraview.compatibility.GetVersion() >= (3, 5):
        def FileNameChanged(self):
            "Called when the filename changes. Selects all variables."
            SourceProxy.FileNameChanged(self)
            self.SelectAllVariables()

        def SelectAllVariables(self):
            "Select all available variables for reading."
            for prop in ('PointVariables', 'EdgeVariables', 'FaceVariables',
                         'ElementVariables', 'GlobalVariables'):
                f = getattr(self, prop)
                f.SelectAll()

        def DeselectAllVariables(self):
            "Deselects all variables."
            for prop in ('PointVariables', 'EdgeVariables', 'FaceVariables',
                         'ElementVariables', 'GlobalVariables'):
                f = getattr(self, prop)
                f.DeselectAll()


class MultiplexerSourceProxy(SourceProxy):
    def UpdateDynamicProperties(self):
        """Update the instance to add properties of newly exposed
        SMProperties. The current limitation is that help() still
        doesn't work as one would expect."""
        exclude = frozenset([p for p in dir(self.__class__) if isinstance(getattr(self.__class__, p), property)])
        cdict = _createClassProperties(self, exclude)
        for key, val in cdict.items():
            self.add_attribute(key, val)


class RepresentationProxy(SourceProxy):
    """Proxy for a representation object. This class adds a few methods to Proxy
    that are specific to representations (ColorBy, ColorBlocksBy).
    """
    def GetView(self):
        for view in self.pxm.GetProxiesInGroup("views").values():
            try:
                if self in view.Representations:
                    return view
            except AttributeError:
                continue

        return None

    def ColorBy(self, value=None, separate=False):
        """Set data array to color a representation by. This will automatically set
        up the color maps and others necessary state for the representations.

        :param value: Name of the array to color by.
        :type value: str
        :param separate: Set to `True` to create a color map unique to this
            representation. Optional, defaults to the global color map ParaView uses
            for any object colored by an array of the same name.
        :type separate: bool"""

        self.UseSeparateColorMap = separate
        association = self.ColorArrayName.GetAssociation()
        arrayname = self.ColorArrayName.GetArrayName()
        component = None
        if value == None:
            self.SetScalarColoring(None, GetAssociationFromString(association))
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
                    array = self.Input.PointData.GetArray(arrayname)
                if association == "CELLS":
                    array = self.Input.CellData.GetArray(arrayname)
                if array:
                    # looking for corresponding component name
                    for i in range(0, array.GetNumberOfComponents()):
                        if componentName == array.GetComponentName(i):
                            component = i
                            break
                        # none have been found, try to use the name as an int
                        if i == array.GetNumberOfComponents() - 1:
                            try:
                                component = int(componentName)
                            except ValueError:
                                pass
        if component is None:
            self.SetScalarColoring(
                arrayname, GetAssociationFromString(association)
            )
        else:
            self.SetScalarColoring(
                arrayname, GetAssociationFromString(association), component
            )
        self.RescaleTransferFunctionToDataRange()

    def ColorBlocksBy(self, selectors=None, value=None, separate=False):
        """Like :func:`ColorBy`, set data array by which to color selected blocks within a
        representation, but color only selected blocks with the specified properties.
        This will automatically set up the color maps and others necessary state
        for the representations.

        :param rep: Must be a representation proxy i.e. the value returned by
            the :func:`GetRepresentation`. Optional, defaults to the display properties
            for the active source, if possible.
        :type rep: Representation proxy
        :param selectors: List of block selectors that choose which blocks to modify
            with this call.
        :type selectors: list of str
        :param value: Name of the array to color by.
        :type value: str
        :param separate: Set to `True` to create a color map unique to this
            representation. Optional, default is that the color map used will be the global
            color map ParaView uses for any object colored by an array of the same name.
        :type separate: bool"""
        if selectors is None or len(selectors) == 0:
            raise ValueError("No selector can be determined.")

        self.SetBlocksUseSeparateColorMap(selectors, separate)

        firstSelector = selectors[0]
        associationInt = self.GetBlockColorArrayAssociation(firstSelector)
        association = (
            GetAssociationAsString(associationInt)
            if associationInt != -1
            else None
        )
        arrayname = self.GetBlockColorArrayName(firstSelector)
        component = None
        if value is None:
            if association is not None:
                self.SetBlocksScalarColoring(
                    selectors, None, GetAssociationFromString(association)
                )
            else:
                self.SetBlocksScalarColoring(selectors, None, 0)
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
                    array = self.Input.PointData.GetArray(arrayname)
                if association == "CELLS":
                    array = self.Input.CellData.GetArray(arrayname)
                if array:
                    # looking for corresponding component name
                    for i in range(0, array.GetNumberOfComponents()):
                        if componentName == array.GetComponentName(i):
                            component = i
                            break
                        # none have been found, try to use the name as an int
                        if i == array.GetNumberOfComponents() - 1:
                            try:
                                component = int(componentName)
                            except ValueError:
                                pass
        if component is None:
            self.SetBlocksScalarColoring(
                selectors, arrayname, GetAssociationFromString(association)
            )
        else:
            self.SetBlocksScalarColoring(
                selectors,
                arrayname,
                GetAssociationFromString(association),
                component,
            )
        self.RescaleBlocksTransferFunctionToDataRange(selectors)

class ViewLayoutProxy(Proxy):
    """Special class to define convenience methods for View Layout"""

    def SplitViewHorizontal(self, view, fraction=0.5):
        """Split the cell containing the specified view horizontally.
        If no fraction is specified, the frame is split into equal parts.
        On success returns a positive number that identifying the new cell
        location that can be used to assign view to, or split further.
        Return -1 on failure."""
        location = self.GetViewLocation(view)
        if location == -1:
            raise RuntimeError("View is not present in this layout.")
        if fraction < 0.0 or fraction > 1.0:
            raise RuntimeError("'fraction' must be in the range [0.0, 1.0]")
        return self.SMProxy.SplitHorizontal(location, fraction)

    def SplitViewVertical(self, view=None, fraction=0.5):
        """Split the cell containing the specified view horizontally.
        If no view is specified, active view is used.
        If no fraction is specified, the frame is split into equal parts.
        On success returns a positive number that identifying the new cell
        location that can be used to assign view to, or split further.
        Return -1 on failure."""
        location = self.GetViewLocation(view)
        if location == -1:
            raise RuntimeError("View is not present in this layout.")
        if fraction < 0.0 or fraction > 1.0:
            raise RuntimeError("'fraction' must be in the range [0.0, 1.0]")
        return self.SMProxy.SplitVertical(location, fraction)

    def AssignView(self, location, view):
        """Assign a view at a particular location. Note that the view's position may
        be changed by subsequent Split() calls. Returns true on success."""
        viewproxy = None
        if isinstance(view, Proxy):
            view = view.SMProxy
        return self.SMProxy.AssignView(location, view)

    def GetViewLocation(self, view):
        if isinstance(view, Proxy):
            view = view.SMProxy
        return self.SMProxy.GetViewLocation(view)


class Property(object):
    """Generic property object that provides access to one of the properties of
    a server object. This class does not allow setting/getting any values but
    provides an interface to update a property using __call__. This can be used
    for command properties that correspond to function calls without arguments.
    For example,
    > proxy.Foo()
    would push a Foo property which may cause the proxy to call a Foo method
    on the actual VTK object.

    For advanced users:
    Python wrapper around a vtkSMProperty with a simple interface.
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

    def __eq__(self, other):
        "Returns true if the properties or properties values are the same."
        return ((self is None and other is None) or
                (self is not None and other is not None and self.__repr__() == other.__repr__()))

    def __repr__(self):
        """Returns a string representation containing property name
        and value"""
        if not type(self) is Property:
            if self.GetData() is not None:
                repr = self.GetData().__repr__()
            else:
                repr = "None"
        else:
            # this happens for command properties i.e. ones that do not store
            # any state. For those, we use this alternate representation.
            repr = "Property name= "
            name = self.Proxy.GetPropertyName(self.SMProperty)
            if name:
                repr += name
            else:
                repr += "Unknown"
        return repr

    def __call__(self):
        """Forces a property update using InvokeCommand."""
        if type(self) is Property:
            self.Proxy.SMProxy.InvokeCommand(self._FindPropertyName())
        else:
            raise RuntimeError("Cannot invoke this property")

    def _FindPropertyName(self):
        "Returns the name of this property."
        return self.Proxy.GetPropertyName(self.SMProperty)

    def _UpdateProperty(self):
        "Pushes the value of this property to the server."
        # For now, we are updating all properties. This is due to an
        # issue with the representations. Their VTK objects are not
        # created until Input is set therefore, updating a property
        # has no effect. Updating all properties everytime one is
        # updated has the effect of pushing values set before Input
        # when Input is updated.
        # self.Proxy.SMProxy.UpdateProperty(self._FindPropertyName())
        self.Proxy.SMProxy.UpdateVTKObjects()

    def __getattr__(self, name):
        "Unknown attribute requests get forwarded to SMProperty."
        return getattr(self.SMProperty, name)

    Name = property(_FindPropertyName, None, None,
                    "Returns the name for the property")


class GenericIterator(object):
    """Iterator for container type objects"""

    def __init__(self, obj):
        self.Object = obj
        self.index = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self.index >= len(self.Object):
            raise StopIteration

        idx = self.index
        self.index += 1
        return self.Object[idx]


class VectorProperty(Property):
    """A VectorProperty provides access to one or more values. You can use
    a slice to get one or more property values:
    > val = property[2]
    or
    > vals = property[0:5:2]
    You can use a slice to set one or more property values:
    > property[2] = val
    or
    > property[1:3] = (1,2)
    """

    def ConvertValue(self, value):
        return value

    def __len__(self):
        """Returns the number of elements."""
        return self.SMProperty.GetNumberOfElements()

    def __iter__(self):
        """Implementation of the sequence API"""
        return GenericIterator(self)

    def __setitem__(self, idx, value):
        """Given a list or tuple of values, sets a slice of values [min, max)"""
        if isinstance(idx, slice):
            indices = idx.indices(len(self))
            for i, j in zip(range(*indices), value):
                self.SMProperty.SetElement(i, self.ConvertValue(j))
            self._UpdateProperty()
        elif idx >= len(self) or idx < 0:
            raise IndexError
        else:
            self.SMProperty.SetElement(idx, self.ConvertValue(value))
            self._UpdateProperty()

    def GetElement(self, index):
        return self.SMProperty.GetElement(index)

    def __getitem__(self, idx):
        """Returns the range [min, max) of elements. Raises an IndexError
        exception if an argument is out of bounds."""
        ls = len(self)
        if isinstance(idx, slice):
            indices = idx.indices(ls)
            retVal = []
            for i in range(*indices):
                retVal.append(self.GetElement(i))
            return retVal
        elif idx >= ls:
            raise IndexError
        elif idx < 0:
            idx = ls + idx
            if idx < 0:
                raise IndexError

        return self.GetElement(idx)

    def GetData(self):
        "Returns all elements as either a list or a single value."
        property = self.SMProperty
        if property.GetRepeatable() or \
                property.GetNumberOfElements() > 1:
            return self[0:len(self)]
        elif property.GetNumberOfElements() == 1:
            return self.GetElement(0)

    def SetData(self, values):
        """Allows setting of all values at once. Requires a single value or
        a iterable object."""
        if self.SMProperty.GetInformationOnly() != 0:
            raise RuntimeError("Cannot set an InformationOnly property!")
        # Python3: str now has attr "__iter__", test separately
        if (not hasattr(values, "__iter__")) or (type(values) == type("")):
            values = (values,)
        if not self.GetRepeatable() and len(values) != self.GetNumberOfElements():
            raise RuntimeError("This property requires %d values." % self.GetNumberOfElements())

        # map returns an iterable, must be converted to list.
        convertedValues = list(map(self.ConvertValue, values))
        if self.GetRepeatable():
            # Clean up first
            self.SMProperty.SetNumberOfElements(len(convertedValues))

        try:
            # SetElements() isn't available for all VectorProperty values.
            # Try to use it so that the proxies are updated only once.
            # Otherwise, call SetElement() for each element.
            self.SMProperty.SetElements(convertedValues)
        except TypeError as e:
            idx = 0
            for val in convertedValues:
                self.SMProperty.SetElement(idx, val)
                idx += 1

        self._UpdateProperty()

    def Clear(self):
        "Removes all elements."
        self.SMProperty().SetNumberOfElements(0)
        self._UpdateProperty()


class DoubleMapProperty(Property):
    """A DoubleMapProperty provides access to a map of double vector values."""

    def __len__(self):
        """Returns the number of elements."""
        return self.SMProperty.GetNumberOfElements()

    def __getitem__(self, key):
        """Returns the values for key."""
        return self.GetData()[key]

    def __setitem__(self, key, values):
        """Sets the values for key."""
        for i, value in enumerate(values):
            self.SMProperty.SetElementComponent(key, i, value)
        self._UpdateProperty()

    def __contains__(self, key):
        """Returns True if the property contains key."""
        return key in self.keys()

    def keys(self):
        """Returns the keys."""
        return self.GetData().keys()

    def items(self):
        """Iterates over the (key, value) pairs."""
        return self.GetData().items()

    def values(self):
        """Returns the values"""
        return self.GetData().values()

    def get(self, key, default_value=None):
        """Returns value of the given key, or the default_value if the key is not found
        in the map."""
        return self.GetData().get(key, default_value)

    def GetData(self):
        """Returns all the elements as a dictionary"""

        data = {}

        iter = self.SMProperty.NewIterator()
        while not iter.IsAtEnd():
            values = []
            for i in range(self.SMProperty.GetNumberOfComponents()):
                values.append(iter.GetElementComponent(i))
            data[iter.GetKey()] = values
            iter.Next()
        iter.UnRegister(None)

        return data

    def SetData(self, elements):
        """Sets all the elements at once."""
        if self.SMProperty.GetInformationOnly() != 0:
            raise RuntimeError("Cannot set an InformationOnly property!")
        # first clear existing data
        self.Clear()

        for key, values in elements.items():
            for i, value in enumerate(values):
                self.SMProperty.SetElementComponent(key, i, value)

        self._UpdateProperty()

    def Clear(self):
        """Removes all elements."""
        self.SMProperty.ClearElements()
        self._UpdateProperty()


class EnumerationProperty(VectorProperty):
    """Subclass of VectorProperty that is applicable for enumeration type
    properties."""

    def GetElement(self, index):
        """Returns the text for the given element if available. Returns
        the numerical values otherwise."""
        val = self.SMProperty.GetElement(index)
        domain = self.SMProperty.FindDomain("vtkSMEnumerationDomain")
        if domain:
            for i in range(domain.GetNumberOfEntries()):
                if domain.GetEntryValue(i) == val:
                    return domain.GetEntryText(i)
        return val

    def ConvertValue(self, value):
        """Converts value to type suitable for vtSMProperty::SetElement()"""
        if type(value) == str:
            domain = self.SMProperty.FindDomain("vtkSMEnumerationDomain")
            if domain:
                if domain.HasEntryText(value):
                    return domain.GetEntryValueForText(value)
                else:
                    raise ValueError("%s is not a valid value." % value)
        return VectorProperty.ConvertValue(self, value)

    def GetAvailable(self):
        "Returns the list of available values for the property, if an enumeration domain is available."
        retVal = []
        domain = self.SMProperty.FindDomain("vtkSMEnumerationDomain")
        if domain:
            for i in range(domain.GetNumberOfEntries()):
                retVal.append(domain.GetEntryText(i))
        return retVal

    Available = property(GetAvailable, None, None, \
                         "This read-only property contains the list of values that can be applied to this property.")


class FileNameProperty(VectorProperty):
    """Property to set/get one or more file names.
    This property updates the pipeline information everytime its value changes.
    This is used to keep the array lists up to date."""

    def _UpdateProperty(self):
        "Pushes the value of this property to the server."
        VectorProperty._UpdateProperty(self)
        try:
            self.Proxy.FileNameChanged()
        except AttributeError:
            pass


class ArraySelectionProperty(VectorProperty):
    "Property to select an array to be processed by a filter."

    def GetAssociation(self):
        val = self.GetElement(3)
        if val == "":
            return None
        return GetAssociationAsString(int(val))

    def GetArrayName(self):
        return self.GetElement(4)

    def __len__(self):
        """Returns the number of elements."""
        return 2

    def __setitem__(self, idx, value):
        raise RuntimeError("This property cannot be accessed using __setitem__")

    def __getitem__(self, idx):
        """Returns attribute type for index 0, array name for index 1"""
        if isinstance(idx, slice):
            indices = idx.indices(len(self))
            retVal = []
            for i in range(*indices):
                if i >= 2 or i < 0:
                    raise IndexError
                if i == 0:
                    retVal.append(self.GetAssociation())
                else:
                    retVal.append(self.GetArrayName())
            return retVal
        elif idx >= 2 or idx < 0:
            raise IndexError

        if idx == 0:
            return self.GetAssociation()
        else:
            return self.GetArrayName()

    def SetData(self, values):
        """Allows setting of all values at once. Requires a single value,
        a tuple or list."""
        if self.SMProperty.GetInformationOnly() != 0:
            raise RuntimeError("Cannot set an InformationOnly property!")
        if not values:
            # if values is None or empty list, we are resetting the selection.
            self.SMProperty.SetElement(4, "")
            self._UpdateProperty()
            return

        if not isinstance(values, tuple) and \
                not isinstance(values, list):
            values = (values,)
        if len(values) == 1:
            self.SMProperty.SetElement(4, values[0])
        elif len(values) == 2:
            if isinstance(values[0], str):
                val = GetAssociationFromString(values[0])
            else:
                # In case user didn't specify valid association,
                # just pick POINTS.
                val = GetAssociationFromString("POINTS")
            self.SMProperty.SetElement(3, str(val))
            self.SMProperty.SetElement(4, values[1])
        else:
            raise RuntimeError("Expected 1 or 2 values.")
        self._UpdateProperty()

    def UpdateDefault(self):
        "Helper method to set default values."
        if self.SMProperty.GetNumberOfElements() != 5:
            return
        if self.GetElement(4) != '' or \
                self.GetElement(3) != '':
            return

        for i in range(0, 3):
            if self.GetElement(i) == '':
                self.SMProperty.SetElement(i, '0')
        self.SMProperty.ResetToDomainDefaults(False)


class StringListProperty(VectorProperty):
    """Property to set/get the a string with a string list domain.
    This property provides an interface to get available strings."""

    def GetAvailable(self):
        "Returns the list of string values available for the property."
        retVal = []
        domain = self.SMProperty.FindDomain("vtkSMStringListDomain")
        if domain:
            for i in range(domain.GetNumberOfStrings()):
                retVal.append(domain.GetString(i))
        return retVal

    Available = property(GetAvailable, None, None, \
                         "This read-only property contains the list of values that can be applied to this property.")


class ArrayListProperty(VectorProperty):
    """This property provides a simpler interface for selecting arrays.
    Simply assign a list of arrays that should be loaded by the reader.
    Use the Available property to get a list of available arrays."""

    def __init__(self, proxy, smproperty):
        VectorProperty.__init__(self, proxy, smproperty)
        self.__arrays = []

    def GetAvailable(self):
        "Returns the list of available arrays"
        dm = self.SMProperty.FindDomain("vtkSMArraySelectionDomain")
        if not dm:
            dm = self.SMProperty.FindDomain("vtkSMChartSeriesSelectionDomain")
        retVal = []
        if dm:
            for i in range(dm.GetNumberOfStrings()):
                retVal.append(dm.GetString(i))
        return retVal

    Available = property(GetAvailable, None, None, \
                         "This read-only property contains the list of items that can be read by a reader.")

    def SelectAll(self):
        "Selects all arrays."
        self.SetData(self.Available)

    def DeselectAll(self):
        "Deselects all arrays."
        self.SetData([])

    def __iter__(self):
        """Implementation of the sequence API"""
        return GenericIterator(self)

    def __len__(self):
        """Returns the number of elements."""
        return len(self.GetData())

    def __setitem__(self, idx, value):
        """Given a list or tuple of values, sets a slice of values [min, max)"""
        self.GetData()
        if isinstance(idx, slice):
            indices = idx.indices(len(self))
            for i, j in zip(range(*indices), value):
                self.__arrays[i] = j
            self.SetData(self.__arrays)
        elif idx >= len(self) or idx < 0:
            raise IndexError
        else:
            self.__arrays[idx] = self.ConvertValue(value)
            self.SetData(self.__arrays)

    def __getitem__(self, idx):
        """Returns the range [min, max) of elements. Raises an IndexError
        exception if an argument is out of bounds."""
        self.GetData()
        if isinstance(idx, slice):
            indices = idx.indices(len(self))
            retVal = []
            for i in range(*indices):
                retVal.append(self.__arrays[i])
            return retVal
        elif idx >= len(self) or idx < 0:
            raise IndexError
        return self.__arrays[idx]

    def SetData(self, values):
        """Allows setting of all values at once. Requires a single value,
        a tuple or list."""
        if self.SMProperty.GetInformationOnly() != 0:
            raise RuntimeError("Cannot set an InformationOnly property!")
        # Clean up first
        iup = self.SMProperty.GetImmediateUpdate()
        self.SMProperty.SetImmediateUpdate(False)
        # Clean up first
        self.SMProperty.SetNumberOfElements(0)
        if not isinstance(values, tuple) and \
                not isinstance(values, list):
            values = (values,)
        fullvalues = []

        # WARNING:
        # The order of the two loops below are delibrately set in this way
        # so that values passed in will take precedence.
        # This is needed for backward compatibility of the
        # property ElementBlocks for vtkExodusIIReader.
        # If you attempt to change this, please verify that
        # python state files for opening old .ex2 file (<=3.14) still works.
        for array in self.Available:
            if not values.__contains__(array):
                fullvalues.append(array)
                fullvalues.append('0')

        for i in range(len(values)):
            val = self.ConvertValue(values[i])
            fullvalues.append(val)
            fullvalues.append('1')

        i = 0
        for value in fullvalues:
            self.SMProperty.SetElement(i, value)
            i += 1

        self._UpdateProperty()
        self.SMProperty.SetImmediateUpdate(iup)

    def GetData(self):
        "Returns all elements as a list."
        property = self.SMProperty
        nElems = property.GetNumberOfElements()
        if nElems % 2 != 0:
            raise ValueError(
                "The SMProperty with XML label '%s' has a size that is not a multiple of 2." % property.GetXMLLabel())
        self.__arrays = []
        for i in range(0, nElems, 2):
            if self.GetElement(i + 1) != '0':
                self.__arrays.append(self.GetElement(i))
        return list(self.__arrays)


class ProxyProperty(Property):
    """A ProxyProperty provides access to one or more proxies. You can use
    a slice to get one or more property values:
    > proxy = property[2]
    or
    > proxies = property[0:5:2]
    You can use a slice to set one or more property values:
    > property[2] = proxy
    or
    > property[1:3] = (proxy1, proxy2)
    You can also append and delete:
    > property.append(proxy)
    and
    > del property[1:2]

    You can also remove all elements with Clear().

    Note that some properties expect only 1 proxy and will complain if
    you set the number of values to be something else.
    """

    def __init__(self, proxy, smproperty):
        """Default constructor.  Stores a reference to the proxy.  Also looks
        at domains to find valid values."""
        Property.__init__(self, proxy, smproperty)
        # Check to see if there is a proxy list domain and, if so,
        # initialize ourself. (Should this go in ProxyProperty?)
        listdomain = self.FindDomain("vtkSMProxyListDomain")
        if listdomain:
            pm = ProxyManager()
            group = "pq_helper_proxies." + proxy.GetGlobalIDAsString()
            if listdomain.GetNumberOfProxies() == 0:
                for i in range(listdomain.GetNumberOfProxyTypes()):
                    igroup = listdomain.GetProxyGroup(i)
                    name = listdomain.GetProxyName(i)
                    iproxy = CreateProxy(igroup, name)
                    listdomain.AddProxy(iproxy)
                smproperty.ResetToDomainDefaults(False)

    def GetAvailable(self):
        """If this proxy has a list domain, then this function returns the
        strings you can use to select from the domain.  If there is no such
        list domain, the returned list is empty."""
        listdomain = self.SMProperty.FindDomain("vtkSMProxyListDomain")
        retval = []
        if listdomain:
            for i in range(listdomain.GetNumberOfProxies()):
                proxy = listdomain.GetProxy(i)
                retval.append(_make_name_valid(proxy.GetXMLLabel()))
        return retval

    Available = property(GetAvailable, None, None,
                         """This read only property is a list of strings you can
                         use to select from the list domain.  If there is no
                         such list domain, the array is empty.""")

    def __iter__(self):
        """Implementation of the sequence API"""
        return GenericIterator(self)

    def __len__(self):
        """Returns the number of elements."""
        return self.SMProperty.GetNumberOfProxies()

    def remove(self, proxy):
        """Removes the first occurrence of the proxy from the property."""
        self.SMProperty.RemoveProxy(proxy.SMProxy)
        self._UpdateProperty()

    def __setitem__(self, idx, value):
        """Given a list or tuple of values, sets a slice of values [min, max)"""
        if isinstance(idx, slice):
            indices = idx.indices(len(self))
            for i, j in zip(range(*indices), value):
                self.SMProperty.SetProxy(i, j.SMProxy)
            self._UpdateProperty()
        elif idx >= len(self) or idx < 0:
            raise IndexError
        else:
            self.SMProperty.SetProxy(idx, value.SMProxy)
            self._UpdateProperty()

    def __delitem__(self, idx):
        """Removes the element idx"""
        if isinstance(idx, slice):
            indices = idx.indices(len(self))
            # Collect the elements to delete to a new list first.
            # Otherwise indices are screwed up during the actual
            # remove loop.
            toremove = []
            for i in range(*indices):
                toremove.append(self[i])
            for i in toremove:
                self.SMProperty.RemoveProxy(i.SMProxy)
            self._UpdateProperty()
        elif idx >= len(self) or idx < 0:
            raise IndexError
        else:
            self.SMProperty.RemoveProxy(self[idx].SMProxy)
            self._UpdateProperty()

    def __getitem__(self, idx):
        """Returns the range [min, max) of elements. Raises an IndexError
        exception if an argument is out of bounds."""
        if isinstance(idx, slice):
            indices = idx.indices(len(self))
            retVal = []
            for i in range(*indices):
                retVal.append(_getPyProxy(self.SMProperty.GetProxy(i)))
            return retVal
        elif idx >= len(self) or idx < 0:
            raise IndexError
        return _getPyProxy(self.SMProperty.GetProxy(idx))

    def __getattr__(self, name):
        "Unknown attribute requests get forwarded to SMProperty."
        return getattr(self.SMProperty, name)

    def index(self, proxy):
        idx = 0
        for px in self:
            if proxy == px:
                return idx
            idx += 1
        raise ValueError("proxy is not in the list.")

    def append(self, proxy):
        "Appends the given proxy to the property values."
        self.SMProperty.AddProxy(proxy.SMProxy)
        self._UpdateProperty()

    def GetData(self):
        "Returns all elements as either a list or a single value."
        property = self.SMProperty
        if property.GetRepeatable() or property.GetNumberOfProxies() > 1:
            return self[0:len(self)]
        else:
            if property.GetNumberOfProxies() > 0:
                return _getPyProxy(property.GetProxy(0))
        return None

    def SetData(self, values):
        """Allows setting of all values at once. Requires a single value,
        a tuple or list."""
        if self.SMProperty.GetInformationOnly() != 0:
            raise RuntimeError("Cannot set an InformationOnly property!")
        if isinstance(values, str):
            position = -1
            try:
                position = self.Available.index(values)
            except:
                raise ValueError(values + " is not a valid object in the domain.")
            listdomain = self.SMProperty.FindDomain("vtkSMProxyListDomain")
            if listdomain:
                values = listdomain.GetProxy(position)
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
    """An InputProperty allows making pipeline connections. You can set either
    a source proxy or an OutputProperty to an input property:

    > property[0] = proxy
    or
    > property[0] = OuputPort(proxy, 1)

    > property.append(proxy)
    or
    > property.append(OutputPort(proxy, 0))
    """

    def __setitem__(self, idx, value):
        """Given a list or tuple of values, sets a slice of values [min, max)"""
        if isinstance(idx, slice):
            indices = idx.indices(len(self))
            for i, j in zip(range(*indices), value):
                op = value[i - min]
                self.SMProperty.SetInputConnection(i, op.SMProxy, op.Port)
            self._UpdateProperty()
        elif idx >= len(self) or idx < 0:
            raise IndexError
        else:
            self.SMProperty.SetInputConnection(idx, value.SMProxy, value.Port)
            self._UpdateProperty()

    def __getitem__(self, idx):
        """Returns the range [min, max) of elements. Raises an IndexError
        exception if an argument is out of bounds."""
        if isinstance(idx, slice):
            indices = idx.indices(len(self))
            retVal = []
            for i in range(*indices):
                port = None
                if self.SMProperty.GetProxy(i):
                    port = OutputPort(_getPyProxy(self.SMProperty.GetProxy(i)), \
                                      self.SMProperty.GetOutputPortForConnection(i))
                retVal.append(port)
            return retVal
        elif idx >= len(self) or idx < 0:
            raise IndexError
        return OutputPort(_getPyProxy(self.SMProperty.GetProxy(idx)), \
                          self.SMProperty.GetOutputPortForConnection(idx))

    def append(self, value):
        """Appends the given proxy to the property values.
        Accepts Proxy or OutputPort objects."""
        self.SMProperty.AddInputConnection(value.SMProxy, value.Port)
        self._UpdateProperty()

    def GetData(self):
        """Returns all elements as either a list of OutputPort objects or
        a single OutputPort object."""
        property = self.SMProperty
        if property.GetRepeatable() or property.GetNumberOfProxies() > 1:
            return self[0:len(self)]
        else:
            if property.GetNumberOfProxies() > 0:
                return OutputPort(_getPyProxy(property.GetProxy(0)), \
                                  self.SMProperty.GetOutputPortForConnection(0))
        return None

    def SetData(self, values):
        """Allows setting of all values at once. Requires a single value,
        a tuple or list. Accepts Proxy or OutputPort objects."""
        if isinstance(values, str):
            ProxyProperty.SetData(self, values)
            return
        if not isinstance(values, tuple) and \
                not isinstance(values, list):
            values = (values,)
        self.SMProperty.RemoveAllProxies()
        for value in values:
            if value:
                self.SMProperty.AddInputConnection(value.SMProxy, value.Port)
        self._UpdateProperty()

    def _UpdateProperty(self):
        "Pushes the value of this property to the server."
        ProxyProperty._UpdateProperty(self)
        iter = PropertyIterator(self.Proxy)
        for prop in iter:
            if isinstance(prop, ArraySelectionProperty):
                prop.UpdateDefault()


class DataInformation(object):
    """DataInformation is a contained for meta-data associated with an
    output data.

    DataInformation is a python wrapper around a vtkPVDataInformation.
    In addition to proving all methods of a vtkPVDataInformation, it provides
    a few convenience methods.

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

    def GetDataSetType(self):
        """Returns the dataset type as defined in vtkDataObjectTypes."""
        if not self.DataInformation:
            raise RuntimeError("No data information is available")
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
            raise AttributeError("class has no attribute %s" % name)
            return None
        return getattr(self.DataInformation, name)


class ArrayInformation(object):
    """Meta-information associated with an array. Use the Name
    attribute to get the array name.

    Please note that some of the methods accessible through the ArrayInformation
    class are not listed by help() because the ArrayInformation objects forward
    unresolved attributes to the underlying object.
    See the doxygen based documentation of the vtkPVArrayInformation C++
    class for a full list.
    """

    def __init__(self, proxy, field, name):
        self.Proxy = proxy
        self.FieldData = field
        self.Name = name

    def __getattr__(self, name):
        """Forward unknown methods to vtkPVArrayInformation"""
        array = self.FieldData.GetFieldData().GetArrayInformation(self.Name)
        if not array: return None
        return getattr(array, name)

    def __repr__(self):
        """Returns a user-friendly representation string."""
        return "Array: " + self.Name

    def GetRange(self, component=0):
        """Given a component, returns its value range as a tuple of 2 values."""
        array = self.FieldData.GetFieldData().GetArrayInformation(self.Name)
        range = array.GetComponentRange(component)
        return (range[0], range[1])

    def __eq__(self, other):
        if (self is None and other is None):
            return True
        if (self is None or other is None):
            return False

        return self.Proxy == other.Proxy and self.FieldData == other.FieldData and self.Name == other.Name

    def __neq__(self, other):
        return not self.__eq__(other)

    if paraview.compatibility.GetVersion() <= (3, 4):
        def Range(self, component=0):
            return self.GetRange(component)


class FieldDataInformationIterator(object):
    """Iterator for FieldDataInformation"""

    def __init__(self, info, items=False):
        self.FieldDataInformation = info
        self.index = 0
        self.items = items

    def __iter__(self):
        return self

    def __next__(self):
        if self.index >= self.FieldDataInformation.GetNumberOfArrays():
            raise StopIteration

        self.index += 1
        ai = self.FieldDataInformation[self.index - 1]
        if self.items:
            return (ai.GetName(), ai)
        else:
            return ai


class FieldDataInformation(object):
    """Meta-data for a field of an output object (point data, cell data etc...).
    Provides easy access to the arrays using the slice interface:
    > narrays = len(field_info)
    > for i in range(narrays):
    >   array_info = field_info[i]

    Full slice interface is supported:
    > arrays = field_info[0:5:3]
    where arrays is a list.

    Array access by name is also possible:
    > array_info = field_info['Temperature']

    The number of arrays can also be accessed using the NumberOfArrays
    property.
    """

    def __init__(self, proxy, idx, field):
        self.Proxy = proxy
        self.OutputPort = idx
        self.FieldData = field

    def GetFieldData(self):
        """Convenience method to get the underlying
        vtkPVDataSetAttributesInformation"""
        return getattr(self.Proxy.GetDataInformation(self.OutputPort), "Get%sInformation" % self.FieldData)()

    def GetNumberOfArrays(self):
        """Returns the number of arrays."""
        self.Proxy.UpdatePipeline()
        return self.GetFieldData().GetNumberOfArrays()

    def GetArray(self, idx):
        """Given an index or a string, returns an array information.
        Raises IndexError if the index is out of bounds."""
        self.Proxy.UpdatePipeline()
        if not self.GetFieldData().GetArrayInformation(idx):
            return None
        if isinstance(idx, str):
            return ArrayInformation(self.Proxy, self, idx)
        elif idx >= len(self) or idx < 0:
            raise IndexError
        return ArrayInformation(self.Proxy, self, self.GetFieldData().GetArrayInformation(idx).GetName())

    def __len__(self):
        """Returns the number of arrays."""
        return self.GetNumberOfArrays()

    def __getitem__(self, idx):
        """Implements the [] operator. Accepts an array name."""
        if isinstance(idx, slice):
            indices = idx.indices(self.GetNumberOfArrays())
            retVal = []
            for i in range(*indices):
                retVal.append(self.GetArray(i))
            return retVal
        return self.GetArray(idx)

    def keys(self):
        """Implementation of the dictionary API"""
        kys = []
        narrays = self.GetNumberOfArrays()
        for i in range(narrays):
            kys.append(self.GetArray(i).GetName())
        return kys

    def values(self):
        """Implementation of the dictionary API"""
        vals = []
        narrays = self.GetNumberOfArrays()
        for i in range(narrays):
            vals.append(self.GetArray(i))
        return vals

    def iteritems(self):
        """Implementation of the PY2 dictionary API"""
        return FieldDataInformationIterator(self, True)

    def items(self):
        """Implementation of the dictionary API"""
        itms = []
        narrays = self.GetNumberOfArrays()
        for i in range(narrays):
            ai = self.GetArray(i)
            itms.append((ai.GetName(), ai))
        return itms

    def has_key(self, key):
        """Implementation of the PY2 dictionary API"""
        if self.GetArray(key):
            return True
        return False

    def __contains__(self, key):
        """Implementation of the dictionary API"""
        if self.GetArray(key):
            return True
        return False

    def __iter__(self):
        """Implementation of the dictionary API"""
        return FieldDataInformationIterator(self)

    def __getattr__(self, name):
        """Forwards unknown attributes to the underlying
        vtkPVDataSetAttributesInformation"""
        array = self.GetArray(name)
        if array: return array
        raise AttributeError("class has no attribute %s" % name)
        return None

    NumberOfArrays = property(GetNumberOfArrays, None, None, "Returns the number of arrays.")


def OutputPort(proxy, outputPort=0):
    if not Proxy:
        return None
    if isinstance(outputPort, str):
        outputPort = proxy.GetOutputPortIndex(outputPort)
    if outputPort >= proxy.GetNumberOfOutputPorts():
        return None
    if proxy.Port == outputPort:
        return proxy
    newinstance = _getPyProxy(proxy.SMProxy, outputPort)
    newinstance.Port = outputPort
    newinstance._Proxy__Properties = proxy._Proxy__Properties
    return newinstance


class ProxyManager(object):
    """When running scripts from the python shell in the ParaView application,
    registering proxies with the proxy manager is the only mechanism to
    notify the graphical user interface (GUI) that a proxy
    exists. Therefore, unless a proxy is registered, it will not show up in
    the user interface. Also, the proxy manager is the only way to get
    access to proxies created using the GUI. Proxies created using the GUI
    are automatically registered under an appropriate group (sources,
    filters, representations and views). To get access to these objects,
    you can use proxyManager.GetProxy(group, name). The name is the same
    as the name shown in the pipeline browser.

    This class is a python wrapper for vtkSMProxyManager. Note that the
    underlying vtkSMProxyManager is a singleton. All instances of this
    class will refer to the same object. In addition to all methods provided by
    vtkSMProxyManager (all unknown attribute requests are forwarded
    to the vtkSMProxyManager), this class provides several convenience
    methods.

    Please note that some of the methods accessible through the ProxyManager
    class are not listed by help() because the ProxyManager objects forwards
    unresolved attributes to the underlying object. To get the full list,
    see also dir(proxy.SMProxyManager). See also the doxygen based documentation
    of the vtkSMProxyManager C++ class.
    """

    def __init__(self, session=None):
        """Constructor. Assigned self.SMProxyManager to
        vtkSMProxyManager.GetProxyManager()."""
        global ActiveConnection
        if not session:
            session = ActiveConnection.Session
        self.SMProxyManager = session.GetSessionProxyManager()

    def RegisterProxy(self, group, name, aProxy):
        """Registers a proxy (either SMProxy or proxy) with the
        server manager"""
        if isinstance(aProxy, Proxy):
            self.SMProxyManager.RegisterProxy(group, name, aProxy.SMProxy)
        else:
            self.SMProxyManager.RegisterProxy(group, name, aProxy)

    def _get_active_model(self, name):
        model = self.GetSelectionModel(name)
        if not model:
            model = vtkSMProxySelectionModel()
            self.RegisterSelectionModel(name, model)
        return model

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

    def GetProxiesInGroup(self, groupname):
        """Returns a map of proxies in a particular group."""
        proxies = {}
        iter = self.NewGroupIterator(groupname)
        for aProxy in iter:
            proxies[(iter.GetKey(), aProxy.GetGlobalIDAsString())] = aProxy
        return proxies

    def UnRegisterProxy(self, groupname, proxyname, aProxy):
        """Unregisters a proxy."""
        if not self.SMProxyManager:
            return
        if aProxy != None and isinstance(aProxy, Proxy):
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
        iter.Begin()
        return iter

    def NewGroupIterator(self, group_name):
        """Returns a ProxyIterator for a group. The resulting object
        can be used to traverse the proxies that are in the given
        group."""
        iter = self.__iter__()
        iter.SetModeToOneGroup()
        iter.Begin(group_name)
        return iter

    def NewDefinitionIterator(self, groupname=None):
        """Returns an iterator that can be used to iterate over
           all groups and types of proxies that the proxy manager
           can create."""
        iter = None
        if groupname != None:
            iter = ProxyDefinitionIterator(self.GetProxyDefinitionManager().NewSingleGroupIterator(groupname, 0))
        else:
            iter = ProxyDefinitionIterator(self.GetProxyDefinitionManager().NewIterator(0))

        return iter

    def __ConvertArgumentsAndCall(self, *args):
        newArgs = []
        for arg in args:
            if issubclass(type(arg), Proxy) or isinstance(arg, Proxy):
                newArgs.append(arg.SMProxy)
            else:
                newArgs.append(arg)
        func = getattr(self.SMProxyManager, self.__LastAttrName)
        retVal = func(*newArgs)
        if isinstance(retVal, vtkSMProxy):
            return _getPyProxy(retVal)
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

    def LoadState(self, filename, loader=None, location=vtkPVSession.CLIENT):
        self.SMProxyManager.LoadXMLState(filename, loader, location)

    def SaveState(self, filename, location=vtkPVSession.CLIENT):
        self.SMProxyManager.SaveXMLState(filename, location)


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
        self.PropertyLabel = None
        self.Proxy = aProxy

    def __iter__(self):
        return self

    def __next__(self):
        if not self.SMIterator:
            raise StopIteration

        if self.SMIterator.IsAtEnd():
            self.Key = None
            raise StopIteration
        self.Key = self.SMIterator.GetKey()
        self.PropertyLabel = self.SMIterator.GetPropertyLabel()
        self.SMIterator.Next()
        return self.Proxy.GetProperty(self.Key)

    def GetProxy(self):
        """Returns the proxy for the property last returned by the call to
        '__next__()'"""
        return self.Proxy

    def GetKey(self):
        """Returns the key for the property last returned by the call to
        '__next__()' """
        return self.Key

    def GetProperty(self):
        """Returns the property last returned by the call to '__next__()' """
        return self.Proxy.GetProperty(self.Key)

    def __getattr__(self, name):
        """returns attributes from the vtkSMPropertyIterator."""
        return getattr(self.SMIterator, name)


class ProxyDefinitionIterator(object):
    """Wrapper for a vtkPVProxyDefinitionIterator class to satisfy
       the python iterator protocol.
       See the doxygen documentation of the vtkPVProxyDefinitionIterator
       C++ class for more information."""

    def __init__(self, iter):
        self.SMIterator = iter
        if self.SMIterator:
            self.SMIterator.UnRegister(None)
            self.SMIterator.InitTraversal()
        self.Group = None
        self.Key = None
        self.XML = None

    def __iter__(self):
        return self

    def __next__(self):
        if self.SMIterator.IsDoneWithTraversal():
            self.Group = None
            self.Key = None
            raise StopIteration
        self.Group = self.SMIterator.GetGroupName()
        self.Key = self.SMIterator.GetProxyName()
        self.XML = self.SMIterator.GetProxyDefinition()
        self.SMIterator.GoToNextItem()
        return {"group": self.Group, "key": self.Key, "xml": self.XML}

    def GetProxyName(self):
        """Returns the key for the proxy definition last returned by the call
        to '__next__()' """
        return self.Key

    def GetGroup(self):
        """Returns the group for the proxy definition last returned by the
        call to '__next__()' """
        return self.Group

    def GetXML(self):
        """Returns the proxy definition XML (as vtkPVXMLElement)"""
        return self.XML

    def __getattr__(self, name):
        """returns attributes from the vtkPVProxyDefinitionIterator."""
        return getattr(self.SMIterator, name)


class ProxyIterator(object):
    """Wrapper for a vtkSMProxyIterator class to satisfy the
     python iterator protocol.
     See the doxygen documentation of vtkSMProxyIterator C++ class for
     more information.
     """

    def __init__(self):
        self.SMIterator = vtkSMProxyIterator()
        self.SMIterator.SetSession(ActiveConnection.Session)
        self.SMIterator.Begin()
        self.AProxy = None
        self.Group = None
        self.Key = None

    def __iter__(self):
        return self

    def __next__(self):
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
        """Returns the proxy last returned by the call to '__next__()'"""
        return self.AProxy

    def GetKey(self):
        """Returns the key for the proxy last returned by the call to
        '__next__()' """
        return self.Key

    def GetGroup(self):
        """Returns the group for the proxy last returned by the call to
        '__next__()' """
        return self.Group

    def __getattr__(self, name):
        """returns attributes from the vtkSMProxyIterator."""
        return getattr(self.SMIterator, name)


# Caution: Observers must be global methods otherwise we run into memory
#          leak when the interpreter get reset from the C++ layer.
class Connection(object):
    """
      This is a python representation for a session/connection.
    """

    def __init__(self, connectionId, session):
        """Default constructor. Creates a Connection with the given
        ID, all other data members initialized to None."""
        global ActiveConnection
        self.ID = connectionId
        self.Session = session
        self.Alive = True
        self.DefinitionObserverTag = 0
        self.CustomDefinitionObserverTag = 0

        # newly created connection always become the active connection.
        ActiveConnection = self

        # Build the list of available proxies for this connection.
        self.ProxiesNS = ConnectionProxyNamespaces(self.Session)

        ActiveConnection = None
        # SetActiveConnection(self)

    def __eq__(self, other):
        "Returns true if the connection ids are the same."
        # Python3 - this is now called when self is None
        return ((self is None and other is None) or
                ((self is not None and other is not None) and (self.ID == other.ID)))

    def __repr__(self):
        """User friendly string representation"""
        return "Connection (%s) [%d]" % (self.Session.GetURI(), self.ID)

    def GetURI(self):
        """Get URI of the connection"""
        return self.Session.GetURI()

    def IsRemote(self):
        """Returns True if the connection to a remote server, False if
        it is local (built-in)"""
        if self.Session.IsA("vtkSMSessionClient"):
            return True
        return False

    def GetNumberOfDataPartitions(self):
        """Returns the number of partitions on the data server for this
           connection"""
        return self.Session.GetServerInformation().GetNumberOfProcesses()

    def close(self):
        if self.DefinitionObserverTag:
            self.Session.GetProxyDefinitionManager().RemoveObserver(self.DefinitionObserverTag)
            self.Session.GetProxyDefinitionManager().RemoveObserver(self.CustomDefinitionObserverTag)
        self.Session = None
        self.Modules = None
        self.Alive = False

    def __del__(self):
        if self.Alive:
            self.close()


def SaveState(filename, location=vtkPVSession.CLIENT):
    """Given a state filename, saves the state of objects registered
    with the proxy manager."""
    pm = ProxyManager()
    pm.SaveState(filename, location)


def LoadState(filename, connection=None, location=vtkPVSession.CLIENT):
    """Given a state filename and an optional connection, loads the server
    manager state."""
    if not connection:
        connection = ActiveConnection
    if not connection:
        raise RuntimeError("Cannot load state without a connection")
    pm = ProxyManager()
    pm.LoadState(filename, None, location)
    views = GetRenderViews()
    for view in views:
        # Make sure that the client window size matches the
        # ViewSize property. In paraview, the GUI takes care
        # of this.
        if view.GetClassName() == "vtkSMIceTDesktopRenderViewProxy":
            view.GetRenderWindow().SetSize(view.ViewSize[0], \
                                           view.ViewSize[1])


def Connect(ds_host=None, ds_port=11111, rs_host=None, rs_port=22221, timeout=60):
    """
    Use this function call to create a new session. On success,
    it returns a vtkSMSession object that abstracts the connection.
    Otherwise, it returns None.

    There are several ways in which this function can be called:

    * When called with no hosts, it creates a new session
      to the built-in server on the client itself.

    * When called with ds_host and ds_port arguments, it
      attempts to connect to a server(data and render server on the same server)
      on the indicated host:port.

    * When called with ds_host, ds_port, rs_host, rs_port, it
      creates a new connection to the data server on ds_host:ds_port and to the
      render server on rs_host: rs_port.

    * All these connection types support a timeout argument in second.
      Default is 60. 0 means no retry, -1 means infinite retries.
    """
    ret = True;
    if ds_host == None:
        session = vtkSMSession()
    elif rs_host == None:
        session = vtkSMSessionClient()
        ret = session.Connect("cs://%s:%d" % (ds_host, ds_port), timeout)
    else:
        session = vtkSMSessionClient()
        ret = session.Connect("cdsrs://%s:%d/%s:%d" % (ds_host, ds_port, rs_host, rs_port), timeout)
    if ret:
        id = vtkProcessModule.GetProcessModule().RegisterSession(session)
        connection = GetConnectionFromId(id)

        # This shouldn't be needed. However, it's needed for old Python scripts that
        # directly import servermanager.py without simple.py
        SetActiveConnection(connection)
        return connection
    else:
        return None


def ConnectToCatalyst(ds_host='localhost', ds_port=22222):
    """
    Use this function to create a new catalyst session.
    """
    # Create an InsituLink
    insituLink = CreateProxy('coprocessing', 'LiveInsituLink')
    insituLink.GetProperty('InsituPort').SetElement(0, ds_port)
    insituLink.GetProperty('Hostname').SetElement(0, ds_host)
    # set process type to Visualization
    insituLink.GetProperty('ProcessType').SetElement(0, 0)
    insituLink.UpdateVTKObjects()

    # Create dummy session
    id = vtkSMSession.ConnectToCatalyst()
    connection = GetConnectionFromId(id)
    insituLink.SetInsituProxyManager(connection.Session.GetSessionProxyManager())

    return insituLink


def ReverseConnect(port=11111):
    """
    Use this function call to create a new session. On success,
    it returns a Session object that abstracts the connection.
    Otherwise, it returns None.
    In reverse connection mode, the client waits for a connection
    from the server (client has to be started first). The server
    then connects to the client (run pvserver with -rc and -ch
    option).
    The optional port specified the port to listen to.
    """
    session = vtkSMSessionClient()
    session.Connect("csrc://hostname:" + port)
    id = vtkProcessModule.GetProcessModule().RegisterSession(session)
    connection = GetConnectionFromId(id)

    # This shouldn't be needed. However, it's needed for old Python scripts that
    # directly import servermanager.py without simple.py
    SetActiveConnection(connection)
    return connection


def Disconnect(connection=None):
    """Disconnects the connection. Make sure to clear the proxy manager
    first."""
    global ActiveConnection
    if not connection:
        connection = ActiveConnection

    if connection:
        vtkSMSession.Disconnect(connection.ID)


def CreateProxy(xml_group, xml_name, session=None):
    """Creates a proxy. If session is set, the proxy's session is
    set accordingly. If session is None, the current Session is used, if
    present. You should not have to use method normally. Instantiate the
    appropriate class from the appropriate module, for example:
    sph = servermanager.sources.SphereSource()"""
    global ActiveConnection
    if not session:
        session = ActiveConnection.Session
    if not session:
        raise RuntimeError("Cannot create objects without a session.")
    pxm = ProxyManager(session)
    return pxm.NewProxy(xml_group, xml_name)


def GetRenderView(connection=None):
    """Return the render view in use.  If more than one render view is in
    use, return the first one."""

    render_module = None
    for aProxy in ProxyManager():
        if aProxy.IsA("vtkSMRenderViewProxy"):
            render_module = aProxy
            break
    return render_module


def GetRenderViews(connection=None):
    """Returns the set of all render views."""
    render_modules = []
    for aProxy in ProxyManager().GetProxiesInGroup("views").values():
        if aProxy.IsA("vtkSMRenderViewProxy"):
            render_modules.append(aProxy)
    return render_modules


def GetContextViews(connection=None):
    """Returns the set of all context views."""
    context_modules = []
    for aProxy in ProxyManager():
        if aProxy.IsA("vtkSMContextViewProxy"):
            context_modules.append(aProxy)
    return context_modules


def CreateRenderView(session=None, **extraArgs):
    """Creates a render window on the particular session. If session
    is not specified, then the active session is used, if available.

    This method can also be used to initialize properties by passing
    keyword arguments where the key is the name of the property. In addition
    registrationGroup and registrationName (optional) can be specified (as
    keyword arguments) to automatically register the proxy with the proxy
    manager."""
    return _create_view("RenderView", session, **extraArgs)


def _create_view(view_xml_name, session=None, **extraArgs):
    """Creates a view on the particular session. If session
    is not specified, then the active session is used, if available.
    This method can also be used to initialize properties by passing
    keyword arguments where the key is the name of the property."""
    if not session:
        session = ActiveConnection.Session
    if not session:
        raise RuntimeError("Cannot create view without session.")
    pxm = ProxyManager()
    view_module = None
    if view_xml_name:
        view_module = CreateProxy("views", view_xml_name, session)
    if not view_module:
        return None
    return _getPyProxy(view_module)


def GetRepresentation(aProxy, view):
    if view:
        return view.FindRepresentation(aProxy.SMProxy, aProxy.Port)
    return None


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
        raise RuntimeError("proxy argument cannot be None.")
    if not view:
        raise RuntimeError("view argument cannot be None.")
    if "proxyName" in extraArgs:
        display = CreateProxy("representations", extraArgs['proxyName'], None)
        del extraArgs['proxyName']
    else:
        display = view.SMProxy.CreateDefaultRepresentation(aProxy.SMProxy, 0)
        if display:
            display.UnRegister(None)
    if not display:
        return None
    proxy = _getPyProxy(display)
    proxy.Input = aProxy
    proxy.Set(**extraArgs)
    proxy.UpdateVTKObjects()
    view.Representations.append(proxy)
    return proxy


import importlib, importlib.abc


class ParaViewMetaPathFinder(importlib.abc.MetaPathFinder):
    def __init__(self):
        self._loader = ParaViewLoader()

    def find_spec(self, fullname, path, target=None):
        info = vtkPVPythonModule.GetModule(fullname)
        if info:
            package = None
            if info.GetIsPackage():
                package = fullname
            return importlib.machinery.ModuleSpec(fullname, self._loader, is_package=package)
        return None


class ParaViewLoader(importlib.abc.InspectLoader):
    def _info(self, fullname):
        return vtkPVPythonModule.GetModule(fullname)

    def is_package(self, fullname):
        return self._info(fullname).GetIsPackage()

    def get_source(self, fullname):
        return self._info(fullname).GetSource()


def LoadPlugin(filename, remote=True, connection=None):
    """ Given a filename and a session (optional, otherwise uses
    ActiveConnection), loads a plugin. It then updates the sources,
    filters and rendering modules.

    remote=True has no effect when the connection is not remote.
    """

    if not connection:
        connection = ActiveConnection
    if not connection:
        raise RuntimeError("Cannot load a plugin without a connection.")
    plm = vtkSMProxyManager.GetProxyManager().GetPluginManager()

    if remote and connection.IsRemote():
        status = plm.LoadRemotePlugin(filename, connection.Session)
    else:
        status = plm.LoadLocalPlugin(filename)

    # shouldn't the extension check happen before attempting to load the plugin?
    if not status:
        raise RuntimeError("Problem loading plugin %s" % (filename))


def Fetch(input, arg1=None, arg2=None, idx=0):
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
    intermediate results to create a global result.

    Optional argument idx is used to specify the output port number to fetch the
    data from. Default is port 0.
    """

    import sys
    integer_types = (int,)

    reducer = filters.ReductionFilter(Input=OutputPort(input, idx))

    # create the pipeline that reduces and transmits the data
    if arg1 == None:
        dinfo = input.GetDataInformation(idx)
        if dinfo.IsCompositeDataSet():
            paraview.print_debug_info("use composite data append")
            groupDataSets = filters.GroupDatasets()
            groupDataSets.OutputType = dinfo.GetDataSetType()
            groupDataSets.CombineFirstLayerMultiblock = True
            reducer.PostGatherHelper = groupDataSets

        elif dinfo.GetDataClassName() == "vtkPolyData":
            paraview.print_debug_info("use append poly data filter")
            reducer.PostGatherHelperName = "vtkAppendPolyData"

        elif dinfo.GetDataClassName() == "vtkRectilinearGrid":
            paraview.print_debug_info("use append rectilinear grid filter")
            reducer.PostGatherHelperName = "vtkAppendRectilinearGrid"

        elif dinfo.IsA("vtkDataSet"):
            paraview.print_debug_info("use unstructured append filter")
            reducer.PostGatherHelperName = "vtkAppendFilter"

    elif type(arg1) in integer_types:
        reducer.PassThrough = arg1

    else:
        reducer.PreGatherHelper = arg1
        reducer.PostGatherHelper = arg2

    # reduce
    reducer.UpdatePipeline()
    dataInfo = reducer.GetDataInformation(0)
    dataType = dataInfo.GetDataSetType()
    if dataInfo.GetCompositeDataSetType() > 0:
        dataType = dataInfo.GetCompositeDataSetType()

    fetcher = filters.ClientServerMoveData(Input=reducer)
    fetcher.OutputDataType = dataType
    fetcher.WholeExtent = dataInfo.GetExtent()[:]
    # fetch
    fetcher.UpdatePipeline()

    op = fetcher.GetClientSideObject().GetOutputDataObject(0)
    opc = op.NewInstance()
    opc.ShallowCopy(op)
    opc.UnRegister(None)
    return opc


def AnimateReader(reader, view):
    """This is a utility function that, given a reader and a view
    animates over all time steps of the reader.
    It creates an AnimationScene and add a new TimeAnimationCue in it.

    When running from the GUI and python shell, prefer using the
    existing scene and cue to maintain time consistency in the
    application."""
    if not reader:
        raise RuntimeError("No reader was specified, cannot animate.")
    if not view:
        raise RuntimeError("No view was specified, cannot animate.")
    # Create an animation scene
    # This is why it is not recommended to use this method from the GUI. See #18984
    scene = animation.AnimationScene()

    # We need to have the reader and the view registered with
    # the time keeper. This is how the scene gets its time values.
    try:
        tk = next(iter(ProxyManager().GetProxiesInGroup("timekeeper").values()))
        scene.TimeKeeper = tk
    except IndexError:
        tk = misc.TimeKeeper()
        scene.TimeKeeper = tk

    if not reader in tk.TimeSources:
        tk.TimeSources.append(reader)
    if not view in tk.Views:
        tk.Views.append(view)

    # with 1 view
    scene.ViewModules = [view]
    # Update the reader to get the time information
    reader.UpdatePipelineInformation()
    # Animate from 1st time step to last
    scene.StartTime = reader.TimestepValues.GetData()[0]
    scene.EndTime = reader.TimestepValues.GetData()[-1]

    # Each frame will correspond to a time step
    scene.PlayMode = 2  # Snap To Timesteps

    # Create a special animation cue for time.
    cue = animation.TimeAnimationCue()
    cue.AnimatedProxy = view
    cue.AnimatedPropertyName = "ViewTime"
    scene.Cues = [cue]
    scene.Play()
    return scene


def GetProgressPrintingIsEnabled():
    return progressObserverTag is not None


def SetProgressPrintingEnabled(value):
    """Turn on/off printing of progress (by default, it is on). You can
    always turn progress off and add your own observer to the process
    module to handle progress in a custom way. See _printProgress for
    an example event observer."""
    global progressObserverTag

    # If value is true and progress printing is currently off...
    if value and not GetProgressPrintingIsEnabled():
        if paraview.fromGUI:
            raise RuntimeError("Printing progress in the GUI is not supported.")
        if ActiveConnection and ActiveConnection.Session:
            progressObserverTag = ActiveConnection.Session.GetProgressHandler().AddObserver(
                "ProgressEvent", _printProgress)

    # If value is false and progress printing is currently on...
    elif GetProgressPrintingIsEnabled():
        if ActiveConnection and ActiveConnection.Session:
            ActiveConnection.Session.GetProgressHandler().RemoveObserver(progressObserverTag)
            progressObserverTag = None


def ToggleProgressPrinting():
    """Turn on/off printing of progress.  See SetProgressPrintingEnabled."""
    SetProgressPrintingEnabled(not GetProgressPrintingIsEnabled())


def Finalize():
    """Although not required, this can be called at exit to cleanup."""
    global progressObserverTag
    # Make sure to remove the observer
    if progressObserverTag:
        ToggleProgressPrinting()
    vtkInitializationHelper.Finalize()


# Internal methods

def _getPyProxy(smproxy, outputPort=0):
    """Returns a python wrapper for a server manager proxy. This method
    first checks if there is already such an object by looking in the
    _pyproxies group and returns it if found. Otherwise, it creates a
    new one. Proxies register themselves in _pyproxies upon creation."""
    global ActiveConnection, _pyproxies

    if isinstance(smproxy, Proxy):
        # if already a pyproxy, do nothing.
        return smproxy

    if not smproxy:
        return None

    if smproxy.IsA("vtkSMOutputPort"):
        return _getPyProxy(smproxy.GetSourceProxy(), smproxy.GetPortIndex())

    if (smproxy, outputPort) in _pyproxies:
        return _pyproxies[(smproxy, outputPort)]()

    classForProxy = ActiveConnection.ProxiesNS.getClass(smproxy)
    if not classForProxy:
        raise RuntimeError("Failed to locate proxy class for proxy (%s, %s)" % \
                           (smproxy.GetXMLGroup(), smproxy.GetXMLName()))

    return classForProxy(proxy=smproxy, port=outputPort)


def _createInitialize(group, name):
    """Internal method to create an Initialize() method for the sub-classes
    of Proxy"""
    pgroup = group
    pname = name

    def aInitialize(self, connection=None, update=True):
        if not connection:
            connection = ActiveConnection
        if not connection:
            raise RuntimeError('Cannot create a proxy without a session.')
        if not connection.Session.GetProxyDefinitionManager().HasDefinition(pgroup, pname):
            error_msg = "The connection does not provide any definition for %s." % pname
            raise RuntimeError(error_msg)
        self.InitializeFromProxy( \
            CreateProxy(pgroup, pname, connection.Session), update)

    return aInitialize


def _createGetProperty(pName):
    """Internal method to create a GetXXX() method where XXX == pName."""
    propName = pName

    def getProperty(self):
        if paraview.compatibility.GetVersion() >= (3, 5):
            return self.GetPropertyValue(propName)
        else:
            return self.GetProperty(propName)

    return getProperty


def _createSetProperty(pName):
    """Internal method to create a SetXXX() method where XXX == pName."""
    propName = pName

    def setProperty(self, value):
        return self.SetPropertyWithName(propName, value)

    return setProperty


def _printProgress(caller, event):
    """The default event handler for progress. Prints algorithm
    name and 1 '.' per 10% progress."""
    global currentAlgorithm, currentProgress

    pm = vtkProcessModule.GetProcessModule()
    progress = caller.GetLastProgress()
    alg = caller.GetLastProgressText()
    if alg != currentAlgorithm and alg:
        if currentAlgorithm:
            while currentProgress <= 10:
                import sys
                sys.stdout.write(".")
                currentProgress += 1
            print("]")
            currentProgress = 0
        print(alg, ": [ ", end="")
        currentAlgorithm = alg
    while currentProgress <= progress:
        import sys
        sys.stdout.write(".")
        # sys.stdout.write("%d " % pm.GetLastProgress())
        currentProgress += 1
    if progress == 10:
        print("]")
        currentAlgorithm = None
        currentProgress = 0


class ProxyNamespace:
    """
    This class is a container for class types representing known proxies types.
    Conceptually, one can think of this a Python equivalent for a proxy group,
    however, due to legacy reasons, it can represent proxies in more than one
    proxy-group.

    Class types are created on demand. All known proxies types can be determined
    using `dir()` call on this instance. This avoids having to define classes
    for all proxy types during initialization, thus speeding up initialization.
    """

    def __init__(self, xmlGroups, session):
        self.xmlGroups = set(xmlGroups) if xmlGroups else set()
        self.session = session

    @staticmethod
    def _getPyName(xml=None, smproxy=None, proxyname=None):
        """In ProxyNamespace, the proxies are named not using their XML name as
        provided in the definition, but using a sanitized version of their
        label. This change happened in ParaView 3.5. This method returns the
        name to use given a proxy definition XML or a proxy itself based on the
        compatibility version set."""
        assert xml or smproxy or proxyname
        if proxyname:
            name = proxyname
        elif paraview.compatibility.GetVersion() >= (3, 5):
            if xml:
                name = xml.GetAttributeOrDefault("label", xml.GetAttribute("name"))
            elif smproxy:
                name = smproxy.GetXMLLabel()
        else:
            if xml:
                name = xml.GetAttribute("name")
            elif smproxy:
                name = smproxy.GetXMLName()
        assert name
        return _make_name_valid(name)

    def _findProxy(self, name=None, xmlname=None):
        assert name or xmlname
        pdm = self.session.GetProxyDefinitionManager()
        for group in self.xmlGroups:
            pditer = ProxyDefinitionIterator(pdm.NewSingleGroupIterator(group, pdm.ALL_DEFINITIONS))
            for item in pditer:
                if (item["xml"].GetAttribute("name")):
                    pname = self._getPyName(xml=item["xml"])
                else:
                    pname = self._getPyName(proxyname=pditer.GetProxyName())
                if name and pname == name:
                    return item
                elif xmlname and xmlname == item["key"]:
                    return item
        return None

    def __dir__(self):
        s = set()
        pdm = self.session.GetProxyDefinitionManager()
        for group in self.xmlGroups:
            pditer = ProxyDefinitionIterator(pdm.NewSingleGroupIterator(group, pdm.ALL_DEFINITIONS))
            for item in pditer:
                if (item["xml"].GetAttribute("name")):
                    s.add(self._getPyName(xml=item["xml"]))
                else:
                    s.add(self._getPyName(proxyname=pditer.GetProxyName()))
        return s

    def __getattr__(self, name):
        """called when default lookup has failed; this implies that a Proxy type
        is requested which has not been created yet.
        """
        ptype = self._findProxy(name=name)
        if not ptype:
            raise AttributeError("No Proxy type named '%s' found." % name)
        cls = _createClass(ptype["group"], ptype["key"], apxm=self.session.GetSessionProxyManager())
        # store class definition to avoid creating new type for each proxy.
        setattr(self, name, cls)
        return cls

    def getDocumentation(self, name):
        ptype = self._findProxy(name=name)
        if not ptype:
            raise RuntimeError("Invalid proxy '%s'" % name)
        xml = ptype["xml"]
        doc = xml.FindNestedElementByName("Documentation")
        return doc.GetCharacterData() if doc and doc.GetCharacterData() else ""

    def findClass(self, smproxy):
        """Given a vtkSMProxy, returns the class type to use for this proxy
        under this namespace."""

        # shortcut. if the proxy's XML group is not something we are responsible
        # for, skip it.
        if self.xmlGroups and smproxy.GetXMLGroup() not in self.xmlGroups:
            return None

        # determine the 'name' that will be used for the class type for this proxy.
        pname = self._getPyName(smproxy=smproxy)

        # now, attempt to access it.
        return getattr(self, pname) if pname else None


class ConnectionProxyNamespaces(object):
    """
    This class stores class types for all known proxies on a particular session.
    The classes are maintained in separate namespaces accessible as read-only properties
    on an instance of this class.
    """

    def __init__(self, session):
        self._session = session
        self._namespaces = {}

    def _get_proxy_types(self, name, xmlGroups=None):
        if name not in self._namespaces:
            groups = xmlGroups if xmlGroups else [name]
            self._namespaces[name] = ProxyNamespace(groups, self._session)
        return self._namespaces[name]

    @classmethod
    def _getNamespaceNames(cls):
        return [item[0] for item in vars(cls).items() if type(item[1]) == property]

    def getNamespaces(self):
        retval = {}
        for n in self._getNamespaceNames():
            retval[n] = getattr(self, n)
        return retval

    def getClass(self, smproxy):
        """
        Given a vtkSMProxy returns a `Proxy` class subclass type appropriate for
        the proxy. This will atttempt to reuse the class type is already seen,
        otherwise a new one will be created.
        """
        for name in self._getNamespaceNames():
            ns = getattr(self, name)
            cls = ns.findClass(smproxy)
            if cls:
                return cls

        nsname = smproxy.GetXMLGroup()

        # if not found in prefined collections, create a new collection for the
        # group. note, this modifies the class so all existing instances will
        # inherit the new property being added, but that's perfectly acceptable.
        cls = self.__class__
        assert not hasattr(cls, nsname)
        setattr(cls, nsname, property(lambda self: self._get_proxy_types(nsname),
                                      doc="%s proxies" % nsname))
        ns = getattr(self, nsname)
        return ns.findClass(smproxy)

    # Here, we have some predefined proxy groups for which we setup standard
    # namespaces. Besides the groups which had defined that combine multiple xml
    # groups e.g. rendering, animation, and the ones used to create functions
    # e.g sources, filters, writers, I am not sure this necessary anymore.
    sources = property(lambda self: self._get_proxy_types("sources"), doc="source proxies")
    filters = property(lambda self: self._get_proxy_types("filters"), doc="filter proxies")
    writers = property(lambda self: self._get_proxy_types("writers"), doc="writers proxies")
    rendering = property(lambda self: self._get_proxy_types("rendering",
                                                            ["annotations", "cameramanipulators", "lookup_tables",
                                                             "representations", "textures", "views"]),
                         doc="rendering proxies")
    animation = property(lambda self: self._get_proxy_types("animation",
                                                            ["animation", "animation_keyframes"]),
                         doc="animation proxies")
    implicit_functions = property(lambda self: self._get_proxy_types("implicit_functions"),
                                  doc="implicit function proxies")
    piecewise_functions = property(lambda self: self._get_proxy_types("piecewise_functions"),
                                   doc="piece-wise functions")
    extended_sources = property(lambda self: self._get_proxy_types("extended_sources"),
                                doc="extented sources")
    misc = property(lambda self: self._get_proxy_types("misc",
                                                       ["misc", "incremental_point_locators", "point_locators"]),
                    doc="miscellaneous proxies")


def _make_name_valid(name):
    return paraview.make_name_valid(name)


def _createClassProperties(proto, excludeset=frozenset()):
    """Builds a dict of properties for all SMProperties on the `proto` proxy.
    If excludeset is not empty, then it is expected to be names of properties
    to exclude."""
    cdict = {}
    iter = PropertyIterator(proto)
    # Add all properties as python properties.
    for prop in iter:
        propName = iter.GetKey()
        if paraview.compatibility.GetVersion() >= (3, 5):
            if (prop.GetInformationOnly() and propName != "TimestepValues" \
                and prop.GetPanelVisibility() == "never") or prop.GetIsInternal():
                continue
        names = [propName]
        if paraview.compatibility.GetVersion() >= (3, 5):
            names = [iter.PropertyLabel]

        propDoc = None
        if prop.GetDocumentation():
            propDoc = prop.GetDocumentation().GetDescription()
        for name in names:
            name = _make_name_valid(name)
            if name and name not in excludeset:
                cdict[name] = property(_createGetProperty(propName),
                                       _createSetProperty(propName),
                                       None,
                                       propDoc)
    return cdict


def _createClass(groupName, proxyName, apxm=None, prototype=None):
    """Defines a new class type for the proxy."""
    if prototype is None:
        pxm = ProxyManager() if not apxm else apxm
        proto = pxm.GetPrototypeProxy(groupName, proxyName)
    else:
        proto = prototype
    if not proto:
        paraview.print_error("Error while loading %s %s" % (groupName, proxyName))
        return None
    pname = proxyName
    if paraview.compatibility.GetVersion() >= (3, 5) and proto.GetXMLLabel():
        pname = proto.GetXMLLabel()
    pname = _make_name_valid(pname)
    if not pname:
        return None
    cdict = {}
    # Create an Initialize() method for this sub-class.
    cdict['Initialize'] = _createInitialize(groupName, proxyName)
    cdict.update(_createClassProperties(proto))

    # Add the documentation as the class __doc__
    if proto.GetDocumentation() and \
            proto.GetDocumentation().GetDescription():
        doc = proto.GetDocumentation().GetDescription()
    else:
        doc = Proxy.__doc__
    cdict['__doc__'] = doc
    # Create the new type
    if proto.GetXMLName() == "ExodusIIReader":
        superclasses = (ExodusIIReaderProxy,)
    elif proto.IsA("vtkSMRepresentationProxy"):
        superclasses = (RepresentationProxy,)
    elif proto.IsA("vtkSMMultiplexerSourceProxy"):
        superclasses = (MultiplexerSourceProxy,)
    elif proto.IsA("vtkSMSourceProxy"):
        superclasses = (SourceProxy,)
    elif proto.IsA("vtkSMViewLayoutProxy"):
        superclasses = (ViewLayoutProxy,)
    else:
        superclasses = (Proxy,)

    cobj = type(pname, superclasses, cdict)
    return cobj


def __determineGroup(proxy):
    """Internal method"""
    if not proxy:
        return None
    xmlgroup = proxy.GetXMLGroup()
    xmlname = proxy.GetXMLName()
    if xmlgroup == "sources":
        if xmlname in ["BlockSelectionSource",
                       "BlockSelectorsSelectionSource",
                       "SelectionQuerySource",
                       "FrustumSelectionSource",
                       "GlobalIDSelectionSource",
                       "PedigreeIDSelectionSource",
                       "IDSelectionSource",
                       "CompositeDataIDSelectionSource",
                       "HierarchicalDataIDSelectionSource",
                       "ThresholdSelectionSource",
                       "LocationSelectionSource"]:
            return "selection_sources"
        return "sources"
    elif xmlgroup == "filters":
        if xmlname == "AppendSelections":
            return "selection_sources"
        return "sources"
    elif xmlgroup == "representations":
        if xmlname == "ScalarBarWidgetRepresentation":
            return "scalar_bars"
        return "representations"
    elif xmlgroup == "animation_keyframes":
        return "animation"
    return xmlgroup


def ResetSession():
    """Reset the session in the active connection to its initial state."""
    global ActiveConnection

    # Simulate disconnect
    pxm = ProxyManager()
    session = ActiveConnection.Session

    pxm.UnRegisterProxies()

    pm = vtkProcessModule.GetProcessModule()
    pm.UnRegisterSession(session)
    id = pm.RegisterSession(session)
    connection = GetConnectionFromId(id)
    return connection


def demo1():
    """This simple demonstration creates a sphere, renders it and delivers
    it to the client using Fetch. It returns a tuple of (data, render
    view)"""
    if not ActiveConnection:
        Connect()
    if paraview.compatibility.GetVersion() <= (3, 4):
        ss = sources.SphereSource(Radius=2, ThetaResolution=32)
        shr = filters.ShrinkFilter(Input=OutputPort(ss, 0))
        cs = sources.ConeSource()
        app = filters.Append()
    else:
        ss = sources.Sphere(Radius=2, ThetaResolution=32)
        shr = filters.Shrink(Input=OutputPort(ss, 0))
        cs = sources.Cone()
        app = filters.AppendDatasets()
    app.Input = [shr, cs]
    rv = CreateRenderView()
    rep = CreateRepresentation(app, rv)
    rv.ResetCamera()
    rv.StillRender()
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
    # Get the list of point arrays.
    if paraview.compatibility.GetVersion() <= (3, 4):
        arraySelection = reader.PointResultArrayStatus
    else:
        arraySelection = reader.PointVariables
    print(arraySelection.Available)
    # Select all arrays
    arraySelection.SetData(arraySelection.Available)

    # Next create a default render view appropriate for the session type.
    rv = CreateRenderView()
    # Create the matching representation
    rep = CreateRepresentation(reader, rv)
    rep.Representation = 1  # Wireframe
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
    pdi = reader[0].PointData
    # This prints a list of all read point data arrays as well as their
    # value ranges.
    print('Number of point arrays:', len(pdi))
    for i in range(len(pdi)):
        ai = pdi[i]
        print("----------------")
        print("Array:", i, ai.Name, ":")
        numComps = ai.GetNumberOfComponents()
        print("Number of components:", numComps)
        for j in range(numComps):
            if paraview.compatibility.GetVersion() <= (3, 4):
                print("Range:", ai.Range(j))
            else:
                print("Range:", ai.GetRange(j))
    # White is boring. Let's color the geometry using a variable.
    # First create a lookup table. This object controls how scalar
    # values are mapped to colors. See VTK documentation for
    # details.
    lt = rendering.PVLookupTable()
    # Assign it to the representation
    rep.LookupTable = lt
    # Color by point array called Pres
    rep.ColorArrayName = ("POINTS", "Pres")
    # Add to RGB points. These are tuples of 4 values. First one is
    # the scalar values, the other 3 the RGB values. This list has
    # 2 points: Pres: 0.00678, color: blue, Pres: 0.0288, color: red
    lt.RGBPoints = [0.00678, 0, 0, 1, 0.0288, 1, 0, 0]
    lt.ColorSpace = 1  # HSV
    rv.StillRender()
    return rv


def demo3():
    """This method demonstrates the use of servermanager with numpy as
    well as pylab for plotting. It creates an artificial data sources,
    probes it with a line, delivers the result to the client using Fetch
    and plots it using pylab. This demo requires numpy and pylab installed.
    It returns a tuple of (data, render view)."""
    from vtkmodules.util import numpy_support
    import pylab

    if not ActiveConnection:
        Connect()
    # Create a synthetic data source
    if paraview.compatibility.GetVersion() <= (3, 4):
        source = sources.RTAnalyticSource()
    else:
        source = sources.Wavelet()
    # Let's get some information about the data. First, for the
    # source to execute
    source.UpdatePipeline()

    di = source.GetDataInformation()
    print("Data type:", di.GetPrettyDataTypeString())
    print("Extent:", di.GetExtent())
    print("Array name:", \
          source[0].PointData[0].Name)

    rv = CreateRenderView()

    rep1 = CreateRepresentation(source, rv)
    rep1.Representation = 3  # outline

    # Let's apply a contour filter
    cf = filters.Contour(Input=source, ContourValues=[200])

    # Select the array to contour by
    # cf.SelectInputScalars = 'RTData'

    rep2 = CreateRepresentation(cf, rv)

    rv.Background = (0.4, 0.4, 0.6)
    # Reset the camera to include the whole thing
    rv.StillRender()
    rv.ResetCamera()
    rv.StillRender()

    # Now, let's probe the data
    if paraview.compatibility.GetVersion() <= (3, 4):
        probe = filters.Probe(Input=source)
        # with a line
        line = sources.LineSource(Resolution=60)
    else:
        probe = filters.ResampleWithDataset(SourceDataArrays=source)
        # with a line
        line = sources.Line(Resolution=60)
    # that spans the dataset
    bounds = di.GetBounds()
    print("Bounds: ", bounds)
    line.Point1 = bounds[0:6:2]
    line.Point2 = bounds[1:6:2]

    probe.Source = line

    # Render with the line
    rep3 = CreateRepresentation(line, rv)
    rv.StillRender()

    # Now deliver it to the client. Remember, this is for small data.
    data = Fetch(probe)
    # Convert it to a numpy array
    data = numpy_support.vtk_to_numpy(
        data.GetPointData().GetArray("RTData"))
    # Plot it using matplotlib
    pylab.plot(data)
    pylab.show()

    return (data, rv, probe)


def demo4(fname="/Users/berk/Work/ParaViewData/Data/can.ex2"):
    """This method demonstrates the user of AnimateReader for
    creating animations."""
    if not ActiveConnection:
        Connect()
    reader = sources.ExodusIIReader(FileName=fname)
    view = CreateRenderView()
    repr = CreateRepresentation(reader, view)
    view.StillRender()
    view.ResetCamera()
    view.StillRender()
    c = view.GetActiveCamera()
    c.Elevation(95)
    return AnimateReader(reader, view)


def demo5():
    """ Simple sphere animation"""
    if not ActiveConnection:
        Connect()
    if paraview.compatibility.GetVersion() <= (3, 4):
        sphere = sources.SphereSource()
    else:
        sphere = sources.Sphere()
    view = CreateRenderView()
    repr = CreateRepresentation(sphere, view)

    view.StillRender()
    view.ResetCamera()
    view.StillRender()

    # Create an animation scene
    scene = animation.AnimationScene()
    # Add 1 view
    scene.ViewModules = [view]

    # Create a cue to animate the StartTheta property
    cue = animation.KeyFrameAnimationCue()
    cue.AnimatedProxy = sphere
    cue.AnimatedPropertyName = "StartTheta"
    # Add it to the scene's cues
    scene.Cues = [cue]

    # Create 2 keyframes for the StartTheta track
    keyf0 = animation.CompositeKeyFrame()
    keyf0.Type = 2  # Set keyframe interpolation type to Ramp.
    # At time = 0, value = 0
    keyf0.KeyTime = 0
    keyf0.KeyValues = [0]

    keyf1 = animation.CompositeKeyFrame()
    # At time = 1.0, value = 200
    keyf1.KeyTime = 1.0
    keyf1.KeyValues = [200]

    # Add keyframes.
    cue.KeyFrames = [keyf0, keyf1]

    scene.Play()
    return scene


ASSOCIATIONS = {'POINTS': 0, 'CELLS': 1, 'FIELD': 2, 'VERTICES': 4, 'EDGES': 5, 'ROWS': 6}
_LEGACY_ASSOCIATIONS = {'POINT_DATA': 0, 'CELL_DATA': 1}


def GetAssociationAsString(val):
    """Returns array association string from its integer value"""
    global ASSOCIATIONS
    if not type(val) == int:
        raise ValueError("argument must be of type 'int'")
    for k in ASSOCIATIONS:
        if ASSOCIATIONS[k] == val:
            return k
    raise RuntimeError("invalid association type '%d'" % val)


def GetAssociationFromString(val):
    """Returns array association integer value from its string representation"""
    global ASSOCIATIONS, _LEGACY_ASSOCIATIONS
    val = str(val).upper()
    try:
        return ASSOCIATIONS[val]
    except KeyError:
        try:
            return _LEGACY_ASSOCIATIONS[val]
        except KeyError:
            raise RuntimeError("invalid association string '%s'" % val)


# Users can set the active connection which will be used by API
# to create proxies etc when no connection argument is passed.
# Connect() automatically sets this if it is not already set.
ActiveConnection = None

"""Keeps track of all connection objects. Unless the process was run with
--multi-servers flag set to True, this will generally be just 1 item long at the
most."""
Connections = []


def SetActiveConnection(connection=None):
    """Set the active connection. If the process was run without multi-server
       enabled and this method is called with a non-None argument while an
       ActiveConnection is present, it will raise a RuntimeError."""
    global ActiveConnection

    # supports_simutaneous_connections =\
    #    vtkProcessModule.GetProcessModule().GetMultipleSessionsSupport()

    # print ("updating active connection", connection)
    ActiveConnection = connection

    #  This will ensure that servemanager.sources.* will point to the right
    #  constructors.
    __exposeActiveModules__()

    # If this method was initiated from Python, we need to pass that info to the
    # ServerManager.
    session = None
    if ActiveConnection:
        session = ActiveConnection.Session

    # This is a no-op if the session is unchanged.
    pxm = vtkSMProxyManager.GetProxyManager()
    pxm.SetActiveSession(session)
    return ActiveConnection


# Needs to be called when paraview module is loaded from python instead
# of pvpython, pvbatch or GUI. If running in parallel this will
# also set up the vtkMPIController automatically as well. Users
# should specify paraview.options.{batch,symmetric} to be true
# or false to set up ParaView properly. If MPI was initialized, calling
# servermanager.Finalize() may also be needed to exit properly without
# VTK_DEBUG_LEAKS reporting memory leaks.
if not vtkProcessModule.GetProcessModule():
    slist = vtkStringList()
    slist.AddString(sys.executable)

    if paraview.options.batch:
        ptype = vtkProcessModule.PROCESS_BATCH
        if paraview.options.symmetric:
            slist.AddString("--symmetric")
    else:
        ptype = vtkProcessModule.PROCESS_CLIENT
        # ensure that we don't init MPI on the client process
        slist.AddString("--no-mpi")

    if not vtkInitializationHelper.Initialize(slist, ptype):
        raise RuntimeError("Failed to initialize ParaView engine!")

    # In case of non-symmetric mode, if we are a satellite,
    # we should lock right away and wait for the requests
    # from master
    pm = vtkProcessModule.GetProcessModule()
    if not pm.GetSymmetricMPIMode() and pm.GetPartitionId() > 0:
        paraview.options.satellite = True
        sid = vtkSMSession.ConnectToSelf()
        pm.GetGlobalController().ProcessRMIs()
        pm.UnRegisterSession(sid)

    # since we initialized paraview, lets ensure that we finalize it too
    import atexit

    atexit.register(Finalize)

# Initialize progress printing. Can be turned off by calling
# ToggleProgressPrinting() again.
progressObserverTag = None
currentAlgorithm = False
currentProgress = 0
if not paraview.fromGUI:
    ToggleProgressPrinting()

_pyproxies = {}

# Set up our custom importer (if possible)
finder = ParaViewMetaPathFinder()
sys.meta_path.append(finder)


def __exposeActiveModules__():
    """Update servermanager submodules to point to the current
    ActiveConnection.Modules.*"""
    global ActiveConnection

    # Expose all active module to the current servermanager module
    if ActiveConnection:
        g = globals()
        for item in ActiveConnection.ProxiesNS.getNamespaces().items():
            g[item[0]] = item[1]
    # need to delete obsolete groups? that will never happen since when a
    # group is registered, it gets registered for all ConnectionProxyNamespaces
    # instances.


def GetConnectionFromId(id):
    """Returns the Connection object corresponding a connection identified by
       the id."""
    global Connections
    for connection in Connections:
        if connection.ID == id:
            return connection
    return None


def GetConnectionFromSession(session):
    """Returns the Connection object corresponding to a vtkSMSession instance."""
    global Connections
    for connection in Connections:
        if connection.Session == session:
            return connection

    pm = vtkProcessModule.GetProcessModule()
    sid = pm.GetSessionID(session)
    if session and sid:
        # it implies that the we simply may not have received the
        # ConnectionCreatedEvent event yet. Create a connection object.
        c = Connection(sid, session)
        Connections.append(c)
        return c
    return None


def __connectionCreatedCallback(obj, string):
    """Callback called when a new session is created."""
    global Connections
    pm = vtkProcessModule.GetProcessModule()
    sid = pm.GetEventCallDataSessionId()
    # this creates the Connection object if needed.
    GetConnectionFromSession(pm.GetSession(sid))


def __connectionClosedCallback(obg, string):
    """Callback called when a new session is closed."""
    global Connections, ActiveConnection
    pm = vtkProcessModule.GetProcessModule()
    sid = pm.GetEventCallDataSessionId()
    if sid:
        c = GetConnectionFromId(sid)
        if c:
            for cc in range(len(Connections)):
                if Connections[cc] == c:
                    del Connections[cc]
                    break
            c.close()
            del c
    import gc
    gc.collect()


def __initialize():
    """Does initialization of the module, ensuring that the module's state
        correctly reflects that of the ProcessModule/ServerManager."""
    global ActiveConnection, Connections

    # Monitor connection creations/deletions on the ProcessModule.
    pm = vtkProcessModule.GetProcessModule()
    pm.AddObserver("ConnectionCreatedEvent", __connectionCreatedCallback)
    pm.AddObserver("ConnectionClosedEvent", __connectionClosedCallback)

    # Iterate over existing connections, if any, and set the datastructures up.
    iter = vtkProcessModule.GetProcessModule().NewSessionIterator();
    iter.UnRegister(None)
    iter.InitTraversal()
    firstSession = None
    while not iter.IsDoneWithTraversal():
        c = Connection(iter.GetCurrentSessionId(), iter.GetCurrentSession())
        Connections.append(c)
        iter.GoToNextItem()

    # Update active session.
    activeConnection = GetConnectionFromSession( \
        vtkSMProxyManager.GetProxyManager().GetActiveSession())
    SetActiveConnection(activeConnection)


__initialize()

if hasattr(sys, "ps1"):
    # session is interactive.
    paraview.print_debug_info(vtkSMProxyManager.GetParaViewSourceVersion());
