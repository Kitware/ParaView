from __future__ import absolute_import, print_function

from functools import wraps
from xml.dom.minidom import parseString
from xml.parsers.expat import ExpatError
from inspect import getargspec

import sys

def _count(values):
    """internal: returns the `number-of-elements` for the argument.
       if argument is a list or type, it returns its `len`, else returns 0 for None
       and 1 for non-None."""
    if type(values) == list or type(values) == tuple:
        return len(values)
    elif values is None:
        return 0
    else:
        return 1

def _stringify(values):
    """internal method: used to convert values to a string suitable for an xml attribute"""
    if type(values) == list or type(values) == tuple:
        return " ".join([str(x) for x in values])
    elif type(values) == type(True):
        return "1" if values else "0"
    else:
        return str(values)

def _generate_xml(attrs, nested_xmls=[]):
    """internal: used to generate an XML string from the arguments.
    `attrs` is a dict with attributes specified using (key, value) for the dict.
       `type` key in `attrs` is treated as the XML tag name.

    `nested_xmls` is a list of strings that get nested in the resulting xmlstring
        returned by this function.
    """
    d = {}
    d["type"] = attrs.pop("type")

    attr_items = filter(lambda item : item[1] is not None, attrs.items())
    d["attrs"] = "\n".join(["%s=\"%s\"" % (x, _stringify(y)) for x,y in attr_items])
    d["nested_xmls"] = "\n".join(nested_xmls)
    xml = """<{type} {attrs}> {nested_xmls} </{type}>"""
    return xml.format(**d)


def _undecorate(func):
    """internal: Traverses through nested decorated objects to return the original
    object. This is not a general mechanism and only supports decorator chains created
    via `_create_decorator`."""
    if hasattr(func, "_pv_original_func"):
        return _undecorate(func._pv_original_func)
    return func

def _create_decorator(kwargs={}, update_func=None, generate_xml_func=None):
    """internal: used to create decorator for class or function objects.

    `kwargs`: these are typically the keyword arguments passed to the decorator itself.
              must be a `dict`. The decorator will often update the dict to have
              defaults or overrides to the parameters passed to the decorator, as appropriate.

    `update_func`: if non-None, must be a callable that takes 2 arguments `(decoratedobj, kwargs)`.
                   The purpose of this callback is to update the kwargs (and return updated version)
                   for required-yet-missing attributes that can be deduced by introspecting
                   the `decoratedobj`.

    `generate_xml_func`: must be non-None and must be a callable that takes 2 arguments `(decoratedobj, kwargs)`.
                   The kwargs can be expected to have been updated by calling `update_func`, if non-None.
                   The callback typically adds an attribute on the decoratedobj for further processing later.
    """
    attrs = kwargs.copy()
    def decorator(func):
        original_func = _undecorate(func)
        if update_func is not None:
            updated_attrs = update_func(original_func, attrs)
        else:
            updated_attrs = attrs
        generate_xml_func(original_func, updated_attrs)
        @wraps(func)
        def wrapper(*args, **kwargs):
            return func(*args, **kwargs)
        setattr(wrapper, "_pv_original_func", original_func)
        return wrapper
    return decorator


class smproperty(object):
    """
    Provides decorators for class methods that are to be exposed as
    server-manager properties in ParaView. Only methods that are decorated
    using one of the available decorators will be exposed be accessible to ParaView
    UI or client-side Python scripting API.
    """
    @staticmethod
    def _append_xml(func, xml):
        pxmls = []
        if hasattr(func, "_pvsm_property_xmls"):
            pxmls = func._pvsm_property_xmls
        pxmls.append(xml)
        setattr(func, "_pvsm_property_xmls", pxmls)

    @staticmethod
    def _generate_xml(func, attrs):
        nested_xmls = []
        if hasattr(func, "_pvsm_domain_xmls"):
            for d in func._pvsm_domain_xmls:
                nested_xmls.append(d)
            delattr(func, "_pvsm_domain_xmls")
        if hasattr(func, "_pvsm_hints_xmls"):
            hints = []
            for h in func._pvsm_hints_xmls:
                hints.append(h)
            nested_xmls.append(_generate_xml({"type":"Hints"}, hints))
            delattr(func, "_pvsm_hints_xmls")

        pxml = _generate_xml(attrs, nested_xmls)
        smproperty._append_xml(func, pxml)

    @staticmethod
    def _update_property_defaults(func, attrs):
        """Function used to populate default attribute values for missing attributes
           on a for a property"""
        # determine unspecified attributes based on the `func`
        if attrs.get("name", None) is None:
            attrs["name"] = func.__name__
        if attrs.get("command", None) is None:
            attrs["command"] = func.__name__
        return attrs

    @staticmethod
    def _update_vectorproperty_defaults(func, attrs):
        """Function used to populate default attribute values for missing attributes
           on a vector property"""
        attrs = smproperty._update_property_defaults(func, attrs)

        if attrs.get("number_of_elements", None) is None:
            attrs["number_of_elements"] = len(getargspec(func).args) - 1

        if attrs.get("default_values", None) is None:
            attrs["default_values"] = "None"
        else:
            # confirm number_of_elements == len(default_values)
            assert attrs["number_of_elements"] == _count(attrs["default_values"])

        # if repeat_command, set number_of_elements_per_command
        # if not set.
        if attrs.get("repeat_command", None) is not None and \
                attrs.get("number_of_elements_per_command", None) is None:
                    attrs["number_of_elements_per_command"] = len(getargspec(func).args) - 1
        return attrs

    @staticmethod
    def _update_proxyproperty_attrs(func, attrs):
        return _update_property_defaults(func, attrs)

    @staticmethod
    def xml(xmlstr):
        """Decorator that be used to directly add a ServerManager property XML for a method."""
        def generate(func, attrs):
            smproperty._append_xml(func, xmlstr)
        return _create_decorator(generate_xml_func=generate)

    @staticmethod
    def intvector(**kwargs):
        attrs = { "type" : "IntVectorProperty"}
        attrs.update(kwargs)
        return _create_decorator(attrs,
                update_func=smproperty._update_vectorproperty_defaults,
                generate_xml_func=smproperty._generate_xml)

    @staticmethod
    def doublevector(**kwargs):
        attrs = { "type" : "DoubleVectorProperty"}
        attrs.update(kwargs)
        return _create_decorator(attrs,
                update_func=smproperty._update_vectorproperty_defaults,
                generate_xml_func=smproperty._generate_xml)

    @staticmethod
    def idtypevector(**kwargs):
        attrs = { "type" : "IdTypeVectorProperty"}
        attrs.update(kwargs)
        return _create_decorator(attrs,
                update_func=smproperty._update_vectorproperty_defaults,
                generate_xml_func=smproperty._generate_xml)

    @staticmethod
    def stringvector(**kwargs):
        attrs = { "type" : "StringVectorProperty" }
        attrs.update(kwargs)
        return _create_decorator(attrs,
                update_func=smproperty._update_vectorproperty_defaults,
                generate_xml_func=smproperty._generate_xml)

    @staticmethod
    def proxy(**kwargs):
        attrs = { "type" : "ProxyProperty" }
        attrs.update(kwargs)
        return _create_decorator(attrs,
                update_func=smproperty._update_proxyproperty_attrs,
                generate_xml_func=smproperty._generate_xml)

    @staticmethod
    def input(**kwargs):
        attrs = { "type" : "InputProperty" }
        if kwargs.get("multiple_input", False) or kwargs.get("repeat_command", False):
            attrs["command"] = "AddInputConnection"
            # FIXME: input property doesn't support cleaning port connections alone :(
            attrs["clean_command"] = "RemoveAllInputs"
        else:
            attrs["command"] = "SetInputConnection"

        # todo: handle inputType
        attrs.update(kwargs)
        return _create_decorator(attrs,
                update_func=smproperty._update_property_defaults,
                generate_xml_func=smproperty._generate_xml)

    @staticmethod
    def dataarrayselection(name=None):
        def generate(func, attrs):
            xml="""<StringVectorProperty name="{name}Info"
                                         command="{command}"
                                         number_of_elements_per_command="2"
                                         information_only="1"
                                         si_class="vtkSIDataArraySelectionProperty" />
                   <StringVectorProperty name="{name}"
                                         information_property="{name}Info"
                                         command="{command}"
                                         number_of_elements_per_command="2"
                                         element_types="2 0"
                                         repeat_command="1"
                                         si_class="vtkSIDataArraySelectionProperty">
                     <ArraySelectionDomain name="array_list">
                        <RequiredProperties>
                            <Property function="ArrayList" name="{name}Info" />
                        </RequiredProperties>
                     </ArraySelectionDomain>
                   </StringVectorProperty>
            """.format(**attrs)
            smproperty._append_xml(func, xml)
        return _create_decorator({"name" : name},
                update_func=smproperty._update_property_defaults,
                generate_xml_func=generate)


class smdomain(object):
    """
    Provides decorators that add domains to properties.
    """
    @staticmethod
    def _append_xml(func, xml):
        domains = []
        if hasattr(func, "_pvsm_domain_xmls"):
            domains = func._pvsm_domain_xmls
        domains.append(xml)
        setattr(func, "_pvsm_domain_xmls", domains)

    @staticmethod
    def _generate_xml(func, attrs):
        smdomain._append_xml(func, _generate_xml(attrs, []))

    @staticmethod
    def xml(xmlstr):
        def generate(func, attrs):
            smdomain._append_xml(func, xmlstr)

        return _create_decorator({},
                generate_xml_func=generate)

    @staticmethod
    def doublerange(**kwargs):
        attrs = { "type" : "DoubleRangeDomain" , "name" : "range" }
        attrs.update(kwargs)
        return _create_decorator(attrs,
                generate_xml_func=smdomain._generate_xml)

    @staticmethod
    def intrange(**kwargs):
        attrs = { "type" : "IntRangeDomain" , "name" : "range" }
        attrs.update(kwargs)
        return _create_decorator(attrs,
                generate_xml_func=smdomain._generate_xml)

    @staticmethod
    def filelist(**kwargs):
        attrs = { "type" : "FileListDomain", "name" : "files" }
        attrs.update(kwargs)
        return _create_decorator(attrs,
                generate_xml_func=smdomain._generate_xml)

    @staticmethod
    def datatype(dataTypes, **kwargs):
        attrs = {"type" : "DataTypeDomain", "name": "input_type"}
        attrs.update(kwargs)
        def generate(func, attrs):
            type_xmls = []
            for atype in dataTypes:
                type_xmls.append(_generate_xml({"type" : "DataType", "value" : atype}, []))
            smdomain._append_xml(func, _generate_xml(attrs, type_xmls))

        return _create_decorator(attrs,
                generate_xml_func=generate)


class smhint(object):
    """Provides decorators that add hints to proxies and properties."""
    @staticmethod
    def _generate_xml(func, attrs):
        lhints = []
        if hasattr(func, "_pvsm_hints_xmls"):
            lhints = func._pvsm_hints_xmls
        lhints.append(_generate_xml(attrs, []))
        setattr(func, "_pvsm_hints_xmls", lhints)

    @staticmethod
    def xml(xmlstr):
        def generate(func, attrs):
            lhints = []
            if hasattr(func, "_pvsm_hints_xmls"):
                lhints = func._pvsm_hints_xmls
            lhints.append(xmlstr)
            setattr(func, "_pvsm_hints_xmls", lhints)
        return _create_decorator({},
                generate_xml_func=generate)

    @staticmethod
    def filechooser(extensions, file_description):
        attrs = {}
        attrs["type"] = "FileChooser"
        attrs["extensions"] = extensions
        attrs["file_description"] = file_description
        return _create_decorator(attrs,
                generate_xml_func=smhint._generate_xml)

def get_qualified_classname(classobj):
    if classobj.__module__ == "__main__":
        return classobj.__name__
    else:
        return "%s.%s" % (classobj.__module__, classobj.__name__)

class smproxy(object):
    """
    Provides decorators for class objects that should be exposed to
    ParaView.
    """
    @staticmethod
    def _update_proxy_defaults(classobj, attrs):
        if attrs.get("name", None) is None:
            attrs["name"] = classobj.__name__
        if attrs.get("class", None) is None:
            attrs["class"] = get_qualified_classname(classobj)
        if attrs.get("label", None) is None:
            attrs["label"] = attrs["name"]
        return attrs

    @staticmethod
    def _generate_xml(classobj, attrs):
        nested_xmls = []

        classobj = _undecorate(classobj)
        if hasattr(classobj, "_pvsm_property_xmls"):
            val = getattr(classobj, "_pvsm_property_xmls")
            if type(val) == type([]):
                nested_xmls += val
            else:
                nested_xmls.append(val)

        prop_xmls_dict = {}
        for pname, val in classobj.__dict__.items():
            val = _undecorate(val)
            if callable(val) and hasattr(val, "_pvsm_property_xmls"):
                pxmls = getattr(val, "_pvsm_property_xmls")
                if len(pxmls) > 1:
                    raise RuntimeError("Multiple property definitions on the same"\
                            "method are not supported.")
                prop_xmls_dict[pname] = pxmls[0]


        # since the order of the properties keeps on changing between invocations,
        # let's sort them by the name for consistency. In future, we may put in
        # extra logic to preserve order they were defined in the class
        nested_xmls += [prop_xmls_dict[key] for key in sorted(prop_xmls_dict.keys())]

        if attrs.get("support_reload", True):
            nested_xmls.insert(0, """
                <Property name="Reload Python Module" panel_widget="command_button">
                    <Documentation>Reload the Python module.</Documentation>
                </Property>""")

        if hasattr(classobj, "_pvsm_hints_xmls"):
            hints = [h  for h in classobj._pvsm_hints_xmls]
            nested_xmls.append(_generate_xml({"type":"Hints"}, hints))

        proxyxml = _generate_xml(attrs, nested_xmls)
        groupxml = _generate_xml({"type":"ProxyGroup", "name":attrs.get("group")},
                                [proxyxml])
        smconfig = _generate_xml({"type":"ServerManagerConfiguration"}, [groupxml])
        setattr(classobj, "_pvsm_proxy_xml", smconfig)

    @staticmethod
    def source(**kwargs):
        attrs = {}
        attrs["type"] = "SourceProxy"
        attrs["group"] = "sources"
        attrs["si_class"] = "vtkSIPythonSourceProxy"
        attrs.update(kwargs)
        return _create_decorator(attrs,
                update_func=smproxy._update_proxy_defaults,
                generate_xml_func=smproxy._generate_xml)

    @staticmethod
    def filter(**kwargs):
        attrs = { "group" : "filters" }
        attrs.update(kwargs)
        return smproxy.source(**attrs)

    @staticmethod
    def reader(file_description, extensions=None, filename_patterns=None, is_directory=False, **kwargs):
        """
        Decorates a reader. Either `filename_patterns` or `extensions` must be
        provided.
        """
        if extensions is None and filename_patterns is None:
            raise RuntimeError("Either `filename_patterns` or `extensions` must be provided for a reader.")

        attrs = { "type" : "ReaderFactory" }
        attrs["file_description"] = file_description
        attrs["extensions"] = extensions
        attrs["filename_patterns"] = filename_patterns
        attrs["is_directory"] = "1" if is_directory else None
        _xml = _generate_xml(attrs, [])
        def decorator(func):
            f = smhint.xml(_xml)(func)
            return smproxy.source(**kwargs)(f)
        return decorator

    @staticmethod
    def writer(file_description, extensions, **kwargs):
        """
        Decorates a writer.
        """
        assert file_description is not None and extensions is not None

        attrs = { "type" : "WriterFactory" }
        attrs["file_description"] = file_description
        attrs["extensions"] = extensions
        _xml = _generate_xml(attrs, [])
        def decorator(func):
            f = smhint.xml(_xml)(func)
            return smproxy.source(group="writers", type="WriterProxy", **kwargs)(f)
        return decorator


def get_plugin_xmls(module_or_package):
    """helper function called by vtkPVPythonAlgorithmPlugin to discover
    all "proxy" decorated classes in the module or package. We don't recurse
    into the package, on simply needs to export all classes that form the
    ParaView plugin in the __init__.py for the package."""
    from inspect import ismodule, isclass
    items = []
    if ismodule(module_or_package):
        items = module_or_package.__dict__.items()
    elif hasattr(module_or_package, "items"):
        items = module_or_package.items()

    xmls = []
    for (k,v) in items:
        v = _undecorate(v)
        if hasattr(v, "_pvsm_proxy_xml"):
            xmls.append(getattr(v, "_pvsm_proxy_xml"))
    return xmls

def get_plugin_name(module_or_package):
    """helper function called by vtkPVPythonAlgorithmPlugin to discover
    ParaView plugin name, if any."""
    from inspect import ismodule, isclass
    if ismodule(module_or_package) and hasattr(module_or_package, "paraview_plugin_name"):
        return str(getattr(module_or_package, "paraview_plugin_name"))
    else:
        return module_or_package.__name__

def get_plugin_version(module_or_package):
    """helper function called by vtkPVPythonAlgorithmPlugin to discover
    ParaView plugin version, if any."""
    from inspect import ismodule, isclass
    if ismodule(module_or_package) and hasattr(module_or_package, "paraview_plugin_version"):
        return str(getattr(module_or_package, "paraview_plugin_version"))
    else:
        return "(unknown)"

def load_plugin(filepath, default_modulename=None):
    """helper function called by vtkPVPythonAlgorithmPlugin to load
    a python file."""

    # should we scope these under a plugins namespace?
    if default_modulename:
        modulename = default_modulename
    else:
        import os.path
        modulename = "%s" % os.path.splitext(os.path.basename(filepath))[0]

    try:
        # for Python 3.5+
        from importlib.util import spec_from_file_location, module_from_spec
        spec = spec_from_file_location(modulename, filepath)
        module = module_from_spec(spec)
        spec.loader.exec_module(module)
    except ImportError:
        # for Python 3.3 and 3.4
        import imp
        module = imp.load_source(modulename, filepath)

    import sys
    sys.modules[modulename] = module
    return module

def reload_plugin_module(module):
    """helper function to reload a plugin module previously loaded via
    load_plugin"""
    from inspect import getsourcefile, ismodule
    if ismodule(module) and getsourcefile(module):
        return load_plugin(getsourcefile(module), module.__name__)
    return module
