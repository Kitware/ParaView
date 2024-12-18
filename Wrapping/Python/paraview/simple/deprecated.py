import os
from paraview.util import proxy as proxy_util
from paraview import print_warning as pw

# =============================================================================
# Version Messages
# =============================================================================

MSG_REMOVE_6_1 = "This method will be removed in ParaView 6.1+. "

# =============================================================================
# Helpers
# =============================================================================

# Toggle to True only for CI to make sure all deprecated has been updated
REMOVE_DEPRECATION_CALLS = False

# For local testing, you can run ctest with PARAVIEW_DEPRECATION_EXCEPTION=1
WARNING_THROUGH_EXCEPTION = (
    REMOVE_DEPRECATION_CALLS or "PARAVIEW_DEPRECATION_EXCEPTION" in os.environ
)


# Custom ParaView deprecation error
class DeprecationError(RuntimeError):
    """Used for deprecated methods and functions."""

    def __init__(self, message="This feature has been deprecated") -> None:
        RuntimeError.__init__(self, message)


# Helper function to print deprecation warning
def print_warning(msg):

    if WARNING_THROUGH_EXCEPTION:
        raise DeprecationError(msg)

    pw(msg)


# =============================================================================
# Warnings
# =============================================================================


def MakeBlueToRedLT(min, max):
    """
    Create a LookupTable that go from blue to red using the scalar range
    provided by the min and max arguments.

    :param min: Minimum range value.
    :type min: float
    :param max: Maximum range value.
    :type max: float
    :return: the blue-to-red lookup table
    :rtype: Lookup table proxy.
    """
    from paraview.simple.color import CreateLookupTable

    # Define RGB points. These are tuples of 4 values. First one is
    # the scalar values, the other 3 the RGB values.
    print_warning(MSG_REMOVE_6_1)

    rgbPoints = [min, 0, 0, 1, max, 1, 0, 0]

    return CreateLookupTable(RGBPoints=rgbPoints, ColorSpace="HSV")


# -----------------------------------------------------------------------------


def SetProperties(proxy=None, **params):
    """
    Sets one or more properties of the given pipeline source. If an argument
    is not provided, the active source is used. Pass in arguments of the form
    `property_name=value` to this function to set property values. For example::

        SetProperties(Center=[1, 2, 3], Radius=3.5)

    :param proxy: The pipeline source whose properties should be set. Optional,
        defaultst to the active source.
    :type proxy: Source proxy
    :param params: A variadic list of `key=value` pairs giving values of
        specific named properties in the pipeline source. For a list of available
        properties, call `help(proxy)`.
    """

    from paraview.simple.session import GetActiveSource

    print_warning(
        MSG_REMOVE_6_1 + "You should call proxy.Set(Radius=5, Center=[0,0,0]) instead."
    )

    if not proxy:
        proxy = GetActiveSource()

    proxy_util.set(proxy, **params)


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

        GetProperty(name="Radius")
        GetProperty(proxy=sphereProxy, name="Radius")
        GetProperty( sphereProxy, "Radius" )
        GetProperty( "Radius" )
    """
    from paraview.simple.session import GetActiveSource

    print_warning(
        MSG_REMOVE_6_1
        + "You should use Property as attribute like `radius = proxy.Radius` instead."
    )

    name = None
    proxy = None
    for key in keywords:
        if key == "name":
            name = keywords[key]
        if key == "proxy":
            proxy = keywords[key]
    if len(arguments) == 1:
        name = arguments[0]
    if len(arguments) == 2:
        proxy = arguments[0]
        name = arguments[1]
    if not name:
        raise RuntimeError(
            "Expecting at least a property name as input. Otherwise keyword could be used to set 'proxy' and property 'name'"
        )
    if not proxy:
        proxy = GetActiveSource()
    return proxy.GetProperty(name)


# -----------------------------------------------------------------------------


def GetRepresentationProperty(*arguments, **keywords):
    """
    Same as :func:`GetProperty()`, except that if no `proxy` parameter is passed,
    it will use the active representation, rather than the active source.
    """
    from paraview.simple.rendering import GetRepresentation

    print_warning(
        MSG_REMOVE_6_1
        + "You should use Property as attribute like `visibility = simple.GetRepresentation().Visibility` instead."
    )

    proxy = None
    name = None
    for key in keywords:
        if key == "name":
            name = keywords[key]
        if key == "proxy":
            proxy = keywords[key]
    if len(arguments) == 1:
        name = arguments[0]
    if len(arguments) == 2:
        proxy = arguments[0]
        name = arguments[1]
    if not proxy:
        proxy = GetRepresentation()
    return GetProperty(proxy, name)


# -----------------------------------------------------------------------------


def GetDisplayProperty(*arguments, **keywords):
    """DEPRECATED: Should use `GetRepresentationProperty()`"""
    return GetRepresentationProperty(*arguments, **keywords)


# -----------------------------------------------------------------------------


def GetViewProperties(view=None):
    """
    Same as :func:`GetActiveView()`. this API is provided just for consistency with
    :func:`GetDisplayProperties()`.
    """
    from paraview.simple.session import GetActiveView

    print_warning(MSG_REMOVE_6_1 + "You should use simple.GetActiveView() instead")

    return GetActiveView()


# -----------------------------------------------------------------------------


def GetViewProperty(*arguments, **keywords):
    """Same as :func:`GetProperty()`, except that if no 'proxy' parameter is passed,
    it will use the active view properties, rather than the active source."""

    print_warning(
        MSG_REMOVE_6_1
        + "You should use Property as attribute like `orientation = simple.GetActiveView().OrientationAxesVisibility` instead."
    )

    proxy = None
    name = None
    for key in keywords:
        if key == "name":
            name = keywords[key]
        if key == "proxy":
            proxy = keywords[key]
    if len(arguments) == 1:
        name = arguments[0]
    if len(arguments) == 2:
        proxy = arguments[0]
        name = arguments[1]
    if not proxy:
        proxy = GetViewProperties()
    return GetProperty(proxy, name)


# -----------------------------------------------------------------------------


def AssignLookupTable(arrayInfo, lutName, rangeOveride=[]):
    """Assign a lookup table to an array by lookup table name.

    Example usage::

        track = GetAnimationTrack("Center", 0, sphere) or
        arrayInfo = source.PointData["Temperature"]
        AssignLookupTable(arrayInfo, "Cool to Warm")

    :param arrayInfo: The information object for the array. The array name and its
        range is determined using the info object provided.
    :type arrayInfo: `paraview.modules.vtkRemotingCore.vtkPVArrayInformation`
    :param lutName: The name for the transfer function preset.
    :type lutName: str
    :param rangeOveride: The range to use instead of the range of the
        array determined using the `arrayInfo` parameter. Defaults to `[]`,
        meaning the array info range will be used.
    :type rangeOveride: 2-element tuple or list of floats
    :return: `True` on success, `False` otherwise.
    :rtype: bool"""
    from paraview.simple.color import AssignFieldToColorPreset

    print_warning(
        MSG_REMOVE_6_1
        + "You should use `simple.AssignFieldToColorPreset('Temperature', 'Cool To Warm', [-5, 10])` instead."
    )

    return AssignFieldToColorPreset(arrayInfo.Name, lutName, rangeOveride)


# -----------------------------------------------------------------------------


def GetLookupTableNames():
    """Returns a list containing the currently available transfer function
    preset names.

    :return: List of currently availabe transfer function preset names.
    :rtype: List of str"""
    from paraview.simple.color import ListColorPresetNames

    print_warning(
        MSG_REMOVE_6_1 + "You should use `simple.ListColorPresetNames()` instead."
    )

    return ListColorPresetNames()


# -----------------------------------------------------------------------------


def LoadLookupTable(fileName):
    """Load transfer function preset from a file. Both JSON (new) and XML (legacy)
    preset formats are supported.

    :param fileName: If the filename ends with a .xml, it's assumed to be a legacy color
        map XML and will be converted to the new format before processing.
    :type fileName: str
    :return: `True` if successful, `False` otherwise.
    :rtype: bool
    """
    from paraview.simple.color import ImportPresets

    print_warning(MSG_REMOVE_6_1 + "You should use `simple.ImportPresets()` instead.")

    return ImportPresets(fileName)


# ==============================================================================================
# Errors
# ==============================================================================================


def TestDeprecationError():
    raise DeprecationError(
        "replace `print_warning` by `raise DeprecationError` when the version bump happen"
    )
