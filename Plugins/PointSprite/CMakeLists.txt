project(PointSpritePlugin)
cmake_minimum_required(VERSION 2.8.8)

# -----------------------------------------------------------------------------
# Disable deprecation warnings for standard C and STL functions in VS2005 and
# later (no, we don't need IF(CMAKE_COMPILER_2005) ... )
# -----------------------------------------------------------------------------
add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
add_definitions(-D_SCL_SECURE_NO_DEPRECATE)

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  # we are building from within ParaView Source. The environment is already set.
  # Nothign more to do.
ELSE (ParaView_SOURCE_DIR)
  find_package(ParaView)
  include("${PARAVIEW_USE_FILE}")
  include (ParaViewPlugins)

  # Load information about VTK/ParaView modules loaded/available.
  pv_setup_module_environment("PointSpritePlugin")
ENDIF (ParaView_SOURCE_DIR)

# -----------------------------------------------------------------------------
# Process all vtk-modules defined in this plugin. This generates the necessary
# wrappings for all these modules as well.
# -----------------------------------------------------------------------------
pv_process_modules()

# -----------------------------------------------------------------------------
# Build the Paraview plugins
# -----------------------------------------------------------------------------
add_subdirectory(ParaViewPlugin)
#
## -----------------------------------------------------------------------------
## Build the tests
## -----------------------------------------------------------------------------
#IF (DEFINED BUILD_EXAMPLES)
#  IF (BUILD_EXAMPLES)
#      add_subdirectory(Examples)
#  ENDIF (BUILD_EXAMPLES)
#ELSE (DEFINED BUILD_EXAMPLES)
#  set(PointSpritePlugin_BUILD_EXAMPLES ON CACHE BOOL "Build the example.")
#  IF(PointSpritePlugin_BUILD_EXAMPLES)
#      add_subdirectory(Examples)
#  ENDIF(PointSpritePlugin_BUILD_EXAMPLES)
#  mark_as_advanced(PointSpritePlugin_BUILD_EXAMPLES)
#ENDIF (DEFINED BUILD_EXAMPLES)
