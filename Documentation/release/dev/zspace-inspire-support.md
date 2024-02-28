## zSpace Inspire Support

Add support to new zSpace Inspire hardware in VTK.

The zSpace plugin now uses a dedicated render window class (vtkZSpaceGenericRenderWindow`) to support
the specific rendering mode of the latest zSpace hardware. The zSpace Inspire do not rely on quad-buffering
to display stereo content like previous models. It uses it's dedicated stereo display instead to perform
stereo rendering. In order to have a working stereo effect, current active view should be shown in fullscreen.

Note that the zSpace Stereo Core Compatibility API should be used in that case.
