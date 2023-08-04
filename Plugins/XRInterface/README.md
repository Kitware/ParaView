# XRInterface Plugin

This is a ParaView plugin providing support for using XR devices with ParaView.
It relies on OpenVR or OpenXR and related VTK modules to provide such support.
For a complete list of features, please take a look at the [doc](https://docs.paraview.org/en/latest/UsersGuide/displayingData.html?highlight=XRInterface#xrinterface-plugin).

This plugin is used and tested on Windows only, but may be usable on other OSes.
It is assured to build on Linux.

## Building the plugin

### Dependencies

Technically, OpenVR and OpenXR are both optional dependencies of this plugin,
however, without any backend, the plugin does not provide any features, so
you may want to install one of them.

#### OpenVR

git clone [OpenVR](https://github.com/ValveSoftware/openvr) somewhere.
Alternatively, compile it yourself, but make sure to turn on `BUILD_SHARED` in the cmake configuration.

#### OpenXR

git clone, configure, build and install [OpenXR](https://github.com/KhronosGroup/OpenXR-SDK) somewhere.

#### OpenXRRemoting

The new OpenXRRemoting module allows to connect the current active render window to a remote XR device.
For now, this feature is experimental and Windows only as it is for the Hololens2.

For remoting, the SDK cannot be used directly as the `DYNAMIC_LOADER` option is missing. You will need to
compile from source [OpenXR](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/BUILDING.md#windows-32-bit).

Then you need an additional NuGet package named `Microsoft.Holographic.Remoting.OpenXr` available
[here](https://www.nuget.org/packages/Microsoft.Holographic.Remoting.OpenXr/2.8.1). Note that you will
need the exact same version of this package between the one used to build this plugin and for the player
application in your remote device. For more details about that check the documentation about the OpenXRRemoting
module in vtk [here](../../VTK/Rendering/OpenXRRemoting/README.md).

### Building ParaView with the XRInterface plugin

During the ParaView configuration, enable `PARAVIEW_PLUGIN_ENABLE_XRInterface`,
then enable the wanted backend options, `PARAVIEW_XRInterface_OpenXR_Support` or
`PARAVIEW_XRInterface_OpenVR_Support` or both.

CMake may then complain about missing VTK modules, you will probably need to set
`VTK_MODULE_ENABLE_VTK_RenderingOpenVR` and/or `VTK_MODULE_ENABLE_VTK_RenderingOpenXR`
to `WANT`.

Then make sure to point to the right location for the backends includes and libs:
 - `OpenVR_INCLUDE_DIR` may point to `path/to/openvr/headers/`
 - `OpenVR_LIBRARY` may point to `path/to/openvr/bin/${arch}/libopenvr_api.so|lib`
 - `OpenXR_INCLUDE_DIR` may point to `path/to/openxr_install/include/openxr/`
 - `OpenXR_LIBRARY` may point to `path/to/openxr_install/lib|bin/openxr_loader.so|lib`

You should then be able to configure and build ParaView with the plugin and start using it in ParaView.

#### Building ParaView with the OpenXRRemoting module

Since the module depends on OpenXR, the CMake option `PARAVIEW_XRInterface_OpenXR_Support` needs to be enabled first.
Then specify include directory and library like above.

For the remoting, enable `PARAVIEW_XRInterface_OpenXRRemoting_Support`.

CMake may then complain about missing VTK modules, you will probably need to set
`VTK_MODULE_ENABLE_VTK_RenderingOpenXRRemoting` to `WANT`.

Then make sure to point to the right includes and bin:
- `OpenXRRemoting_INCLUDE_DIR` may point to `path/to/Microsoft.Holographic.Remoting.OpenXr/build/native/include/openxr`.
- `OpenXRRemoting_BIN_DIR` may point to `path/to/Microsoft.Holographic.Remoting.OpenXr/build/native/bin/x64/Desktop`.

Note that you will also need to add these paths to the environment variable `PATH`.

### Other options

If you are on Windows and turn on `PARAVIEW_XRInterface_Imago_Support` then this
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

## Testing

Automatic tests are lacking for now, with only simple smoke tests that load the plugin
and check that the buttons are working without actually using VR in any way.
These tests can be used without any backends.

For actual manual testing of the features, see [testing](TESTING.md).

## OpenXR/OpenVR Differences

OpenXR and OpenVR are pretty much at the same level in terms of provided features, there are a few limitations with OpenXR:
 - Controller model is simple
 - Grounded movement is not working
 - Base stations are not visible
 - Widgets are causing ton of warnings, closing the Output message window is advised
