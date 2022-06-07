## SpaceMouseInteractor

This plugin allows navigation in ParaView using [3Dconnexion](https://3dconnexion.com) SpaceMouse devices. It was developed with the SpaceMouse Compact, but will function with any SpaceMouse.

It uses the official 3DxWare 10 SDK provided by 3Dconnexion. The license for the SDK requires each developer/company to register with 3Dconnexion to obtain the SDK: https://3dconnexion.com/us/software-developer-program/ . For the offical ParaView binaries and Kitware developers, the SDK is available through the ParaView superbuild when the build machine is on the internal Kitware network.

To build the plugin on Windows, unzip the SDK, and in CMake, check `PARAVIEW_PLUGIN_ENABLE_SpaceMouseInteractor`, and set `3DxWareSDK_ROOT` to point to the SDK, for example: `-D3DxWareSDK_DIR="C:/projects/3DxWare_SDK_v4-0-2_r17624"`. The plugin uses a header-only library from an example, so one directory must be copied: `3DxWare_SDK_v4-0-2_r17624/samples/navlib_viewer/src/SpaceMouse` should be copied to `3DxWare_SDK_v4-0-2_r17624/include/SpaceMouse`.

On MacOS, the SDK is installed with the 3DxWare 10 drivers, and is located at `/Library/frameworks/3DconnexionNavlib.framework`. Unfortunately it needs to be rearranged to match the Windows SDK, and requires the `SpaceMouse` directory from the Windows SDK as well. (3Dconnexion support indicated this may change soon.)
* Create a directory `3DxWare_SDK_MacOS` with subdirectories `include` and `lib`
* Copy files from `/Library/frameworks/3DconnexionNavlib.framework` to this new directory:
   * copy `siappcmd_types.h` from `Headers` to `include`
   * copy all other `*.h` from `Headers` to `include/navlib/`
   * copy `3DconnexionNavlib` to `lib`
   * copy a directory from the Windows SDK, `3DxWare_SDK_v4-0-2_r17624/samples/navlib_viewer/src/SpaceMouse` to `include/SpaceMouse`
* Now in CMake for ParaView, check `PARAVIEW_PLUGIN_ENABLE_SpaceMouseInteractor`, and set `3DxWareSDK_ROOT` to point to the `3DxWare_SDK_MacOS` directory.

On Linux, a do-nothing implementation is provided. 3Dconnexion support indicated no plans for an equivalent SDK for Linux. The open-source driver at http://spacenav.sourceforge.net may be worth exploring in the future.
