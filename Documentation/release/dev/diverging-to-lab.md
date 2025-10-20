## Changed matplotlib maps from diverging to lab color space

Several monotonic colormaps (that I believe originated from matplotlib) were
using the diverging color space for their interpolation. This is wrong as these
colormaps are not diverging. They are monotonic. Instead, they should be
interpolated in the CIELAB colorspace.

There are enough control points to make the interpolation generally unimportant,
but using the wrong interpolation can lead to weird artifacts as seen in these
other MRs:

  * https://gitlab.kitware.com/vtk/vtk/-/merge_requests/12540
  * https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7499
