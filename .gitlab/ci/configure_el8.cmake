# libCDI is not installed in el8 CI
set(PARAVIEW_PLUGIN_ENABLE_CDIReader OFF CACHE BOOL "")
include("${CMAKE_CURRENT_LIST_DIR}/configure_fedora_common.cmake")
