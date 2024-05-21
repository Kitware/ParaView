Added support for compiling against, and using an external VTK. The
version (git hash) of the external VTK has to match the version of the
VTK sub-module included in ParaView. While this is not enforced, it is
likely that you would get a compilation error or a crash if you try to
use ParaView with a differnet version of VTK than the one it was
tested with (the version of the VTK submodule).

This feature is enabled with PARAVIEW_USE_EXTERNAL_VTK
checked. VTK_DIR can be used to specify the location for the config
file of the VTK install, such as: <vtk_install_dir>/lib/cmake/vtk-9.3.

Depending on what is enabled in ParaView, certain features would need
to be provided by the external VTK. The features required by ParaView
but not provided by the VTK install, would be printed on the command
line at cmake configure time, in a format that can be copied and
pasted in a VTK reconfigure command that would compile those features
in VTK.
