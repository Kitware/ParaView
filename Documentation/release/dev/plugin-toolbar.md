# Adding QToolBar via plugins

ParaView now supports add a QToolBar subclass via the plugin mechanism. In the past, one had to add
actions to a toolbar via a QActionGroup. Now, directly adding a QToolBar is a supported by using
`add_paraview_toolbar` CMake function.
