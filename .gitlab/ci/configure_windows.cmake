set(PARAVIEW_PLUGIN_ENABLE_pvNVIDIAIndeX ON CACHE BOOL "")

# libCDI is not installed in windows CI
set(PARAVIEW_PLUGIN_ENABLE_CDIReader OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
