import paraview
from paraview import servermanager
from paraview.util import proxy as proxy_util

from paraview.modules.vtkRemotingCore import vtkPVSession

from paraview.simple.layout import RemoveViewsAndLayouts
from paraview.simple.session import (
    SetActiveView,
    GetActiveSource,
    GetActiveView,
    _create_func,
)
from paraview.simple.view import GetRenderViews
from paraview.simple.animation import GetAnimationScene

# ==============================================================================
# XML State management
# ==============================================================================


def LoadState(
    statefile,
    data_directory=None,
    restrict_to_data_directory=False,
    filenames=None,
    location=vtkPVSession.CLIENT,
    *args,
    **kwargs
):
    """
    Load PVSM state file.

    This will load the state specified in the `statefile`.

    ParaView can update absolute paths for data files used in the state which
    can be useful to portably load state file across different systems.

    Alternatively, `filenames` can be used to specify a list of filenames explicitly.
    This must be list of the following form:

    .. code-block:: json

        [
            {
                // either 'name' or 'id' are required. if both are provided, 'id'
                // is checked first.
                "name" : "[reader name shown in pipeline browser]",
                "id"   : "[reader id used in the pvsm state file]",

                // all modified filename-like properties on this reader
                "FileName" : "..."
                // ...
            }

            // ...
        ]

    Calling this function with other positional or keyword arguments
    will invoke the legacy signature of this function `_LoadStateLegacy`.

    :param statefile: Path to the statefile to load
    :type statefile: str

    :param data_directory: If not `None`, then ParaView searches for files matching
                           those used in the state under the specified directory and if found, replaces
                           the state to use the found files instead. Optional, defaults to `None`.
    :type data_directory: str

    :param restrict_to_data_directory: If set to `True`,  if a file is not found under
                                       the `data_directory`, it will raise an error, otherwise it is left unchanged.
                                       Optional, defaults to `False`.
    :type restrict_to_data_directory: bool

    :param filenames: JSON-formatted string to specify a list of filenames.
    :type filenames: str

    :param location: Where the statefile is located, e.g., pass `vtkPVSession.CLIENT`
                     if the statefile is located on the client system (default value), pass in
                     `vtkPVSession.SERVERS` if on the server. Optional, defaults to client.
    :type location: `vtkPVServer.ServerFlags` enum value

    """
    if kwargs:
        return _LoadStateLegacy(statefile, *args, **kwargs)

    RemoveViewsAndLayouts()

    pxm = servermanager.ProxyManager()
    pyproxy = servermanager._getPyProxy(pxm.NewProxy("options", "LoadStateOptions"))
    if pyproxy.PrepareToLoad(statefile, location):
        pyproxy.LoadStateDataFileOptions = pyproxy.SMProxy.USE_FILES_FROM_STATE
        if pyproxy.HasDataFiles():
            if data_directory is not None:
                pyproxy.LoadStateDataFileOptions = pyproxy.SMProxy.USE_DATA_DIRECTORY
                pyproxy.DataDirectory = data_directory
                if restrict_to_data_directory:
                    pyproxy.OnlyUseFilesInDataDirectory = 1
            elif filenames is not None:
                pyproxy.LoadStateDataFileOptions = (
                    pyproxy.SMProxy.CHOOSE_FILES_EXPLICITLY
                )
                for item in filenames:
                    if not { "name", "FileName" } <= item.keys():
                        keylist = list(item.keys())
                        raise RuntimeError(
                            "filename dictionaries must include 'name' and 'FileName' keys. "
                            "This dictionary had keys {}".format(keylist)
                        )
                    for pname in item.keys():
                        if pname == "name" or pname == "id":
                            continue
                        proxy_id = 0 # indicates no id is available
                        if ("id" in item.keys()):
                            proxy_id = int(item.get("id"))
                        smprop = pyproxy.FindProperty(
                            item.get("name"), proxy_id, pname
                        )
                        if not smprop:
                            raise RuntimeError(
                                "Invalid item specified in 'filenames': %s", item
                            )
                        prop = servermanager._wrap_property(pyproxy, smprop)
                        prop.SetData(item[pname])
        pyproxy.Load()

    # Try to set the new view active
    if len(GetRenderViews()) > 0:
        SetActiveView(GetRenderViews()[0])


def _LoadStateLegacy(filename, connection=None, **extraArgs):
    """Python scripts from version < 5.9 used a different signature. This
    function supports that.

    :param filename: Name of the statefile to load.
    :type filename: str
    :param connection: Unused.
    :type connection: `None`
    """
    RemoveViewsAndLayouts()

    pxm = servermanager.ProxyManager()
    pyproxy = servermanager._getPyProxy(pxm.NewProxy("options", "LoadStateOptions"))
    if (pyproxy is not None) and pyproxy.PrepareToLoad(filename):
        if pyproxy.HasDataFiles() and (extraArgs is not None):
            for pname, value in extraArgs.items():
                smprop = pyproxy.FindLegacyProperty(pname)
                if smprop:
                    if not smprop:
                        raise RuntimeError("Invalid argument '%s'", pname)
                    prop = servermanager._wrap_property(pyproxy, smprop)
                    prop.SetData(value)
                else:
                    pyproxy.__setattr__(pname, value)
        pyproxy.Load()

    # Try to set the new view active
    if len(GetRenderViews()) > 0:
        SetActiveView(GetRenderViews()[0])


# -----------------------------------------------------------------------------


def SaveState(filename, location=vtkPVSession.CLIENT):
    """Save a ParaView statefile (.pvsm) to disk on a system provided by the
    location parameter.

    :param filename: Path where the state file should be saved.
    :type filename: str
    :param location: Where the statefile should be save, e.g., pass `vtkPVSession.CLIENT`
        if the statefile is located on the client system (default value), pass in
        `vtkPVSession.SERVERS` if on the server.
    :type location: `vtkPVServer.ServerFlags` enum value"""
    servermanager.SaveState(filename, location)


# ==============================================================================
# I/O methods
# ==============================================================================


def OpenDataFile(filename, **extraArgs):
    """Creates a reader to read the give file, if possible.
    This uses extension matching to determine the best reader possible.

    :return: Returns a suitable reader if found. If a reader cannot be identified,
             then this returns `None`.
    :rtype: Reader proxy, or `None`

    """
    session = servermanager.ActiveConnection.Session
    reader_factor = servermanager.vtkSMProxyManager.GetProxyManager().GetReaderFactory()
    if reader_factor.GetNumberOfRegisteredPrototypes() == 0:
        reader_factor.UpdateAvailableReaders()
    first_file = filename
    if type(filename) == list:
        first_file = filename[0]
    if not reader_factor.TestFileReadability(first_file, session):
        msg = "File not readable: %s " % first_file
        raise RuntimeError(msg)
    if not reader_factor.CanReadFile(first_file, session):
        msg = "File not readable. No reader found for '%s' " % first_file
        raise RuntimeError(msg)
    prototype = servermanager.ProxyManager().GetPrototypeProxy(
        reader_factor.GetReaderGroup(), reader_factor.GetReaderName()
    )
    xml_name = paraview.make_name_valid(prototype.GetXMLLabel())
    reader_func = _create_func(xml_name, servermanager.sources)
    pname = servermanager.vtkSMCoreUtilities.GetFileNameProperty(prototype)
    if pname:
        extraArgs[pname] = filename
        reader = reader_func(**extraArgs)
    return reader


# -----------------------------------------------------------------------------
def ReloadFiles(proxy=None):
    """Forces a reader proxy to reload the data files.

    :param proxy: Reader proxy whose files should be reloaded. Optional, defaults
        to reloading files in the active source.
    :type proxy: Reader proxy
    :return: Returns `True` if files reloaded successfully, `False` otherwise
    :rtype: bool"""
    if not proxy:
        proxy = GetActiveSource()
    helper = servermanager.vtkSMReaderReloadHelper()
    return helper.ReloadFiles(proxy.SMProxy)


def ExtendFileSeries(proxy=None):
    """For a reader proxy that supports reading files series, detect any new files
    added to the series and update the reader's filename property.

    :param proxy: Reader proxy that should check for a extended file series. Optional,
        defaults to extending file series in the active source.
    :return: `True` if the operation succeeded, `False` otherwise
    :rtype: bool"""
    if not proxy:
        proxy = GetActiveSource()
    helper = servermanager.vtkSMReaderReloadHelper()
    return helper.ExtendFileSeries(proxy.SMProxy)


# -----------------------------------------------------------------------------
def ReplaceReaderFileName(readerProxy, files, propName):
    """Replaces the readerProxy by a new one given a list of files.

    :param readerProxy: Reader proxy whose filename should be updated.
    :type readerProxy: Reader proxy.
    :param files: List of file names to be read by the reader.
    :type files: list of str
    :param propName: should be "FileNames" or "FileName" depending on the property
        name in the reader.
    :type propName: str"""
    servermanager.vtkSMCoreUtilities.ReplaceReaderFileName(
        readerProxy.SMProxy, files, propName
    )


# -----------------------------------------------------------------------------
def CreateWriter(filename, proxy=None, **extraArgs):
    """Creates a writer that can write the data produced by the source proxy in
    the given file format (identified by the extension). This function doesn't
    actually write the data, it simply creates the writer and returns it

    :param filename: The name of the output data file.
    :type filename: str
    :param proxy: Source proxy whose output should be written. Optional, defaults to
        the creating a writer for the active source.
    :type proxy: Source proxy.
    :param extraArgs: Additional arguments that should be passed to the writer
        proxy.
    :type extraArgs: A variadic list of `key=value` pairs giving values of
        specific named properties in the writer."""
    if not filename:
        raise RuntimeError("filename must be specified")
    session = servermanager.ActiveConnection.Session
    writer_factory = (
        servermanager.vtkSMProxyManager.GetProxyManager().GetWriterFactory()
    )
    if writer_factory.GetNumberOfRegisteredPrototypes() == 0:
        writer_factory.UpdateAvailableWriters()
    if not proxy:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError("Could not locate source to write")
    writer_proxy = writer_factory.CreateWriter(filename, proxy.SMProxy, proxy.Port)
    writer_proxy.UnRegister(None)
    pyproxy = servermanager._getPyProxy(writer_proxy)
    if pyproxy and extraArgs:
        proxy_util.set(pyproxy, **extraArgs)
    return pyproxy


def SaveData(filename, proxy=None, **extraArgs):
    """Save data produced by the `proxy` parameter into a file.

    Properties to configure the writer can be passed in
    as keyword arguments. Example usage::

        SaveData("sample.pvtp", source0)
        SaveData("sample.csv", FieldAssociation="Points")

    :param filenam: Path where output file should be saved.
    :type filename: str
    :param proxy: Proxy to save. Optional, defaults to saving the active source.
    :type proxy: Source proxy.
    :param extraArgs: A variadic list of `key=value` pairs giving values of
        specific named properties in the writer."""
    writer = CreateWriter(filename, proxy, **extraArgs)
    if not writer:
        raise RuntimeError("Could not create writer for specified file or data type")
    writer.UpdateVTKObjects()
    writer.UpdatePipeline()
    del writer


# -----------------------------------------------------------------------------
def _SaveScreenshotLegacy(
    filename, view=None, layout=None, magnification=None, quality=None, **params
):
    """Legacy funcion for saving a screenshot.

    :param filename: Path where output screenshot should be saved.
    :type filename: str
    :param view: View proxy. Optional, defaults to using the active view.
    :type view: View proxy
    :param layout: Layout proxy containing the view. Optional, defaults to using the active
        layout.
    :type layout: Layout proxy.
    :param magnification: Integer magnification factor for the screenshot. Optional,
        defaults to no magnification.
    :type magnification: int
    :param quality: Integer quality level for the saved file. Optional, defaults
        to 100, or highest quality.
    :type quality: int
    :param params: A variadic list of `key=value` pairs giving values of
        named properties in the screenshot writer proxy.
    :return: `True` if writing was successful, `False` otherwise.
    :rtype: bool"""
    if view is not None and layout is not None:
        raise ValueError("both view and layout cannot be specified")

    viewOrLayout = view if view else layout
    viewOrLayout = viewOrLayout if viewOrLayout else GetActiveView()
    if not viewOrLayout:
        raise ValueError("view or layout needs to be specified")
    try:
        magnification = int(magnification) if int(magnification) > 0 else 1
    except TypeError:
        magnification = 1
    try:
        quality = max(0, min(100, int(quality)))
    except TypeError:
        quality = 100

    # convert magnification to image resolution.
    if viewOrLayout.IsA("vtkSMViewProxy"):
        size = viewOrLayout.ViewSize
    else:
        assert viewOrLayout.IsA("vtkSMViewLayoutProxy")
        exts = [0] * 4
        viewOrLayout.GetLayoutExtent(exts)
        size = [exts[1] - exts[0] + 1, exts[3] - exts[2] + 1]

    imageResolution = (size[0] * magnification, size[1] * magnification)

    import os.path

    _, extension = os.path.splitext(filename)
    if extension == ".jpg":
        return SaveScreenshot(
            filename, viewOrLayout, ImageResolution=imageResolution, Quality=quality
        )
    elif extension == ".png":
        compression = int(((quality * 0.01) - 1.0) * -9.0)
        return SaveScreenshot(
            filename,
            viewOrLayout,
            ImageResolution=imageResolution,
            CompressionLevel=compression,
        )
    else:
        return SaveScreenshot(filename, viewOrLayout, ImageResolution=imageResolution)


def SaveScreenshot(
    filename,
    viewOrLayout=None,
    saveInBackground=False,
    location=vtkPVSession.CLIENT,
    **params
):
    """Save screenshot for a view or layout (collection of views) to an image.

    `SaveScreenshot` is used to save the rendering results to an image.

    :param filename: Name of the image file to save to. The filename extension is
        used to determine the type of image file generated. Supported extensions are
        `png`, `jpg`, `tif`, `bmp`, and `ppm`.
    :type filename: str
    :param viewOrLayout: The view or layout to save image from, defaults to `None`.
        If `None`, then the active view is used, if available. To save image from
        a single view, this must be set to a view, to save an image from all views in a
        layout, pass the layout.
    :type viewOrLayout: View or layout proxy. Optional.
    :param saveInBackground: If set to `True`, the screenshot will be saved by a
        different thread and run in the background. In such circumstances, one can
        wait until the file is written by calling :func:`WaitForScreenshot(filename)`.
    :type saveInBackground: bool
    :param location: Location where the screenshot should be saved. This can be one
        of the following values: `vtkPVSession.CLIENT`, `vtkPVSession.DATA_SERVER`.
        Optional, defaults to `vtkPVSession.CLIENT`.
    :type location: `vtkPVSession.ServerFlags` enum

    **Keyword Parameters (optional)**

        ImageResolution (tuple(int, int))
            A 2-tuple to specify the output image resolution in pixels as
            `(width, height)`. If not specified, the view (or layout) size is
            used.

        FontScaling (str)
            Specify whether to scale fonts proportionally (`"Scale fonts
            proportionally"`) or not (`"Do not scale fonts"`). Defaults to
            `"Scale fonts proportionally"`.

        SeparatorWidth (int)
            When saving multiple views in a layout, specify the width (in
            approximate pixels) for a separator between views in the generated
            image.

        SeparatorColor (tuple(float, float, float))
            Specify the color for separator between views, if applicable.

        OverrideColorPalette (:obj:str, optional)
            Name of the color palette to use, if any. If none specified, current
            color palette remains unchanged.

        StereoMode (str)
            Stereo mode to use, if any. Available values are `"No stereo"`,
            `"Red-Blue"`, `"Interlaced"`, `"Left Eye Only"`, `"Right Eye Only"`,
            `"Dresden"`, `"Anaglyph"`, `"Checkerboard"`,
            `"Side-by-Side Horizontal"`, and the default `"No change"`.

        TransparentBackground (int)
            Set to 1 (or True) to save an image with background set to alpha=0, if
            supported by the output image format.

    In addition, several format-specific keyword parameters can be specified.
    The format is chosen based on the file extension.

    For JPEG (`*.jpg`), the following parameters are available (optional)

        Quality (int) [0, 100]
            Specify the JPEG compression quality. `O` is low quality (maximum compression)
            and `100` is high quality (least compression).

        Progressive (int):
            Set to 1 (or True) to save progressive JPEG.

    For PNG (`*.png`), the following parameters are available (optional)

        CompressionLevel (int) [0, 9]
            Specify the *zlib* compression level. `0` is no compression, while `9` is
            maximum compression.

    **Legacy Parameters**

        Prior to ParaView version 5.4, the following parameters were available
        and are still supported. However, they cannot be used together with
        other keyword parameters documented earlier.

        view (proxy)
            Single view to save image from.

        layout (proxy)
            Layout to save image from.

        magnification (int)
            Magnification factor to use to save the output image. The current view
            (or layout) size is scaled by the magnification factor provided.

        quality (int)
            Output image quality, a number in the range [0, 100].
    """
    # Let's handle backwards compatibility.
    # Previous API for this method took the following arguments:
    # SaveScreenshot(filename, view=None, layout=None, magnification=None, quality=None)
    # If we notice any of the old arguments, call legacy method.
    if "magnification" in params or "quality" in params:
        if viewOrLayout is not None and not "view" in params:
            # since in previous variant, view could have been passed as a
            # positional argument, we handle it.
            params["view"] = viewOrLayout
        return _SaveScreenshotLegacy(filename, **params)

    # sometimes users love to pass 'view' or 'layout' as keyword arguments
    # even though the signature for this function doesn't support it. let's
    # handle that, it's easy enough.
    if viewOrLayout is None:
        if "view" in params:
            viewOrLayout = params["view"]
            del params["view"]
        elif "layout" in params:
            viewOrLayout = params["layout"]
            del params["layout"]

    # use active view if no view or layout is specified.
    viewOrLayout = viewOrLayout if viewOrLayout else GetActiveView()

    if not viewOrLayout:
        raise ValueError("A view or layout must be specified.")

    controller = servermanager.ParaViewPipelineController()
    options = servermanager.misc.SaveScreenshot()
    controller.PreInitializeProxy(options)

    options.SaveInBackground = saveInBackground
    options.Layout = viewOrLayout if viewOrLayout.IsA("vtkSMViewLayoutProxy") else None
    options.View = viewOrLayout if viewOrLayout.IsA("vtkSMViewProxy") else None
    options.SaveAllViews = True if viewOrLayout.IsA("vtkSMViewLayoutProxy") else False

    # this will choose the correct format.
    options.UpdateDefaultsAndVisibilities(filename)

    controller.PostInitializeProxy(options)

    # explicitly process format properties.
    formatProxy = options.Format
    formatProperties = formatProxy.ListProperties()
    for prop in formatProperties:
        if prop in params:
            formatProxy.SetPropertyWithName(prop, params[prop])
            del params[prop]

    proxy_util.set(options, **params)
    return options.WriteImage(filename, location)


# -----------------------------------------------------------------------------
def SetNumberOfCallbackThreads(n):
    """Sets the number of threads used by the threaded callback queue that can be
    used for saving screenshots.

    :parameter n: Number of callback threads.
    :type n: int"""
    paraview.modules.vtkRemotingSetting.GetInstance().SetNumberOfCallbackThreads(n)


# -----------------------------------------------------------------------------
def GetNumberOfCallbackThreads(n):
    """Gets the number of threads used by the threaded callback queue that can be
    used for saving screenshots.

    :parameter n: Not used
    :type n: int"""
    paraview.modules.vtkRemotingSetting.GetInstance().GetNumberOfCallbackThreads()


# -----------------------------------------------------------------------------
def SetNumberOfSMPThreads(n):
    """Sets the number of threads used by vtkSMPTools. It is used in various filters."""
    paraview.modules.vtkRemotingSetting.GetInstance().SetNumberOfSMPThreads(n)

    # -----------------------------------------------------------------------------
    """Gets the number of threads used by vtkSMPTools. It is used in various filters.
    """


def GetNumberOfSMPThreads(n):
    paraview.modules.vtkRemotingSetting.GetInstance().GetNumberOfSMPThreads()


# -----------------------------------------------------------------------------
def WaitForScreenshot(filename=None):
    """Pause this thread until saving a screenshot has terminated.

    :param filename: Path where screenshot should be saved. If no filename is
       provided, then this thread pauses until all screenshots have been saved.
    :type filename: str
    """
    if not filename:
        paraview.servermanager.vtkRemoteWriterHelper.Wait()
    else:
        paraview.servermanager.vtkRemoteWriterHelper.Wait(filename)


# -----------------------------------------------------------------------------
def SaveAnimation(
    filename, viewOrLayout=None, scene=None, location=vtkPVSession.CLIENT, **params
):
    """Save animation as a movie file or series of images.

    `SaveAnimation` is used to save an animation as a movie file (avi, mp4, or ogv) or
    a series of images.

    :param filename: Name of the output file. The extension is used to determine
        the type of the output. Supported extensions are `png`, `jpg`, `tif`,
        `bmp`, and `ppm`. Based on platform (and build) configuration, `avi`, `mp4`,
        and `ogv` may be supported as well.
    :type filename: str
    :param viewOrLayout: The view or layout to save image from, defaults to `None`.
        If `None`, then the active view is used, if available. To save an image from
        a single view, this must be set to a view, to save an image from all views
        in a layout, pass the layout.
    :type viewOrLayout: View or layout proxy.
    :param scene: Animation scene to save. If not provided, then the active scene
        returned by `GetAnimationScene` is used.
    :type scene: Animation scene proxy
    :param location: Location where the screenshot should be saved. This can be one
        of the following values: `vtkPVSession.CLIENT`, `vtkPVSession.DATA_SERVER`.
        The default is `vtkPVSession.CLIENT`.
    :type location: `vtkPVSession.ServerFlags` enum

    **Keyword Parameters (optional)**

        `SaveAnimation` supports all keyword parameters supported by
        `SaveScreenshot`. In addition, the following parameters are supported:

        FrameRate (int):
            Frame rate in frames per second for the output. This only affects the
            output when generated movies (`avi` or `ogv`), and not when saving the
            animation out as a series of images.

        FrameWindow (tuple(int,int))
            To save a part of the animation, provide the range in frames or
            timesteps index.

    In addition, several format-specific keyword parameters can be specified.
    The format is chosen based on the file extension.

    For Image-based file-formats that save series of images e.g. PNG, JPEG,
    following parameters are available.

        SuffixFormat (string):
            Format string used to convert the frame number to file name suffix.

    FFMPEG avi file format supports following parameters.

        Compression (int)
            Set to 1 or True to enable compression.

        Quality:
            When compression is 1 (or True), this specifies the compression
            quality. `0` is worst quality (smallest file size) and `2` is best
            quality (largest file size).

    VideoForWindows (VFW) avi file format supports following parameters.

        Quality:
            This specifies the compression quality. `0` is worst quality
            (smallest file size) and `2` is best quality (largest file size).

    OGG/Theora file format supports following parameters.

        Quality:
            This specifies the compression quality. `0` is worst quality
            (smallest file size) and `2` is best quality (largest file size).

        UseSubsampling:
            When set to 1 (or True), the video will be encoded using 4:2:0
            subsampling for the color channels.
    """
    # use active view if no view or layout is specified.
    viewOrLayout = viewOrLayout if viewOrLayout else GetActiveView()

    if not viewOrLayout:
        raise ValueError("A view or layout must be specified.")

    scene = scene if scene else GetAnimationScene()
    if not scene:
        raise RuntimeError("Missing animation scene.")

    controller = servermanager.ParaViewPipelineController()
    options = servermanager.misc.SaveAnimation()
    controller.PreInitializeProxy(options)

    options.AnimationScene = scene
    options.Layout = viewOrLayout if viewOrLayout.IsA("vtkSMViewLayoutProxy") else None
    options.View = viewOrLayout if viewOrLayout.IsA("vtkSMViewProxy") else None
    options.SaveAllViews = True if viewOrLayout.IsA("vtkSMViewLayoutProxy") else False

    # this will choose the correct format.
    options.UpdateDefaultsAndVisibilities(filename)

    controller.PostInitializeProxy(options)

    # explicitly process format properties.
    formatProxy = options.Format
    formatProperties = formatProxy.ListProperties()
    for prop in formatProperties:
        if prop in params:
            # see comment at vtkSMSaveAnimationProxy.cxx:327
            # certain 'prop' (such as FrameRate) are present
            # in both SaveAnimation and formatProxy (FFMPEG with
            # panel_visibility="never"). In this case save it only
            # in SaveAnimation
            if formatProxy.GetProperty(prop).GetPanelVisibility() != "never":
                formatProxy.SetPropertyWithName(prop, params[prop])
                del params[prop]

    proxy_util.set(options, **params)
    return options.WriteAnimation(filename, location)


def WriteAnimationGeometry(filename, view=None):
    """Save the animation geometry from a specific view to a file specified.
    The animation geometry is written out as a PVD file.

    :param filename: The file path of the PVD file to write.
    :type filename: str
    :param view: The view holding the geometry that should be saved. Optional,
        defaults to the active view if possible.
    :type view: View proxy"""
    view = view if view else GetActiveView()
    if not view:
        raise ValueError("Please specify the view to use")
    scene = GetAnimationScene()
    writer = servermanager.vtkSMAnimationSceneGeometryWriter()
    writer.SetFileName(filename)
    writer.SetAnimationScene(scene.SMProxy)
    writer.SetViewModule(view.SMProxy)
    writer.Save()


def FetchData(proxy=None, **kwargs):
    """Fetches data from the specified data producer for processing locally. Use this
    function with caution since this can cause large amounts of data to be
    gathered and delivered to the client.

    If no producer is specified, the active source is used.

    **Basic Usage**

        #to fetch data from port 0

        dataMap = FetchData(producer)

        # to fetch data from a specific port

        dataMap = FetchData(OutputPort(producer, 1))


    `FetchData()` does not explicitly update the pipeline. It is expected that the
    pipeline is already updated. This will simply deliver the current data.

    Returns a map where the key is an integer representing a rank and value is
    the dataset fetched from that rank.

    **Keyword Parameters**

    The following keyword parameters can be used to customize the fetchs.

        GatherOnAllRanks (bool/int, optional):
            This is used only in symmetric batch (or ParaView-Catalyst) mode.
            If True, then FetchData() will gather the data on all ranks. Default
            is to only gather the data on the root node.

        SourceRanks (list(int), optional):
            List of ints to specity explicitly the ranks from which to fetch
            data. By default, data from all ranks is fetched.
    """
    if proxy is None:
        proxy = GetActiveSource()

    if not proxy:
        raise RuntimeError("Cannot fetch data from invalid proxy")

    dataMover = servermanager.misc.DataMover()
    dataMover.Producer = proxy
    dataMover.PortNumber = proxy.Port
    # set properties on dataMover
    proxy_util.set(dataMover, **kwargs)
    dataMover.SMProxy.InvokeCommand("Execute")

    vtkObj = dataMover.GetClientSideObject()
    result = {}
    for i in range(vtkObj.GetNumberOfDataSets()):
        result[vtkObj.GetDataSetRank(i)] = vtkObj.GetDataSetAtIndex(i)
    del dataMover
    return result
