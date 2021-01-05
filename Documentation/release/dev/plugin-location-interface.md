## Plugin Location Interface

Dynamically-loaded plugins can now get the file system location of the
plugin binary file (DLL, shared object) with the addition of the
`pqPluginLocationInterface` class and `paraview_add_plugin_location` cmake
function. This allows dynamic plugins to include text and/or data files
that can be located and loaded at runtime.
