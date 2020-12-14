## Plugin RPATH entries are improved

By default, `paraview_plugin_build(ADD_INSTALL_RPATHS)` is now `ON` if not
specified. This ensures that installed plugins can load modules beside them
properly out of the box.
