# File containing all the macros that aid in packaging plugins with ParaView (or
# other applications). This doesn't include the macros for creating the plugins,
# those are in ParaViewPlugins.cmake.

INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)

# Add an optional plugin. 
# Arguments:
#   name -- name of the plugin
#   comment 
#   subdirectory --- directory containing the plugin code
#   default -- default value for the plugin
MACRO(paraview_build_optional_plugin name comment subdirectory default)
  OPTION(PARAVIEW_BUILD_PLUGIN_${name} "Build ${comment}" ${default})
  MARK_AS_ADVANCED(PARAVIEW_BUILD_PLUGIN_${name})
  IF(PARAVIEW_BUILD_PLUGIN_${name})
    MESSAGE(STATUS "Plugin: ${comment} enabled")
    ADD_SUBDIRECTORY("${subdirectory}")
  ELSE()
    MESSAGE(STATUS "Plugin: ${comment} disabled")
  ENDIF()
ENDMACRO()
