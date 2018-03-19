Replace the option to make the pqHelpWindow use QWebKit with an option to use QWebEngine.

This remove the CMake option `PARAVIEW_USE_QTWEBKIT`, which did not work with
Qt 5 since WebKit was deprecated/removed and replaces it with a new option
`PARAVIEW_USE_QTWEBENGINE` which uses Qt's WebEngine (the library that replaced
WebKit).
