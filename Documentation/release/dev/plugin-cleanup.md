# Plugins build and installation tree cleanup

Plugins are built and installed in self contained directories named same as the
plugin under `PARAVIEW_BUILD_PLUGINS_DIR` and `PARAVIEW_INSTALL_PLUGINS_DIR`.
These are set to `lib/paraview-${paraview-version}/plugins` (`bin/plugins` on Windows)
by default. For macOS app bundles too, while the plugins are installed under
`<app>/Contents/Plugins`, they are now contained in a separate directory per
plugin.

The `.plugins` file has also been moved to `PARAVIEW_BUILD_PLUGINS_DIR` and
`PARAVIEW_INSTALL_PLUGINS_DIR` for builds and installs respectively.
