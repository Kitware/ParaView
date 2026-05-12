# Add rpath entries for Xcode frameworks.
# Test CAVEInteraction with VRUI support
set(PARAVIEW_PLUGIN_ENABLE_CAVEInteraction ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI ON CACHE BOOL "")

# Test CDIReader plugin
set(PARAVIEW_PLUGIN_ENABLE_CDIReader ON CACHE BOOL "")

set(PARAVIEW_PLUGIN_ENABLE_ONNXPlugin ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_ENABLE_pvNVIDIAIndeX ON CACHE BOOL "")

set(PARAVIEW_BUILD_TRANSLATIONS ON CACHE BOOL "")

set(rpaths
  # Add rpath entries for Xcode frameworks.
  "$ENV{DEVELOPER_DIR}/Library/Frameworks")

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "python")
  list(APPEND rpaths
    # Add rpath entry for the CI-downloaded Python.
    "$ENV{GIT_CLONE_PATH}/.gitlab/python/Python.framework")
endif ()

set(CMAKE_BUILD_RPATH "${rpaths}" CACHE STRING "")
set(CMAKE_INSTALL_RPATH "${rpaths}" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
