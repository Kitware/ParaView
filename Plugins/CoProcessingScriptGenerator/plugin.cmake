if (PARAVIEW_BUILD_QT_GUI AND PARAVIEW_ENABLE_PYTHON)
  pv_plugin(CoProcessingScriptGenerator
    DESCRIPTION "Plugin for creating python coprocessing scripts"
    DEFAULT_ENABLED
    # Since the plugin name is different, we need to provide this option.
    PLUGIN_NAMES CoProcessingPlugin)
endif()
