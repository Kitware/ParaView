# Make sure we have Tcl

IF (NOT TCL_LIBRARY)
  MESSAGE("VTK_WRAP_TCL is ON, but the Tcl library was not found.  Please set TCL_LIBRARY." "Error")
ENDIF (NOT TCL_LIBRARY)

SET(KWWIDGETS_TCL_LIBNAME_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")

# Configure the Tcl package index files for the build tree.
# You can then point TCLLIBPATH to the Wrapping/Tcl dir in the right
# build directory (so that the right shared lib is loaded)

SET(KWWIDGETS_TCL_SCRIPT_DIR "${KWWidgets_SOURCE_DIR}/Wrapping/Tcl")
SET(KWWIDGETS_TCL_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})
IF(KWWIDGETS_TCL_CONFIGURATION_TYPES)
  FOREACH(config ${KWWIDGETS_TCL_CONFIGURATION_TYPES})
    SET(KWWIDGETS_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}/${config}")
    CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                   ${KWWidgets_BINARY_DIR}/Wrapping/Tcl/${config}/pkgIndex.tcl
                   @ONLY IMMEDIATE)
  ENDFOREACH(config)
ELSE(KWWIDGETS_TCL_CONFIGURATION_TYPES)
  SET(KWWIDGETS_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}")
  CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                 ${KWWidgets_BINARY_DIR}/Wrapping/Tcl/pkgIndex.tcl
                 @ONLY IMMEDIATE)
ENDIF(KWWIDGETS_TCL_CONFIGURATION_TYPES)

# Configure the Tcl package index file for the install tree.
# You can then point TCLLIBPATH to the lib/KWWidgets/tcl dir in the right
# installation directory
# We configure a new pkgIndex file that uses KWWIDGETS_TCL_LIBRARY_DIR
# It has to be in Install/Hide so that Tcl does not find it automatically

SET(KWWIDGETS_TCL_SCRIPT_DIR "[file dirname [info script]]")
IF(UNIX)
  # KWWIDGETS_TCL_LIBRARY_DIR is used to find the library in the bin
  # directory relative to the pkgIndex file located in the lib directory
  SET(KWWIDGETS_TCL_LIBRARY_DIR "[file dirname [file dirname [info script]]]")
ELSE(UNIX)
  SET(KWWIDGETS_TCL_LIBRARY_DIR
      "[file join [file dirname [file dirname [file dirname [file dirname [info script]]]]] bin]")
ENDIF(UNIX)

CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
               ${KWWidgets_BINARY_DIR}/Wrapping/Tcl/Install/Hide/pkgIndex.tcl
               @ONLY IMMEDIATE)
INSTALL_FILES(${KW_INSTALL_LIB_DIR}/tcl FILES
              ${KWWidgets_BINARY_DIR}/Wrapping/Tcl/Install/Hide/pkgIndex.tcl)

# Install kits

SUBDIRS(kwwidgets)

# Configure some helper file to setup paths

GET_FILENAME_COMPONENT(TCL_TCLSH_PATH ${TCL_TCLSH} PATH)
SET(VTK_WRAPPING_TCL_DIR "${VTK_DIR}/Wrapping/Tcl")
SET(KWWIDGETS_WRAPPING_TCL_DIR "${KWWidgets_BINARY_DIR}/Wrapping/Tcl")
IF(WIN32)
  STRING(REGEX REPLACE "/" "\\\\" TCL_BINARY_DIR "${TCL_TCLSH_PATH}")
  STRING(REGEX REPLACE "/" "\\\\" VTK_EXECUTABLE_OUTPUT_PATH "${VTK_LIBRARY_DIRS}")
  STRING(REGEX REPLACE "/" "\\\\" KWWIDGETS_EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}")
  CONFIGURE_FILE(
    ${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/KWWidgetsTclPath.bat.in
    ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsTclPath.bat
    IMMEDIATE)
ELSE(WIN32)
  CONFIGURE_FILE(
    ${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/KWWidgetsTclPath.sh.in
    ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsTclPath.sh
    IMMEDIATE)
ENDIF(WIN32)
