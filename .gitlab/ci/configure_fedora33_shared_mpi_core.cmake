# Test VRPlugin with VRUI support
set(PARAVIEW_PLUGIN_ENABLE_VRPlugin ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_VRPlugin_USE_VRUI ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora33.cmake")
