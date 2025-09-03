# =========================================================
# Used by ${ROOT}/Qt/Python/pqPythonCompleter.cxx
# ---------------------------------------------------------
# Expected API:
#  - ListProperties
#  - _get_function_arguments
# =========================================================

from paraview.simple.session import _create_func
from paraview import servermanager, make_name_valid

__all__ = [
    "ListProperties",
    "_get_function_arguments",
]


# -----------------------------------------------------------------------------


def _listProperties(create_func):
    """Internal function that, given a proxy creation function, e.g.,
    paraview.simple.Sphere, returns the list of named properties that can
    be set during construction."""
    key = create_func.__paraview_create_object_key
    module = create_func.__paraview_create_object_module
    prototype_func = _create_func(key, module, skipRegistration=True)

    # Find the prototype by XML label name
    xml_definition = module._findProxy(name=key)
    xml_group = xml_definition["group"]
    xml_name = xml_definition["key"]
    prototype = servermanager.ProxyManager().GetPrototypeProxy(xml_group, xml_name)

    # Iterate over properties and add them to the list
    property_iter = prototype.NewPropertyIterator()
    property_iter.UnRegister(None)
    property_names = []
    while not property_iter.IsAtEnd():
        label = property_iter.GetPropertyLabel()
        if label is None:
            label = property_iter.GetKey()
        property_names.append(make_name_valid(label))
        property_iter.Next()

    return property_names


# -----------------------------------------------------------------------------


def ListProperties(proxyOrCreateFunction):
    """Given a proxy or a proxy creation function, e.g. paraview.simple.Sphere,
    returns the list of properties for the proxy that would be created.

    :param proxyOrCreateFunction: Proxy or proxy creation function whose property
        names are desired.
    :type proxyOrCreateFunction: Proxy or proxy creation function
    :return: List of property names
    :rtype: List of str"""

    try:
        return _listProperties(proxyOrCreateFunction)
    except:
        pass

    try:
        return proxyOrCreateFunction.ListProperties()
    except:
        pass

    return None


# -----------------------------------------------------------------------------


def _get_function_arguments(function):
    """The a list of named arguments for a function.
    Used for autocomplete in PythonShell in ParaView GUI and in pvpython"""
    import inspect

    if not inspect.isfunction(function) and not inspect.ismethod(function):
        raise TypeError(f"input argument: {function} is not a function")
    result = []
    if hasattr(function, "__paraview_create_object_tag") and getattr(
        function, "__paraview_create_object_tag"
    ):
        result = ListProperties(function)
    else:
        for _, parameter in inspect.signature(function).parameters.items():
            # skip *args and **kwargs
            if (
                parameter.kind != inspect.Parameter.VAR_POSITIONAL
                and parameter.kind != inspect.Parameter.VAR_KEYWORD
            ):
                result.append(parameter.name)
    return result
