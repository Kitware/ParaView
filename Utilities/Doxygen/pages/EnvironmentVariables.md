Environment Variables       {#EnvironmentVariables}
=====================

This page documents environment variables that affect ParaView behavior at
runtime.

Variable | Description
---------|---------------------------------------------------------
PARAVIEW_DATA_ROOT  | Change the location of the data root for testing.
PV_DEBUG_APPLY_BUTTON | When set, debugging text will be printed out to assist developers with the reasons behind the change in state for the "Apply" button on the properties panel (pqPropertiesPanel).
PV_DEBUG_PANELS | When set, debugging text will be printed out explaining the reason for creation of various widgets on the properties panel (pqPropertiesPanel).
PV_DEBUG_SKIP_OPENGL_VERSION_CHECK | Skip test to validate OpenGL support at launch.
PV_DEBUG_TEST | Prints debugging information about the testing framework during playback to cout.
PV_ICET_WINDOW_BORDERS | Force render windows to be 400x400 instead of fullscreen.
PV_DEBUG_REMOTE_RENDERING | Forces server-side render windows to swap buffers in order to see what is being rendered on the server ranks.
PV_MACRO_PATH | Additional directories defined by the user to store macros.
PV_NO_OFFSCREEN_SCREENSHOTS | Disable the use of offsceen screenshots.
PV_ALLOW_BATCH_INTERACTION | Allow interactions in batch mode.
PV_PLUGIN_CONFIG_FILE | XML Plugin Configuration Files to specify which plugin to load on startup.
PV_PLUGIN_DEBUG | Prints debugging information when loading plugins into ParaView.
PV_PLUGIN_PATH | Directories containing plugins to be loaded on startup.
PV_SETTINGS_DEBUG | When set, debugging text will be printed out to assist developers debug settings.
QT_MAC_NO_NATIVE_MENUBAR | Qt flag to force the Qt menu bar rather than the native mac menu bar.
