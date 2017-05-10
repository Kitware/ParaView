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

def setattr(proxy, pname, value):
    """
    Attempts to emulate setattr() when called using a deprecated name for a
    proxy property.

    Will make a reasonable attempt to set the value of the property with the new
    name if the property was deprecated and the paraview compatibility version
    was set to a version older than when the property was deprecated.
    """
    version = paraview.compatibility.GetVersion()

    if pname == "ColorAttributeType" and proxy.SMProxy.GetProperty("ColorArrayName"):
        if paraview.compatibility.GetVersion() <= 4.1:
            # set ColorAttributeType on ColorArrayName property instead.
            caProp = proxy.GetProperty("ColorArrayName")

            proxy.GetProperty("ColorArrayName").SetData((value, caProp[1]))
            raise Continue()
        else:
            # if ColorAttributeType is being used, print debug information.
            paraview.print_debug_info(\
                "'ColorAttributeType' is obsolete. Simply use 'ColorArrayName' instead.  Refer to ParaView Python API changes documentation online.")
            # we let the exception be raised as well, hence don't return here.

    if pname == "AspectRatio" and proxy.SMProxy.GetProperty("ScalarBarThickness"):
        if paraview.compatibility.GetVersion() <= 5.3:
            # We can't do this perfectly, so we set the ScalarBarThickness
            # property instead. Assume a reasonable modern screen size of
            # 1280x1024 with a Render view size of 1000x600. Even if we had
            # access to the View proxy to get the screen size, there is no
            # guarantee that the view size will remain the same later on in the
            # Python script.
            span = 600 # vertical
            if proxy.GetProperty("Orientation").GetData() == "Horizontal":
                span = 1000

            # Assume a scalar bar length 40% of the span.
            thickness = 0.4 * span / value

            proxy.GetProperty("ScalarBarThickness").SetData(int(thickness))
            raise Continue()
        else:
            #if AspectRatio is being used, print debug info
            paraview.print_debug_info(\
                "'AspectRatio' is obsolete. Use the 'ScalarBarThickness' property to set the width instead")
            # we let the exception be raised as well, hence don't return here.

    if pname == "Position2" and proxy.SMProxy.GetProperty("ScalarBarLength"):
        if paraview.compatibility.GetVersion() <= 5.3:
            # The scalar bar length corresponds to Position2[0] when the
            # orientation is horizontal and Position2[1] when the orientation
            # is vertical.
            length = value[0]
            if proxy.Orientation == "Vertical":
                length = value[1]

            proxy.GetProperty("ScalarBarLength").SetData(length)
        else:
            #if Position2 is being used, print debug info
            paraview.print_debug_info(\
                "'Position2' is obsolete. Use the 'ScalarBarLength' property to set the length instead")
            # we let the exception be raised as well, hence don't return here.

    if not hasattr(proxy, pname):
        raise AttributeError()
    proxy.__dict__[pname] = value

    raise Continue()

_fgetattr = getattr

def getattr(proxy, pname):
    """
    Attempts to emulate getattr() when called using a deprecated property name
    for a proxy.

    Will return a *reasonable* stand-in if the property was
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

    # In 5.4, we removed the AspectRatio property and replaced it with the
    # ScalarBarThickness property.
    if pname == "AspectRatio" and proxy.SMProxy.GetProperty("ScalarBarThickness"):
        if version <= 5.3:
            return 20.0
        else:
            raise NotSupportedException(
                    'The AspectRatio property has been removed in ParaView '\
                    '5.4. Please use the ScalarBarThickness property instead '\
                    'to set the thickness in terms of points.')

    # In 5.4, we removed the Position2 property and replaced it with the
    # ScalarBarLength property.
    if pname == "Position2" and proxy.SMProxy.GetProperty("ScalarBarLength"):
        if version <= 5.3:
            if proxy.GetProperty("Orientation").GetData() == "Horizontal":
                return [0.05, proxy.GetProperty("ScalarBarLength").GetData()]
            else:
                return [proxy.GetProperty("ScalarBarLength").GetData(), 0.05]
        else:
            raise NotSupportedException(
                    'The Position2 property has been removed in ParaView '\
                    '5.4. Please set the ScalarBarLength property instead.')

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
