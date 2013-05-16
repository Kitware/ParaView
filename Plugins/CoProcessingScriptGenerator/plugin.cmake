if (PARAVIEW_BUILD_QT_GUI AND PARAVIEW_ENABLE_PYTHON AND PARAVIEW_ENABLE_CATALYST)
  pv_plugin(CoProcessingScriptGenerator
    DESCRIPTION "Plugins for creating co-processing Python scripts"
    DEFAULT_ENABLED
    # Since the plugin name is different, we need to provide this option.
    PLUGIN_NAMES CoProcessingPlugin)
endif()
