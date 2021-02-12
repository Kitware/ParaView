Environment Variables       {#EnvironmentVariables}
=====================

This page documents environment variables that affect ParaView behavior at
runtime.

Variable | Description
---------|---------------------------------------------------------
`PARAVIEW_DATA_ROOT`  | Change the location of the data root for testing.
`PARAVIEW_OVERRIDE_EXTRACTS_OUTPUT_DIRECTORY` | Override output directory used to save extracts.
`PARAVIEW_USE_MPI_SSEND` | When set on the server processes, `MPI_Send` may be replaced with `MPI_Ssend` (useful for debugging purposes).
`PV_DEBUG_PANELS` | When set, debugging text will be printed out explaining the reason for creation of various widgets on the properties panel (pqPropertiesPanel).
`PV_DEBUG_REMOTE_RENDERING` | Forces server-side render windows to swap buffers in order to see what is being rendered on the server ranks.
`PV_DEBUG_SKIP_OPENGL_VERSION_CHECK` | Skip test to validate OpenGL support at launch.
`PV_DEBUG_TEST` | Prints debugging information about the testing framework during playback to `stdout`.
`PV_ICET_WINDOW_BORDERS` | Force render windows to be 400x400 instead of fullscreen.
`PV_MACRO_PATH` | Additional directories defined by the user to store macros.
`PV_PLUGIN_CONFIG_FILE` | XML Plugin Configuration Files to specify which plugin to load on startup.
`PV_PLUGIN_PATH` | Directories containing plugins to be loaded on startup.
`PV_SHARED_WINDOW_SIZE`  | Similar to `PV_ICET_WINDOW_BORDERS` except that the value is specified as `WxH` where `W` and `H` is the width and height for the render window.
`QT_MAC_NO_NATIVE_MENUBAR` | Qt flag to force the Qt menu bar rather than the native mac menu bar.
`VTK_DISABLE_OSPRAY` | Skip rendering support tests to enable OSPRay.
`VTK_DISABLE_VISRTX` | Skip rendering support tests to enable VisRTX.

Obsolete Variable | Description
---------|---------------------------------------------------------
`PV_PLUGIN_DEBUG` | (obsolete) Use `PARAVIEW_LOG_PLUGIN_VERBOSITY` instead.
`PV_SETTINGS_DEBUG` | (obsolete) Use `PARAVIEW_LOG_APPLICATION_VERBOSITY` instead.
`PV_DEBUG_APPLY_BUTTON` | (obsolete) Use `PARAVIEW_LOG_APPLICATION_VERBOSITY` instead.
`PV_ALLOW_BATCH_INTERACTION` | (obsolete) No longer needed.

ParaView supports generating logs that includes debugging and tracking
information. The log messages are categorized and it is possible to temporarily
elevate the log level for any category using the following environment
variables. The value for each of the variables can be a number in the range
[-2, 9] or the strings `INFO`, `ERROR`, `WARNING`, `TRACE`, or `MAX` (See
`vtkLogger` for additional information about log levels).

Variable | Description
---------|-----------------------------------------
`PARAVIEW_LOG_APPLICATION_VERBOSITY` | Log messages related to the application (see vtkPVLogger::GetApplicationVerbosity())
`PARAVIEW_LOG_CATALYST_VERBOSITY` | Log messages related to Catalyst (see vtkPVLogger::GetCatalystVerbosity())
`PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY` | Log messages related to data movement for rendering and other tasks (see vtkPVLogger::GetDataMovementVerbosity())
`PARAVIEW_LOG_PIPELINE_VERBOSITY`  | Log messages related to Pipeline execution (see vtkPVLogger::GetPipelineVerbosity())
`PARAVIEW_LOG_PLUGIN_VERBOSITY` | Log messages related to ParaView plugins (see vtkPVLogger::GetPluginVerbosity())
