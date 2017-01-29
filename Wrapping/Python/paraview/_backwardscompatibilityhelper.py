r"""
Internal module used by paraview.servermanager to help warn about properties
changed or removed.

If the compatibility version is less than the version where a particular
property was removed, `check_attr` should ideally continue to work as before
or return a value of appropriate form so old code doesn't fail. Otherwise
`check_attr` should throw the NotSupportedException with appropriate debug
message.
"""

import paraview

class NotSupportedException(Exception):
    def __init__(self, msg):
        self.msg = msg
        paraview.print_debug_info("\nDEBUG: %s\n" % msg)

class Continue(Exception):
    pass

class _CubeAxesHelper(object):
    def __init__(self):
        self.CubeAxesVisibility = 0
        self.CubeAxesColor = [0, 0, 0]
        self.CubeAxesCornerOffset = 0.0
        self.CubeAxesFlyMode = 1
        self.CubeAxesInertia = 1
        self.CubeAxesTickLocation = 0
        self.CubeAxesXAxisMinorTickVisibility = 0
        self.CubeAxesXAxisTickVisibility = 0
        self.CubeAxesXAxisVisibility = 0
        self.CubeAxesXGridLines = 0
        self.CubeAxesXTitle = ""
        self.CubeAxesUseDefaultXTitle = 0
        self.CubeAxesYAxisMinorTickVisibility = 0
        self.CubeAxesYAxisTickVisibility = 0
        self.CubeAxesYAxisVisibility = 0
        self.CubeAxesYGridLines = 0
        self.CubeAxesYTitle = ""
        self.CubeAxesUseDefaultYTitle = 0
        self.CubeAxesZAxisMinorTickVisibility = 0
        self.CubeAxesZAxisTickVisibility = 0
        self.CubeAxesZAxisVisibility = 0
        self.CubeAxesZGridLines = 0
        self.CubeAxesZTitle = ""
        self.CubeAxesUseDefaultZTitle = 0
        self.CubeAxesGridLineLocation = 0
        self.DataBounds = [0, 1, 0, 1, 0, 1]
        self.CustomBounds = [0, 1, 0, 1, 0, 1]
        self.CustomBoundsActive = 0
        self.OriginalBoundsRangeActive = 0
        self.CustomRange = [0, 1, 0, 1, 0, 1]
        self.CustomRangeActive = 0
        self.UseAxesOrigin = 0
        self.AxesOrigin = [0, 0, 0]
        self.CubeAxesXLabelFormat = ""
        self.CubeAxesYLabelFormat = ""
        self.CubeAxesZLabelFormat = ""
        self.StickyAxes = 0
        self.CenterStickyAxes = 0
_ACubeAxesHelper = _CubeAxesHelper()

_fgetattr = getattr

def getattr(proxy, pname):
    """
    Attempts to emulate getattr() when called using a deprecated property name
    for a proxy.

    Will return a *resonable* standin if the property was
    deprecated and the paraview compatibility version was set to a version older
    than when the property was deprecated.

    Will raise ``NotSupportedException`` if the property was deprecated and
    paraview compatibility version is newer than that deprecation version.

    Will raise ``Continue`` to indicate the property name is unaffected by
    any API deprecation and the caller should follow normal code execution
    paths.
    """
    version = paraview.compatibility.GetVersion()

    # In 4.2, we removed ColorAttributeType property. One is expected to use
    # ColorArrayName to specify the attribute type as well.
    if pname == "ColorAttributeType" and proxy.SMProxy.GetProperty("ColorArrayName"):
        if version <= 4.1:
            if proxy.GetProperty("ColorArrayName")[0] == "CELLS":
                return "CELL_DATA"
            else:
                return "POINT_DATA"
        else:
            # if ColorAttributeType is being used, warn.
            raise NotSupportedException(
                "'ColorAttributeType' is obsolete. Simply use 'ColorArrayName' instead.  Refer to ParaView Python API changes documentation online.")

    # In 5.1, we removed CameraClippingRange property. It was not of any use
    # since we added support to render view to automatically reset clipping
    # range for each render.
    if pname == "CameraClippingRange" and not proxy.SMProxy.GetProperty("CameraClippingRange"):
        if version <= 5.0:
            return [0.0, 0.0, 0.0]
        else:
            raise NotSupportedException(
                    'CameraClippingRange is obsolete. Please remove '\
                    'it from your script. You no longer need it.')

    # In 5.1, we remove support for Cube Axes and related properties.
    global _ACubeAxesHelper
    if proxy.SMProxy.IsA("vtkSMPVRepresentationProxy") and hasattr(_ACubeAxesHelper, pname):
        if version <= 5.0:
            return _fgetattr(_ACubeAxesHelper, pname)
        else:
            raise NotSupportedException(
                    'Cube Axes and related properties are now obsolete. Please '\
                    'remove them from your script.')
    raise Continue()

def GetProxy(module, key):
    version = paraview.compatibility.GetVersion()
    if version < 5.2:
        if key == "ResampleWithDataset":
            return module.__dict__["LegacyResampleWithDataset"]()
    if version < 5.3:
        if key == "PLYReader":
            # note the case. The old reader didn't support `FileNames` property,
            # only `FileName`.
            return module.__dict__["plyreader"]()
    return module.__dict__[key]()
