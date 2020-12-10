# Stock CI builds test everything possible (platforms will disable modules as
# needed).
set(PARAVIEW_BUILD_LEGACY_REMOVE ON CACHE BOOL "")
set(PARAVIEW_BUILD_TESTING WANT CACHE STRING "")
set(PARAVIEW_BUILD_EXAMPLES ON CACHE BOOL "")

set(VTK_DEBUG_LEAKS ON CACHE BOOL "")
set(VTK_USE_LARGE_DATA ON CACHE BOOL "")

# Test VRPlugin with VRUI support
set(PARAVIEW_PLUGIN_ENABLE_VRPlugin ON CACHE BOOL "")
set(PARAVIEW_PLUGIN_VRPlugin_USE_VRUI ON CACHE BOOL "")

# The install trees on CI machines need help since dependencies are not in a
# default location.
set(PARAVIEW_RELOCATABLE_INSTALL ON CACHE BOOL "")

# Enable default-off plugins.
set(PARAVIEW_PLUGIN_ENABLE_TemporalParallelismScriptGenerator ON CACHE BOOL "")

# We run the install right after the build. Avoid rerunning it when installing.
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY "ON" CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_options.cmake")

# Default to Release builds.
if ("$ENV{CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")
else ()
  set(CMAKE_BUILD_TYPE "$ENV{CMAKE_BUILD_TYPE}" CACHE STRING "")
endif ()

include("${CMAKE_CURRENT_LIST_DIR}/configure_sccache.cmake")
