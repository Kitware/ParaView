# Add rpath entries for Xcode frameworks.
set(CMAKE_BUILD_RPATH "$ENV{DEVELOPER_DIR}/Library/Frameworks" CACHE STRING "")
set(CMAKE_INSTALL_RPATH "$ENV{DEVELOPER_DIR}/Library/Frameworks" CACHE STRING "")

# Test VRPlugin with VRUI support
set(PARAVIEW_PLUGIN_ENABLE_VRPlugin ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_VRPlugin_USE_VRUI ON CACHE BOOL "")

# libCDI is not installed in macos CI
set(PARAVIEW_PLUGIN_ENABLE_CDIReader OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
