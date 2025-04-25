from paraview.modules.vtkRemotingApplicationComponents import vtkPropertyDecorator
from paraview.vtk.vtkCommonCore import vtkCollection


def _get_decorators_from_hints(sm_proxy, hints):
    output = []
    if not hints:
        return output
    collection = vtkCollection()
    hints.FindNestedElementByName("PropertyWidgetDecorator", collection)
    for config in collection:
        decorator = vtkPropertyDecorator.Create(config, sm_proxy)
        if decorator:
            output.append(decorator)
    return output


def get_decorators(proxy, property_name):
    """Get  a list of all vtkPropertyDecorators associated with this property.
    Decorators are defined in the xml definition of the property with the
    <PropertyWidgetDecorator> tag. A decorator can be assigned directly to a
    property or to a propertygroup that this property belongs to."""

    sm_proxy = proxy.SMProxy
    if not sm_proxy.GetProperty(property_name):
        return None
    sm_property = sm_proxy.GetProperty(property_name)
    hints = sm_proxy.GetProperty(property_name).GetHints()
    output = []
    if hints:
        output = _get_decorators_from_hints(sm_proxy, hints)

    # now let's look into property groups that contain this property for any relevant hints
    n = sm_proxy.GetNumberOfPropertyGroups()
    for i in range(n):
        group = sm_proxy.GetPropertyGroup(i)
        f = group.GetFunction(sm_property)
        if f:
            output += _get_decorators_from_hints(sm_proxy, group.GetHints())

    return output


def should_trace_based_on_decorators(proxy, property_name):
    decorators = get_decorators(proxy, property_name)
    if not decorators:
        return True
    return all([d.CanShow(True) and d.Enable() for d in decorators])


__all__ = ["get_decorators", "should_trace_based_on_decorators"]
