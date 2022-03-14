# Test CAVEInteraction with VRUI support
set(PARAVIEW_PLUGIN_ENABLE_CAVEInteraction ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora35.cmake")
