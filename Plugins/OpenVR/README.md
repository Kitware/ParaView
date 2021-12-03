# OpenVR Plugin

## Building the plugin (and build options)

If you are on windows and turn on PARAVIEW_OpenVR_Imago_Support then this
plugin will include settings for loading images from imago.live and
displaying them in VR. This requirs that your data (typically borehole
plots) have the correct cell data values to link to the online images.

If you turn on VTK_ENABLE_VR_COLLABORATION then you will get collaboration
support in this plugin which includes the ability to connect to
collaborative VR servers and show others users as avatars in VR. Note that
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
property. In VR the laser pointers can be used to interact with the web page
and text can be entered as well from the properties panel for the
representation.
