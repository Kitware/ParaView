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

NotSupportedException = paraview.NotSupportedException

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

    For properties that are no longer present on a proxy, the code should do the
    following:

    1. If `paraview.compatibility.GetVersion()` is less than the version in
       which the property was removed, attempt to handle the request and raise
       `Continue` to indicate that the request has been handled. If it is too
       complicated to support the old API, then it is acceptable to raise
       a warning message, but don't raise an exception.

    2. If compatibility version is >= the version in which the property was
       removed, raise `NotSupportedException` with details including suggestions
       to update the script.
    """
    version = paraview.compatibility.GetVersion()

    if pname == "ColorAttributeType" and proxy.SMProxy.GetProperty("ColorArrayName"):
        if paraview.compatibility.GetVersion() <= 4.1:
            # set ColorAttributeType on ColorArrayName property instead.
            caProp = proxy.GetProperty("ColorArrayName")
            proxy.GetProperty("ColorArrayName").SetData((value, caProp[1]))
            raise Continue()
        else:
            # if ColorAttributeType is being used, raise NotSupportedException.
            raise NotSupportedException(
                    "'ColorAttributeType' is obsolete as of ParaView 4.2. Simply use 'ColorArrayName' "\
                    "instead. Refer to ParaView Python API changes documentation online.")

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
            #if AspectRatio is being used, raise NotSupportedException
            raise NotSupportedException(\
                "'AspectRatio' is obsolete as of ParaView 5.4. Use the "\
                "'ScalarBarThickness' property to set the width instead.")

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
            #if Position2 is being used, raise NotSupportedException
            raise NotSupportedException(\
                "'Position2' is obsolete as of ParaView 5.4. Use the "\
                "'ScalarBarLength' property to set the length instead.")

    if pname == "LockScalarRange" and proxy.SMProxy.GetProperty("AutomaticRescaleRangeMode"):
        if paraview.compatibility.GetVersion() <= 5.4:
            if value:
                from paraview.modules.vtkRemotingViews import vtkSMTransferFunctionManager
                proxy.GetProperty("AutomaticRescaleRangeMode").SetData(vtkSMTransferFunctionManager.NEVER)
            else:
                pxm = proxy.SMProxy.GetSessionProxyManager()
                settingsProxy = pxm.GetProxy("settings", "GeneralSettings")
                mode = settingsProxy.GetProperty("TransferFunctionResetMode").GetElement(0)
                proxy.GetProperty("AutomaticRescaleRangeMode").SetData(mode)

            raise Continue()
        else:
            raise NotSupportedException(
                    "'LockScalarRange' is obsolete as of ParaView 5.5. Use "\
                    "'AutomaticRescaleRangeMode' property instead.")

    # In 5.5, we changed the vtkArrayCalculator to use a different set of constants to control which
    # data it operates on.  This change changed the method and property name from AttributeMode to
    # AttributeType
    if pname == "AttributeMode" and proxy.SMProxy.GetXMLName() == "Calculator":
        if paraview.compatibility.GetVersion() <= 5.4:
            # The Attribute type uses enumeration values from vtkDataObject::AttributeTypes
            # rather than custom constants for the calculator.  For the values supported by
            # ParaView before this change, the conversion works out to subtracting 1 if value
            # is an integer. If value is an enumerated string we use that as is since it matches
            # the previous enumerated string options.
            if isinstance(value, int):
                proxy.GetProperty("AttributeType").SetData(value - 1)
            else:
                proxy.GetProperty("AttributeType").SetData(value)
            raise Continue()
        else:
            raise NotSupportedException(\
                "'AttributeMode' is obsolete.  Use 'AttributeType' property of Calculator filter instead.")

    if pname == "UseOffscreenRenderingForScreenshots" and proxy.SMProxy.IsA("vtkSMViewProxy"):
        if paraview.compatibility.GetVersion() <= 5.4:
            raise Continue()
        else:
            raise NotSupportedException(\
                    "'UseOffscreenRenderingForScreenshots' is obsolete. Simply remove it from your script.")
    if pname == "UseOffscreenRendering" and proxy.SMProxy.IsA("vtkSMViewProxy"):
        if paraview.compatibility.GetVersion() <= 5.4:
            raise Continue()
        else:
            raise NotSupportedException(\
                    "'UseOffscreenRendering' is obsolete. Simply remove it from your script.")

    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "CGNSSeriesReader":
        # in 5.5, CGNS reader had some changes. "BaseStatus", "FamilyStatus",
        # "LoadBndPatch", and "LoadMesh" properties were removed.
        if pname in ["LoadBndPatch", "LoadMesh", "BaseStatus", "FamilyStatus"]:
            if paraview.compatibility.GetVersion() <= 5.4:
                paraview.print_warning(\
                  "'%s' is no longer supported and will have no effect. "\
                  "Please use `Blocks` property to specify blocks to load." % pname)
                raise Continue()
            else:
                raise NotSupportedException("'%s' is obsolete. Use the `Blocks` "\
                        "property to select blocks using SIL instead." % pname)

    if pname == "DataBoundsInflateFactor" and proxy.SMProxy.GetProperty("DataBoundsScaleFactor"):
        if paraview.compatibility.GetVersion() <= 5.4:
            # In 5.5, The axes grid data bounds inflate factor have been
            # translated by 1 to become the scale factor.
            proxy.GetProperty("DataBoundsScaleFactor").SetData(value + 1)
        else:
            #if inflat factor is being used, raise NotSupportedException
            raise NotSupportedException(\
                "'DataBoundsInflateFactor' is obsolete as of ParaView 5.5. Use the "\
                "'DataBoundsScaleFactor' property to modify the axes gris data bounds instead.")

    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "AnnotateAttributeData":
        # in 5.5, Annotate Attribute Data changed how it sets the array to annotate
        if pname == "ArrayAssociation":
            if paraview.compatibility.GetVersion() <= 5.4:
                paraview.print_warning(\
                "'ArrayAssociation' is obsolete.  Use 'SelectInputArray' property of AnnotateAttributeData instead.")

                if value == "Point Data":
                    value = "POINTS"
                elif value == "Cell Data":
                    value = "CELLS"
                elif value == "Field Data":
                    value = "FIELD"
                elif value == "Row Data":
                    value = "ROWS"

                arrayProp = proxy.GetProperty("SelectInputArray")
                proxy.GetProperty("SelectInputArray").SetData((value, arrayProp[1]))
                raise Continue()
            else:
                raise NotSupportedException(\
                    "'ArrayAssociation' is obsolete as of ParaView 5.5.  Use 'SelectInputArray' instead.")
        elif pname == "ArrayName":
            if paraview.compatibility.GetVersion() <= 5.4:
                paraview.print_warning(\
                "'ArrayName' is obsolete.  Use 'SelectInputArray' property of AnnotateAttributeData instead.")

                arrayProp = proxy.GetProperty("SelectInputArray")
                proxy.GetProperty("SelectInputArray").SetData((arrayProp[0], value))
                raise Continue()
            else:
                raise NotSupportedException(\
                    "'ArrayName' is obsolete as of ParaView 5.5.  Use 'SelectInputArray' instead.")

    # In 5.5, we changed the Clip to be inverted from what it was before and changed the InsideOut
    # property to be called Invert to be clearer.
    if pname == "InsideOut" and proxy.SMProxy.GetXMLName() == "Clip":
        if paraview.compatibility.GetVersion() <= 5.4:
            proxy.GetProperty("Invert").SetData(1-value)
            raise Continue()
        else:
            raise NotSupportedException(\
                "'InsideOut' is obsolete.  Use 'Invert' property of Clip filter instead.")

    # In 5.6, we changed the "SpreadSheetRepresentation" proxy to no longer have
    # the "FieldAssociation" and "GenerateCellConnectivity" properties. They are
    # now moved to the view.
    if pname in ["FieldAssociation", "GenerateCellConnectivity"] and proxy.SMProxy.GetXMLName() == "SpreadSheetRepresentation":
        if paraview.compatibility.GetVersion() <= 5.5:
            raise Continue()
        else:
            raise NotSupportedException(
                  "'%s' is obsolete on SpreadSheetRepresentation as of ParaView 5.6 and has been migrated to the view." % pname)

    # In 5.7, we changed to the names of the input proxies in ResampleWithDataset to clarify what
    # each source does.
    if pname == "Input" and proxy.SMProxy.GetXMLName() == "ResampleWithDataset":
        if paraview.compatibility.GetVersion() < 5.7:
            proxy.GetProperty("SourceDataArrays").SetData(value)
            raise Continue()
        else:
            raise NotSupportedException(
                'The ResampleWithDataset.Input property has been changed in ParaView 5.7. '\
                'Please set the SourceDataArrays property instead.')

    if pname == "Source" and proxy.SMProxy.GetXMLName() == "ResampleWithDataset":
        if paraview.compatibility.GetVersion() < 5.7:
            proxy.GetProperty("DestinationMesh").SetData(value)
            raise Continue()
        else:
            raise NotSupportedException(
                'The ResampleWithDataset.Source property has been changed in ParaView 5.7. '\
                'Please set the DestinationMesh property instead.')

    # In 5.7, we removed `ArrayName` property on the `GenerateIdScalars` filter
    # and replaced it with `CellIdsArrayName` and `PointIdsArrayName`.
    if pname == "ArrayName" and proxy.SMProxy.GetXMLName() == "GenerateIdScalars":
        if paraview.compatibility.GetVersion() < 5.7:
            proxy.GetProperty("PointIdsArrayName").SetData(value)
            proxy.GetProperty("CellIdsArrayName").SetData(value)
            raise Continue()
        else:
            raise NotSupportedException(
                'The GenerateIdScalars.ArrayName property has been removed in ParaView 5.7. '\
                'Please set `PointIdsArrayName` or `CellIdsArrayName` property instead.')

    # In 5.7, we renamed the 3D View's ray tracing interface from OSPRay to RayTracing
    if pname == "EnableOSPRay" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("EnableRayTracing").SetData(value)
        else:
            raise NotSupportedException(
                'The `EnableOSPRay` control has been renamed in ParaView 5.7 to `EnableRayTracing`.')
    if pname == "OSPRayRenderer" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        newvalue = "OSPRay raycaster"
        if value == "pathtracer":
            newvalue = "OSPRay pathtracer"
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("BackEnd").SetData(newvalue)
        else:
            raise NotSupportedException(
                'The `OSPRayRenderer` control has been renamed in ParaView 5.7 to `BackEnd` and '\
                'the settings `scivis` and `pathtracer` have been renamed to `OSPRay scivis` '\
                'and `OSPRay pathtracer` respectively.')
    if pname == "OSPRayTemporalCacheSize" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("TemporalCacheSize").SetData(value)
        else:
            raise NotSupportedException(
                'The `OSPRayTemporalCacheSize` control has been renamed in ParaView 5.7 to `TemporalCacheSize`.')
    if pname == "OSPRayUseScaleArray" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("UseScaleArray").SetData(value)
        else:
            raise NotSupportedException(
                'The `OSPRayUseScaleArray` control has been renamed in ParaView 5.7 to `UseScaleArray`.')
    if pname == "OSPRayScaleFunction" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("ScaleFunction").SetData(value)
        else:
            raise NotSupportedException(
                'The `OSPRayScaleFunction` control has been renamed in ParaView 5.7 to `ScaleFunction`.')
    if pname == "OSPRayMaterial" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("Material").SetData(value)
        else:
            raise NotSupportedException(
                'The `OSPRayMaterial` control has been renamed in ParaView 5.7 to `Material`.')

    if not hasattr(proxy, pname):
        raise AttributeError()
    proxy.__dict__[pname] = value

    raise Continue()

def setattr_fix_value(proxy, pname, value, setter_func):
    if pname == "ShaderPreset" and proxy.SMProxy.GetXMLName().endswith("Representation"):
        if value == "Gaussian Blur (Default)":
            if paraview.compatibility.GetVersion() <= 5.5:
                paraview.print_warning(\
                    "The 'Gaussian Blur (Default)' option has been renamed to 'Gaussian Blur'.  Please use that instead.")
                setter_func(proxy, "Gaussian Blur")
                raise Continue()
            else:
                raise NotSupportedException("'Gaussian Blur (Default)' is an obsolete value for ShaderPreset. "\
                    " Use 'Gaussian Blur' instead.")

    if pname == "FieldAssociation" and proxy.SMProxy.GetXMLName() in ["DataSetCSVWriter", "CSVWriter"]:
        if paraview.compatibility.GetVersion() < 5.8:
            if value == "Points":
                value = "Point Data"
            elif value == "Cells":
                value = "Cell Data"
            setter_func(proxy, value)
            raise Continue()
        else:
            raise NotSupportedException("'FieldAssociation' is using an obsolete "\
                    "value '%s', use `Point Data` or `Cell Data` instead." % value)

    # In 5.9, we changed "High Resolution Line Source" to "Line" and "Point Source" to
    # "Point Cloud"
    seed_sources_from = ["High Resolution Line Source", "Point Source"]
    seed_sources_to = ["Line", "Point Cloud"]
    seed_sources_proxyname = ["HighResLineSource", "PointSource"]
    if value in seed_sources_from and proxy.GetProperty(pname).SMProperty.IsA("vtkSMInputProperty"):
        domain = proxy.GetProperty(pname).SMProperty.FindDomain("vtkSMProxyListDomain")
        for i in range(len(seed_sources_to)):
            if value == seed_sources_from[i]:
                domain_proxy = domain.FindProxy("extended_sources", seed_sources_proxyname[i])
                if domain_proxy:
                    if paraview.compatibility.GetVersion() < 5.9:
                        value = seed_sources_to[i]
                        setter_func(proxy, value)
                        raise Continue()
                    else:
                        raise NotSupportedException("%s is an obsolete value. Use %s instead." % (seed_sources_from[i], seed_sources_to[i]))

    # Always keep this line last
    raise ValueError("'%s' is not a valid value for %s!" % (value, pname))

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

    # In 5.5, we removed the PVLookupTable.LockScalarRange boolean property and
    # replaced it with the enumeration AutomaticRescaleRangeMode.
    if pname == "LockScalarRange" and proxy.SMProxy.GetProperty("AutomaticRescaleRangeMode"):
        if version <= 5.4:
            from paraview.modules.vtkRemotingViews import vtkSMTransferFunctionManager
            if proxy.GetProperty("AutomaticRescaleRangeMode").GetData() == "Never":
                return 1
            else:
                return 0
        else:
            raise NotSupportedException(
                    'The PVLookupTable.LockScalarRange property has been removed '\
                    'in ParaView 5.5. Please set the AutomaticRescaleRangeMode property '\
                    'instead.')
    # In 5.5, we changed the vtkArrayCalculator to use a different set of constants to control which
    # data it operates on.  This change changed the method and property name from AttributeMode to
    # AttributeType
    if pname == "AttributeMode" and proxy.SMProxy.GetName() == "Calculator":
        if paraview.compatibility.GetVersion() <= 5.4:
            # The Attribute type uses enumeration values from vtkDataObject::AttributeTypes
            # rather than custom constants for the calculator.  For the values supported by
            # ParaView before this change, the conversion works out to adding 1 if it is an
            # integer. If the value is an enumerated string we use that as is since it matches
            # the previous enumerated string options.
            value = proxy.GetProperty("AttributeType").GetData()
            if isinstance(value, int):
                return value + 1
            return value
        else:
            raise NotSupportedException(
                    'The Calculator.AttributeMode property has been removed in ParaView 5.5. '\
                    'Please set the AttributeType property instead. Note that different '\
                    'constants are needed for the two properties.')

    if pname == "UseOffscreenRenderingForScreenshots" and proxy.SMProxy.IsA("vtkSMViewProxy"):
        if version <= 5.4:
            return 0
        else:
            raise NotSupportedException('`UseOffscreenRenderingForScreenshots` '\
                    'is no longer supported. Please remove it.')

    if pname == "UseOffscreenRendering" and proxy.SMProxy.IsA("vtkSMViewProxy"):
        if version <= 5.4:
            return 0
        else:
            raise NotSupportedException(\
                    '`UseOffscreenRendering` is no longer supported. Please remove it.')

    if proxy.SMProxy.GetXMLName() == "CGNSSeriesReader" and\
            pname in ["LoadBndPatch", "LoadMesh", "BaseStatus", "FamilyStatus"]:
        if version < 5.5:
            if pname in ["LoadMesh", "LoadBndPatch"]:
                return 0
            else:
                paraview.print_warning(\
                  "'%s' is no longer supported and will have no effect. "\
                  "Please use `Blocks` property to specify blocks to load." % pname)
                return []
        else:
            raise NotSupportedException(\
              "'%s' is obsolete. Use `Blocks` to make block based selection." % pname)

    # In 5.5, we removed the DataBoundsInflateFactor property and replaced it with the
    # DataBoundsScaleFactor property.
    if pname == "DataBoundsInflateFactor" and proxy.SMProxy.GetProperty("DataBoundsScaleFactor"):
        if version <= 5.4:
            inflateValue = proxy.GetProperty("DataBoundsScaleFactor").GetData() - 1
            if inflateValue >= 0:
                return inflateValue
            else:
                return 0
        else:
            raise NotSupportedException(
                    'The  DataBoundsInflateFactorproperty has been removed in ParaView '\
                    '5.4. Please use the DataBoundsScaleFactor property instead.')

    if proxy.SMProxy and proxy.SMProxy.GetXMLName() == "AnnotateAttributeData":
        # in 5.5, Annotate Attribute Data changed how it sets the array to annotate
        if pname == "ArrayAssociation":
            if paraview.compatibility.GetVersion() <= 5.4:
                paraview.print_warning(\
                "'ArrayAssociation' is obsolete.  Use 'SelectInputArray' property of AnnotateAttributeData instead.")

                value = proxy.GetProperty("SelectInputArray")[0]
                if value == "CELLS":
                    return "Cell Data"
                elif value == "FIELD":
                    return "Field Data"
                elif value == "ROWS":
                    return "Row Data"
                else:
                    return "Point Data"
            else:
                raise NotSupportedException(\
                    "'ArrayAssociation' is obsolete as of ParaView 5.5.  Use 'SelectInputArray' instead.")
        elif pname == "ArrayName":
            if paraview.compatibility.GetVersion() <= 5.4:
                paraview.print_warning(\
                "'ArrayName' is obsolete.  Use 'SelectInputArray' property of AnnotateAttributeData instead.")
                return proxy.GetProperty("SelectInputArray")[1]
            else:
                raise NotSupportedException(\
                    "'ArrayName' is obsolete as of ParaView 5.5.  Use 'SelectInputArray' instead.")

    # In 5.5, we changed the Clip to be inverted from what it was before and changed the InsideOut
    # property to be called Invert to be clearer.
    if pname == "InsideOut" and proxy.SMProxy.GetXMLName() == "Clip":
        if paraview.compatibility.GetVersion() <= 5.4:
            return proxy.GetProperty("Invert").GetData()
        else:
            raise NotSupportedException(
                    'The Clip.InsideOut property has been changed in ParaView 5.5. '\
                    'Please set the Invert property instead.')

    # In 5.6, we changed the "SpreadSheetRepresentation" proxy to no longer have
    # the "FieldAssociation" and "GenerateCellConnectivity" properties. They are
    # now moved to the view.
    if pname in ["FieldAssociation", "GenerateCellConnectivity"] and proxy.SMProxy.GetXMLName() == "SpreadSheetRepresentation":
        if paraview.compatibility.GetVersion() <= 5.5:
            return 0
        else:
            raise NotSupportedException(
                  "'%s' is obsolete on SpreadSheetRepresentation as of ParaView 5.6 and has been migrated to the view." % pname)

    # In 5.7, we changed to the names of the input proxies in ResampleWithDataset to clarify what
    # each source does.
    if pname == "Input" and proxy.SMProxy.GetXMLName() == "ResampleWithDataset":
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("SourceDataArrays")
        else:
            raise NotSupportedException(
                'The ResampleWithDataset.Input property has been changed in ParaView 5.7. '\
                'Please access the SourceDataArrays property instead.')

    if pname == "Source" and proxy.SMProxy.GetXMLName() == "ResampleWithDataset":
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("DestinationMesh")
        else:
            raise NotSupportedException(
                'The ResampleWithDataset.Source property has been changed in ParaView 5.7. '\
                'Please access the DestinationMesh property instead.')

    # In 5.7, we removed `ArrayName` property on the `GenerateIdScalars` filter
    # and replaced it with `CellIdsArrayName` and `PointIdsArrayName`.
    if pname == "ArrayName" and proxy.SMProxy.GetXMLName() == "GenerateIdScalars":
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("PointIdsArrayName")
        else:
            raise NotSupportedException(
                'The GenerateIdScalars.ArrayName property has been removed in ParaView 5.7. ' \
                'Please access `PointIdsArrayName` or `CellIdsArrayName` property instead.')

    # In 5.7, we renamed the 3D View's ray tracing interface from OSPRay to RayTracing
    if pname == "EnableOSPRay" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("EnableRayTracing")
        else:
            raise NotSupportedException(
                'The `EnableOSPRay` control has been renamed in ParaView 5.7 to `EnableRayTracing`.')
    if pname == "OSPRayRenderer" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("BackEnd")
        else:
            raise NotSupportedException(
                'The `OSPRayRenderer` control has been renamed in ParaView 5.7 to `BackEnd` and '\
                'the settings `scivis` and `pathtracer` have been renamed to `OSPRay scivis` '\
                'and `OSPRay pathtracer` respectively.')
    if pname == "OSPRayTemporalCacheSize" and proxy.SMProxy.IsA("vtkSMRenderViewProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("TemporalCacheSize")
        else:
            raise NotSupportedException(
                'The `OSPRayTemporalCacheSize` control has been renamed in ParaView 5.7 to `TemporalCacheSize`.')
    if pname == "OSPRayUseScaleArray" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("UseScaleArray")
        else:
            raise NotSupportedException(
                'The `OSPRayUseScaleArray` control has been renamed in ParaView 5.7 to `UseScaleArray`.')
    if pname == "OSPRayScaleFunction" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("ScaleFunction")
        else:
            raise NotSupportedException(
                'The `OSPRayScaleFunction` control has been renamed in ParaView 5.7 to `ScaleFunction`.')
    if pname == "OSPRayMaterial" and proxy.SMProxy.IsA("vtkSMRepresentationProxy"):
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("Material")
        else:
            raise NotSupportedException(
                'The `OSPRayMaterial` control has been renamed in ParaView 5.7 to `Material`.')

    #  In 5.7, the `Box` implicit function's Scale property was renamed to
    #  Length.
    if pname == "Scale" and proxy.SMProxy.GetXMLName() == "Box":
        if paraview.compatibility.GetVersion() < 5.7:
            return proxy.GetProperty("Length")
        else:
            raise NotSupportedException(
                    'The `Scale` property has been renamed in ParaView 5.7 to `Length`.')

    # In 5.9, CGNSSeriesReader no longer supports the "Blocks" property.
    if pname == "Blocks" and proxy.SMProxy.GetXMLName() == "CGNSSeriesReader":
        if paraview.compatibility.GetVersion() < 5.9:
            return []
        else:
            raise NotSupportedException(
                    "The 'Blocks' property has been removed in ParaView 5.9. Use "
                    "'Bases' to choose bases and 'Families' to choose families "
                    "to load instead. 'LoadMesh' and 'LoadPatches' may also be "
                    "used to enable loading of meshes and BC-patches.")
    raise Continue()

def GetProxy(module, key, **kwargs):
    version = paraview.compatibility.GetVersion()
    if version < 5.2:
        if key == "ResampleWithDataset":
            return module.__dict__["LegacyResampleWithDataset"](**kwargs)
    if version < 5.3:
        if key == "PLYReader":
            # note the case. The old reader didn't support `FileNames` property,
            # only `FileName`.
            return module.__dict__["plyreader"](**kwargs)
    if version < 5.5:
        if key == "Clip":
            # in PV 5.5 we changed the default for Clip's InsideOut property to 1 instead of 0
            # also InsideOut was changed to Invert in 5.5
            clip = module.__dict__[key](**kwargs)
            clip.Invert = 0
            return clip
    if version < 5.6:
        if key == "Glyph":
            # In PV 5.6, we replaced the Glyph filter with a new implementation that has a
            # different set of properties. The previous implementation was renamed to
            # GlyphLegacy.
            print("Creating GlyphLegacy")
            glyph = module.__dict__["GlyphLegacy"](**kwargs)
            print(glyph)
            return glyph
    if version < 5.6:
        if key == "Glyph":
            # In PV 5.6, we replaced the Glyph filter with a new implementation that has a
            # different set of properties. The previous implementation was renamed to
            # GlyphLegacy.
            print("Creating GlyphLegacy")
            glyph = module.__dict__["GlyphLegacy"](**kwargs)
            print(glyph)
            return glyph
    if version < 5.7:
        if key == "ExodusRestartReader" or key == "ExodusIIReader":
            # in 5.7, we changed the names for blocks, this preserves old
            # behavior
            reader = module.__dict__[key](**kwargs)
            reader.UseLegacyBlockNamesWithElementTypes = 1
            return reader
    return module.__dict__[key](**kwargs)

def lookupTableUpdate(lutName):
    """
    Provide backwards compatibility for color lookup table name changes.
    """
    # For backwards compatibility
    reverseLut = False
    version = paraview.compatibility.GetVersion()
    if (version <= 5.8):
        # In 5.9, some redundant color maps were removed and some had name changes.
        # Replace these with a color map that remains. Also handle some color map
        # name changes.
        reverse = ["Red to Blue Rainbow"]
        if lutName in reverse:
            reverseLut = True
        nameChanges = {
            "jet": "Jet",
            "coolwarm": "Cool to Warm",
            "Asymmtrical Earth Tones (6_21b)": "Asymmetrical Earth Tones (6_21b)",
            "CIELab_blue2red": "CIELab Blue to Red",
            "gray_Matlab": "Grayscale",
            "rainbow": "Blue to Red Rainbow",
            "Red to Blue Rainbow": "Blue to Red Rainbow"
        }
        try:
            lutName = nameChanges[lutName]
        except:
            pass

    return (lutName, reverseLut)
