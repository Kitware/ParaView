# plugin-dir-structure-loading

`PV_PLUGIN_PATH` now supports directories which hold the structure made by
ParaView's plugin build macros. Namely, that directories named `PluginName`
found in these paths will trigger a search for `PluginName.dll` or
`PluginName.so` (depending on platform) under that directory.

Plugin XML files (which list plugins and `auto_load` settings) may now specify
a relative path to a plugin. When using a relative path, it must end up under
the original XML file's directory (i.e., using `..` to "escape" is not
allowed).
