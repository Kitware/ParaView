# This test should only be run in static builds. It checks that the
# VTK and ParaView modules are being loaded from zip archives.
from paraview import simple

filename = simple.__file__
assert "_paraview.zip" in simple.__file__, \
    "`simple` module not loaded from zip archive!"

from paraview.vtk import vtkCommonCore
assert "_vtk.zip" in vtkCommonCore.__file__, \
    "`vtkCommonCore` module not loaded from zip archive!"
