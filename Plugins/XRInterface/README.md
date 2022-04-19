# XRInterface Plugin

## Building the plugin (and build options)

If you are on Windows and turn on PARAVIEW_XRInterface_Imago_Support then this
plugin will include settings for loading images from imago.live and
displaying them in XR. This requires that your data (typically borehole
plots) have the correct cell data values to link to the online images.

If you turn on VTK_ENABLE_VR_COLLABORATION then you will get collaboration
support in this plugin which includes the ability to connect to
collaborative XR servers and show others users as avatars in XR. Note that
this option requires you have libzmq available.

If you turn on PARAVIEW_ENABLE_FFMPEG and set
VTK_MODULE_ENABLE_VTK_RenderingFFMPEGOpenGL2 to YES then you will have access
to the Skybox Moview representation listed under the Sources menu, which can
be used to show video skyboxes. For that representation you provide the full
path file name of the movie in the text table property. Note this requires
you have ffmpeg available.

If you turn on PARAVIEW_USE_QTWEBENGINE then you will get the WebPage
representation that will show up under the Sources menu. The
webPageRepresentation enables you to place web pages into your 3D view and
interact with them. The web page to load is pyut into the text table
property. In XR the laser pointers can be used to interact with the web page
and text can be entered as well from the properties' panel for the
representation.

## Building the plugin with OpenXR support

To enable support for the OpenXR runtime in the ParaView plugin, first build
the OpenXR-SDK.  For development and debugging purposes use
[this](https://github.com/KhronosGroup/OpenXR-SDK-Source) repo, for
production, use [this](https://github.com/KhronosGroup/OpenXR-SDK) one.

Then provide the following additional cmake variables when configuring
paraview:

`-DVTK_MODULE_ENABLE_VTK_RenderingOpenXR:STRING=WANT`
`-DOpenXR_INCLUDE_DIR:PATH=<path-to-openxr-install-dir>\include\openxr`
`-DOpenXR_LIBRARY:FILEPATH=<path-to-openxr-install-dir>\lib\openxr_loader.lib`
