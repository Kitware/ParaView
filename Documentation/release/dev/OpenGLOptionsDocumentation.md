## Improvements of the documentation and cmake management about OpenGL options ##

The documentation about OpenGL options (https://kitware.github.io/paraview-docs/nightly/cxx/Offscreen.html)
has been improved as well as cmake checks of which options are compatible with each other.

`PARAVIEW_USE_QT` should not be used to detect if the `paraview` Qt executable
has been built, instead, `TARGET ParaView::paraview` should be used.
