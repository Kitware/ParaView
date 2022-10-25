# Add rpath entries for Xcode frameworks.
set(CMAKE_BUILD_RPATH "$ENV{DEVELOPER_DIR}/Library/Frameworks" CACHE STRING "")
set(CMAKE_INSTALL_RPATH "$ENV{DEVELOPER_DIR}/Library/Frameworks" CACHE STRING "")

# Test CAVEInteraction with VRUI support
set(PARAVIEW_PLUGIN_ENABLE_CAVEInteraction ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_common.cmake")
