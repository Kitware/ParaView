# Handle the subdirectories

SUBDIRS(Tcl)

IF(VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF(VTK_WRAP_PYTHON)

# Generate a few small scripts (.bat, .sh, .csh) that can be sourced to set
# the various environments variables (PATH, TCLLIBPATH, LD_LIBRARY_PATH, etc.) 
# required by this executable and its known third-party dependencies (VTK, ITK,
# SOV, KWWidgets, etc.).
# Also Generate a lightweight C launcher for testing purposes. This 
# launcher sets the above environment variables before launching an executable.

IF(KWWidgets_USE_SOVIEWER AND SOV_FOUND)
  INCLUDE(${SOV_USE_FILE})
ENDIF(KWWidgets_USE_SOVIEWER AND SOV_FOUND)

INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsPathsMacros.cmake")
KWWidgets_GENERATE_SETUP_PATHS_SCRIPTS("${EXECUTABLE_OUTPUT_PATH}")
IF(BUILD_TESTING AND NOT KWWidgets_DISABLE_TESTING)
  KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER(
    "${CMAKE_CURRENT_BINARY_DIR}" "KWWidgetsSetupPathsLauncher" "" "")
ENDIF(BUILD_TESTING AND NOT KWWidgets_DISABLE_TESTING)

