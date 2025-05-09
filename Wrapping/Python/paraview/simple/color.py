import paraview
from paraview import servermanager
from paraview.util import proxy as proxy_util

from paraview.modules.vtkRemotingCore import vtkPVSession

from paraview.simple import compatibility


# -----------------------------------------------------------------------------


def LoadPalette(paletteName):
    """Load a color palette to override the default foreground and background
    colors used by ParaView views. The current global palette's colors are set
    to the colors in the loaded palette.

    :param paletteName: Name of the palette to load from this list:
        'WarmGrayBackground', 'DarkGrayBackground', 'NeutralGrayBackground',
        'LightGrayBackground', 'WhiteBackground', 'BlackBackground',
        'GradientBackground'.
    :type paletteName: str"""
    name = compatibility.GetPaletteName(paletteName)

    pxm = servermanager.ProxyManager()
    palette = pxm.GetProxy("settings", "ColorPalette")
    prototype = pxm.GetPrototypeProxy("palettes", name)

    if palette is None or prototype is None:
        return

    palette.Copy(prototype)
    palette.UpdateVTKObjects()


# -----------------------------------------------------------------------------


def GetColorTransferFunction(arrayname, representation=None, separate=False, **params):
    """Get the color transfer function used to map a data array with the given name to
    colors.

    :param arrayname: Name of the array whose color transfer function is requested.
    :type arrayname: str
    :param representation: Used to modify the array name when using a separate color
        transfer function. Optional, defaults to the active proxy.
    :type representation: Representation proxy.
    :param separate: If `True`, used to recover the separate color transfer function
        even if it is not used currently by the representation. Optional, defaults to
        `False`.
    :type separate: bool
    :return: This may create a new color transfer function if none exists, or return an
        existing one if found.
    :rtype: Color transfer function proxy"""
    if representation:
        if separate or representation.UseSeparateColorMap:
            arrayname = "%s_%s_%s" % (
                "Separate",
                representation.SMProxy.GetGlobalIDAsString(),
                arrayname,
            )
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    lut = servermanager._getPyProxy(
        tfmgr.GetColorTransferFunction(arrayname, session.GetSessionProxyManager())
    )
    proxy_util.set(lut, **params)
    return lut


# -----------------------------------------------------------------------------


def GetBlockColorTransferFunction(
    selector, arrayname, representation=None, separate=False, **params
):
    """Get the color transfer function used to map a data array with the given name to
    colors in the blocks referred to by a selector expresson.

    :param selector: Selector expression used to select blocks from whom the color
        transfer should be retrieved.
    :type selector: str
    :param arrayname: Name of the array whose color transfer function is requested.
    :type arrayname: str
    :param representation: Used to modify the array name when using a separate color
        transfer function. Optional, defaults to the active proxy.
    :type representation: Representation proxy.
    :param separate: If `True`, used to recover the separate color transfer function
        even if it is not used currently by the representation. Optional, defaults to
        `False`.
    :type separate: bool
    :return: This may create a new color transfer function if none exists, or return an
        existing one if found.
    :rtype: Color transfer function proxy"""
    if representation:
        if representation.GetBlockUseSeparateColorMap(selector):
            arrayname = "%s_%s_%s_%s" % (
                "Separate_",
                representation.SMProxy.GetGlobalIDAsString(),
                selector,
                arrayname,
            )
        elif separate or representation.UseSeparateColorMap:
            arrayname = "%s_%s_%s" % (
                "Separate",
                representation.SMProxy.GetGlobalIDAsString(),
                arrayname,
            )
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    lut = servermanager._getPyProxy(
        tfmgr.GetColorTransferFunction(arrayname, session.GetSessionProxyManager())
    )
    proxy_util.set(lut, **params)
    return lut


# -----------------------------------------------------------------------------


def GetOpacityTransferFunction(
    arrayname, representation=None, separate=False, **params
):
    """Get the opacity transfer function used to map a data array with the given name to
    opacities in the blocks referred to by a selector expresson.

    :param arrayname: Name of the array whose opacity transfer function is requested.
    :type arrayname: str
    :param representation: Used to modify the array name when using a separate opacity
        transfer function. Optional, defaults to the active representation.
    :type representation: Representation proxy.
    :param separate: If `True`, used to recover the separate opacity transfer function
        even if it is not used currently by the representation. Optional, defaults to
        `False`.
    :type separate: bool
    :return: This may create a new opacity transfer function if none exists, or return an
        existing one if found.
    :rtype: Opacity transfer function proxy"""
    if representation:
        if separate or representation.UseSeparateColorMap:
            arrayname = "%s%s_%s" % (
                "Separate_",
                representation.SMProxy.GetGlobalIDAsString(),
                arrayname,
            )
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    otf = servermanager._getPyProxy(
        tfmgr.GetOpacityTransferFunction(arrayname, session.GetSessionProxyManager())
    )
    proxy_util.set(otf, **params)
    return otf


# -----------------------------------------------------------------------------


def GetTransferFunction2D(
    arrayname, array2name=None, representation=None, separate=False, **params
):
    """Get the 2D color transfer function used to map a data array with the given name to
    colors.

    :param arrayname: Name of the first array whose color transfer function is requested.
    :type arrayname: str
    :param array2name: Name of the second array whose color transfer function is requested.
    :type array2name: str
    :param representation: Used to modify the array name when using a separate color
        transfer function. Optional, defaults to the active representation.
    :type representation: Representation proxy.
    :param separate: If `True`, used to recover the separate color transfer function
        even if it is not used currently by the representation. Optional, defaults to
        `False`.
    :type separate: bool
    :return: This may create a new 2D color transfer function if none exists, or return an
        existing one if found.
    :rtype: 2D color transfer function proxy"""
    array2name = "" if array2name is None else ("_%s" % array2name)
    if representation:
        if separate or representation.UseSeparateColorMap:
            arrayname = "%s%s_%s%s" % (
                "Separate_",
                representation.SMProxy.GetGlobalIDAsString(),
                arrayname,
                array2name,
            )
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    tfmgr = servermanager.vtkSMTransferFunctionManager()
    tf2d = servermanager._getPyProxy(
        tfmgr.GetTransferFunction2D(arrayname, session.GetSessionProxyManager())
    )
    proxy_util.set(tf2d, **params)
    return tf2d


# -----------------------------------------------------------------------------


def ImportPresets(filename, location=vtkPVSession.CLIENT):
    """Import presets from a file. The file can be in the legacy color map XML
    format or in the new JSON format.

    :param filename: Path of the file containing the presets.
    :type filename: str
    :param location: Where the statefile should be save, e.g., pass `vtkPVSession.CLIENT`
        if the statefile is located on the client system (default value), pass in
        `vtkPVSession.SERVERS` if on the server. Optional, defaults to
        `vtkPVSession.CLIENT`.
    :type location: `vtkPVServer.ServerFlags` enum value
    :return: `True` on success, `False` otherwise."""
    presets = servermanager.vtkSMTransferFunctionPresets.GetInstance()
    return presets.ImportPresets(filename, location)


# -----------------------------------------------------------------------------
def ExportTransferFunction(
    colortransferfunction,
    opacitytransferfunction,
    tfname,
    filename,
    location=vtkPVSession.CLIENT,
):
    """Export transfer function to a file. The file will be saved in the new JSON format.

    :param colortransferfunction: The color transfer function to export.
    :type colortransferfunction: Color transfer function proxy.
    :param opacitytransferfunction: The opacity transfer functon to export, if provided.
        can be None.
    :type opacitytransferfunction: Opacity transfer function proxy.
    :param tfname: The name that will be given to the transfer function preset if imported
        back into ParaView.
    :type tfname: str
    :param filename: Path to file where exported transfer function should be saved.
    :type filename: str
    :param location: Where the statefile should be saved, e.g., pass `vtkPVSession.CLIENT`
        if the statefile is located on the client system (default value), pass in
        `vtkPVSession.SERVERS` if on the server. Optional, defaults to `vtkPVSession.CLIENT`.
    :type location: `vtkPVServer.ServerFlags` enum value
    :return: `True` on success, `False` otherwise."""
    return servermanager.vtkSMTransferFunctionProxy.ExportTransferFunction(
        colortransferfunction.SMProxy,
        opacitytransferfunction.SMProxy,
        tfname,
        filename,
        location,
    )


# -----------------------------------------------------------------------------


def CreateLookupTable(**params):
    """Create and return a lookup table.

    :param params: A variadic list of `key=value` pairs giving values of
        specific named properties of the lookup table.
    :return: Lookup table
    :rtype: Lookup table proxy"""
    lt = servermanager.rendering.PVLookupTable()
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeProxy(lt)
    proxy_util.set(lt, **params)
    controller.RegisterColorTransferFunctionProxy(lt)
    return lt


# -----------------------------------------------------------------------------


def CreatePiecewiseFunction(**params):
    """Create and return a piecewise function.

    :param params: A variadic list of `key=value` pairs giving values of
        specific named properties of the opacity function.
    :return: Piecewise opacity function
    :rtype: Opacity functon proxy"""
    pfunc = servermanager.piecewise_functions.PiecewiseFunction()
    controller = servermanager.ParaViewPipelineController()
    controller.InitializeProxy(pfunc)
    proxy_util.set(pfunc, **params)
    controller.RegisterOpacityTransferFunction(pfunc)
    return pfunc


# -----------------------------------------------------------------------------


def AssignFieldToColorPreset(array_name, preset_name, range_override=None):
    """Assign a lookup table to an array by lookup table name.

    Example usage::

        AssignFieldToPreset("Temperature", "Cool to Warm")

    :param array_name: The array name for which we want to bind a given color preset.
    :type array_name: str
    :param preset_name: The name for the color preset.
    :type preset_name: str
    :param range_override: The range to use instead of the range of the array.
    :type range_override: 2-element tuple or list of floats
    :return: `True` on success, `False` otherwise.
    :rtype: bool"""

    # If the named LUT is not in the presets, see if it was one that was removed and
    # substitute it with the backwards compatibility helper
    presets = servermanager.vtkSMTransferFunctionPresets.GetInstance()
    reverse = False
    if not presets.HasPreset(preset_name):
        (preset_name, reverse) = (
            paraview._backwardscompatibilityhelper.lookupTableUpdate(preset_name)
        )

    # If no alternate LUT exists, raise an exception
    if not presets.HasPreset(preset_name):
        raise RuntimeError("no preset with name `%s` present", preset_name)

    # Preset found. Apply it.
    lut = GetColorTransferFunction(array_name)
    if not lut.ApplyPreset(preset_name):
        return False

    if range_override:
        lut.RescaleTransferFunction(range_override)

    # Reverse if necessary for backwards compatibility
    if reverse:
        lut.InvertTransferFunction()

    return True


# -----------------------------------------------------------------------------


def ListColorPresetNames():
    """Returns a list containing the currently available color transfer function
    preset names.

    :return: List of currently available transfer function preset names.
    :rtype: List of str"""
    presets = servermanager.vtkSMTransferFunctionPresets.GetInstance()
    return [
        presets.GetPresetName(index) for index in range(presets.GetNumberOfPresets())
    ]


# -----------------------------------------------------------------------------


def GenerateRGBPoints(preset_name=None, range_min=None, range_max=None):
    """Create and return a list of RGB points by using the provided arguments.
    The returned RGB points are intended to be used by transfer functions.

    This function is used by Python state files so that they do not have to
    write out every single RGB point for a transfer function if those RGB
    points can be easily re-created.

    :param preset_name: A preset name to apply, if any.
    :type preset_name: str or None
    :param range_min: The minimum value for rescaling the RGBPoints range
    :type range_min: float or None
    :param range_min: The maximum value for rescaling the RGBPoints range
    :type range_max: float or None
    :return: A list of RGBPoints which can be used in transfer functions
    :rtype: list[float]

    NOTE: this function might be used by old state files.
    As such, we must not break backward-compatibility with it.
    """
    # Create a default PVLookupTable
    lut = servermanager.rendering.PVLookupTable()

    # If a preset name was provided, apply the preset
    if preset_name is not None:
        lut.SMProxy.ApplyPreset(preset_name)

    any_rescale = any(x is not None for x in (range_min, range_max))
    if any_rescale:
        if range_min is None:
            range_min = lut.RGBPoints[0]
        if range_max is None:
            range_max = lut.RGBPoints[-4]

        lut.SMProxy.RescaleTransferFunction([range_min, range_max])

    return lut.RGBPoints
