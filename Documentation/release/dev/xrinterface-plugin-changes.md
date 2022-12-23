# Changes in XRInterface plugin

## Add a Reset Camera button in XRInterface menu

When using Virtual Reality (VR) in ParaView, the menu now includes a button to reset the camera.
This brings the dataset(s) at the center of the physical world (the space where you can move).

## CMake Options

The XRInterface plugin now only optionally requires OpenVR to be built but a new cmake option
needs to be turned on to enable it: `PARAVIEW_XRInterface_OpenVR_Support`.
In order to turn on OpenXR, one needs to enable `PARAVIEW_XRInterface_OpenXR_Support`.
Enabling the VTK modules is still needed.
See [README.md](https://gitlab.kitware.com/paraview/paraview/-/blob/master/Plugins/XRInterface/README.md) for more info.
