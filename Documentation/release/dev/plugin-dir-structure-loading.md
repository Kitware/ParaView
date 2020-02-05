# plugin-dir-structure-loading

`PV_PLUGIN_PATH` now supports directories which hold the structure made by
ParaView's plugin build macros. Namely, that directories named `PluginName`
found in these paths will trigger a search for `PluginName.dll` or
`PluginName.so` (depending on platform) under that directory.
