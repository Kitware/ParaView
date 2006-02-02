PROJECT(KWWidgetsExamples)

# This is done for convenience, so that this very directory can
# be used as a top-level source directory for CMake. 
# Provided that KWWidgets_DIR is set, this will enable testing of all 
# examples separately from the library (say, from an installed KWWidgets tree)
# The relevant variables are:
#   KWWidgets_DIR
#   TCL_TCLSH (for Tcl tests)
#   PYTHON_EXECUTABLE (for Python tests)
#   KWWidgets_USE_SOVIEWER (for SOViewer tests)
#   SOV_DIR (for SOViewer tests)

IF(NOT DEFINED BUILD_TESTING)
  INCLUDE(Dart)
  IF(NOT DEFINED TCL_TCLSH)
    INCLUDE(FindTclsh)
  ENDIF(NOT DEFINED TCL_TCLSH)
  IF(NOT DEFINED PYTHON_EXECUTABLE)
    INCLUDE(FindPythonInterp)
  ENDIF(NOT DEFINED PYTHON_EXECUTABLE)
  MARK_AS_ADVANCED(BUILD_TESTING DART_ROOT TCL_TCLSH PYTHON_EXECUTABLE)
ENDIF(NOT DEFINED BUILD_TESTING)
IF(BUILD_TESTING)
  ENABLE_TESTING()
ENDIF(BUILD_TESTING)

SUBDIRS(Cxx Tcl Python Data Resources)
