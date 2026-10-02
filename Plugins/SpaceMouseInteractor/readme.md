## SpaceMouseInteractor

This plugin allows navigation in ParaView using
[3Dconnexion](https://3dconnexion.com) SpaceMouse devices. It was developed
with the SpaceMouse Compact but supports other SpaceMouse devices.

On Windows and macOS, the plugin uses the official 3DxWare 10 SDK. The SDK
license requires developers and companies to
[register with 3Dconnexion](https://3dconnexion.com/us/software-developer-program/)
to obtain it. For official ParaView binaries and Kitware developers, the SDK
is available through the ParaView superbuild when the build machine is on the
internal Kitware network.

### Windows

Unzip the SDK, enable `PARAVIEW_PLUGIN_ENABLE_SpaceMouseInteractor` in CMake,
and set `3DxWareSDK_ROOT` to the SDK directory (for example,
`C:/projects/3DxWare_SDK_v4-0-2_r17624`). Copy the header-only example
directory `samples/navlib_viewer/src/SpaceMouse` into the SDK's
`include/SpaceMouse` directory.

### macOS

The SDK is installed with the 3DxWare 10 drivers at
`/Library/frameworks/3DconnexionNavlib.framework`. The headers need to be
arranged to match the Windows SDK, including the `SpaceMouse` directory from
the Windows SDK. This requires administrator access:

1. Inside the framework, create `Headers/navlib` and copy all `*.h` files from
   `Headers` into it, except `siappcmd_types.h`.
2. Copy the Windows SDK directory `samples/navlib_viewer/src/SpaceMouse` into
   `Headers/SpaceMouse`.
3. Enable `PARAVIEW_PLUGIN_ENABLE_SpaceMouseInteractor` in CMake. It should
   find the framework and set `3DxWareSDK_INCLUDE_DIR` and
   `3DxWareSDK_LIBRARY`. If needed, set `3DxWareSDK_ROOT` to
   `/Library/frameworks`.

### Linux

Install and run [spacenavd](https://spacenav.sourceforge.net/) and build the
plugin against libspnav and its development headers. Enable
`PARAVIEW_PLUGIN_ENABLE_SpaceMouseInteractor` in CMake. For a locally built,
uninstalled libspnav, pass `-DSPNAV_ROOT=/path/to/libspnav` (or set
`Spnav_INCLUDE_DIR` and `Spnav_LIBRARY` explicitly).

An uninstalled libspnav source build may contain only `libspnav.so.0.5`, whose
SONAME is `libspnav.so.0`. In this case, create a `libspnav.so.0` symlink
pointing to the built library in a directory on the runtime library search
path, such as `build/lib`.

The Linux implementation uses libspnav's native socket connection, which
does not require X11 and works under Wayland. The puck navigates the active
Render View around its center of rotation. In 2D mode, lateral motion pans,
depth motion zooms, and rotation is limited to roll around the viewing
direction. Input is discarded while the server is busy, and the plugin
reconnects if the daemon is unavailable or restarts. Axis mapping, dead
zones, and sensitivity can be adjusted using spacenavd's configuration tools.
