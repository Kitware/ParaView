# Make sure we have Tcl

INCLUDE(${CMAKE_ROOT}/Modules/FindTCL.cmake)
IF (NOT TCL_LIBRARY)
  MESSAGE("VTK_WRAP_TCL is ON, but the Tcl library was not found.  Please set TCL_LIBRARY." "Error")
ENDIF (NOT TCL_LIBRARY)

SET(KWWidgets_TCL_LIBNAME_PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")

# Configure the Tcl package index files for the build tree.
# You can then point TCLLIBPATH to the Wrapping/Tcl dir in the right
# build directory (so that the right shared lib is loaded)

SET(KWWidgets_TCL_SCRIPT_DIR "${KWWidgets_SOURCE_DIR}/Wrapping/Tcl")
SET(KWWidgets_TCL_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})
IF(KWWidgets_TCL_CONFIGURATION_TYPES)
  FOREACH(config ${KWWidgets_TCL_CONFIGURATION_TYPES})
    SET(KWWidgets_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}/${config}")
    CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                   ${KWWidgets_BINARY_DIR}/Wrapping/Tcl/${config}/pkgIndex.tcl
                   @ONLY IMMEDIATE)
  ENDFOREACH(config)
ELSE(KWWidgets_TCL_CONFIGURATION_TYPES)
  SET(KWWidgets_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}")
  CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
                 ${KWWidgets_BINARY_DIR}/Wrapping/Tcl/pkgIndex.tcl
                 @ONLY IMMEDIATE)
ENDIF(KWWidgets_TCL_CONFIGURATION_TYPES)

# Configure the Tcl package index file for the install tree.
# You can then point TCLLIBPATH to the lib/KWWidgets/tcl dir in the right
# installation directory
# We configure a new pkgIndex file that uses KWWidgets_TCL_LIBRARY_DIR
# It has to be in Install/Hide so that Tcl does not find it automatically

SET(KWWidgets_TCL_SCRIPT_DIR "[file dirname [info script]]")
IF(UNIX)
  # KWWidgets_TCL_LIBRARY_DIR is used to find the library in the bin
  # directory relative to the pkgIndex file located in the lib directory
  SET(KWWidgets_TCL_LIBRARY_DIR "[file dirname [file dirname [info script]]]")
ELSE(UNIX)
  SET(KWWidgets_TCL_LIBRARY_DIR
      "[file join [file dirname [file dirname [file dirname [file dirname [info script]]]]] bin]")
ENDIF(UNIX)

CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/pkgIndex.tcl.in
               ${KWWidgets_BINARY_DIR}/Wrapping/Tcl/Install/Hide/pkgIndex.tcl
               @ONLY IMMEDIATE)
IF(NOT KWWidgets_INSTALL_NO_RUNTIME)
  INSTALL_FILES(${KWWidgets_INSTALL_LIB_DIR}/tcl FILES
                ${KWWidgets_BINARY_DIR}/Wrapping/Tcl/Install/Hide/pkgIndex.tcl)
ENDIF(NOT KWWidgets_INSTALL_NO_RUNTIME)

# Install kits

SUBDIRS(kwwidgets)

# Configure some helper file to setup paths

GET_FILENAME_COMPONENT(KWWidgets_PATHS ${TCL_TCLSH} PATH)
SET(KWWidgets_PATHS ${KWWidgets_PATHS} 
  ${VTK_LIBRARY_DIRS} ${VTK_RUNTIME_DIRS} 
  ${LIBRARY_OUTPUT_PATH} ${EXECUTABLE_OUTPUT_PATH})
SET(VTK_TCL_PATH "${VTK_DIR}/Wrapping/Tcl")
SET(KWWidgets_TCL_PATH "${KWWidgets_BINARY_DIR}/Wrapping/Tcl")

IF(WIN32)
  IF(CMAKE_CONFIGURATION_TYPES)
    FOREACH(config ${CMAKE_CONFIGURATION_TYPES})
      GET_FILENAME_COMPONENT(KWWidgets_PATHS "${TCL_TCLSH}" PATH)
      FOREACH(dir ${VTK_LIBRARY_DIRS})
        SET(KWWidgets_PATHS ${KWWidgets_PATHS} "${dir}/${config}")
      ENDFOREACH(dir)
      FOREACH(dir ${VTK_RUNTIME_DIRS})
        SET(KWWidgets_PATHS ${KWWidgets_PATHS} "${dir}/${config}")
      ENDFOREACH(dir)
      SET(KWWidgets_PATHS ${KWWidgets_PATHS} 
        "${LIBRARY_OUTPUT_PATH}/${config}" 
        "${EXECUTABLE_OUTPUT_PATH}/${config}")
      SET(VTK_TCL_PATH "${VTK_DIR}/Wrapping/Tcl/${config}")
      SET(KWWidgets_TCL_PATH "${KWWidgets_BINARY_DIR}/Wrapping/Tcl/${config}")
      CONFIGURE_FILE(
        ${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/KWWidgetsSetupPaths.bat.in
        ${EXECUTABLE_OUTPUT_PATH}/${config}/KWWidgetsSetupPaths.bat
        IMMEDIATE)
      # For Cygwin
      STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
        KWWidgets_PATHS "${KWWidgets_PATHS}")
      STRING(REGEX REPLACE ";" ":" KWWidgets_PATHS "${KWWidgets_PATHS}")
      STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
        VTK_TCL_PATH "${VTK_TCL_PATH}")
      STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
        KWWidgets_TCL_PATH "${KWWidgets_TCL_PATH}")
      CONFIGURE_FILE(
        ${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/KWWidgetsSetupPaths.sh.in
        ${EXECUTABLE_OUTPUT_PATH}/${config}/KWWidgetsSetupPaths.sh
        IMMEDIATE)
    ENDFOREACH(config)
  ELSE(CMAKE_CONFIGURATION_TYPES)
    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/KWWidgetsSetupPaths.bat.in
      ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsSetupPaths.bat
      IMMEDIATE)
    # For Cygwin
    STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
      KWWidgets_PATHS "${KWWidgets_PATHS}")
    STRING(REGEX REPLACE ";" ":" KWWidgets_PATHS "${KWWidgets_PATHS}")
    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      VTK_TCL_PATH "${VTK_TCL_PATH}")
    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      KWWidgets_TCL_PATH "${KWWidgets_TCL_PATH}")
    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/KWWidgetsSetupPaths.sh.in
      ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsSetupPaths.sh
      IMMEDIATE)
  ENDIF(CMAKE_CONFIGURATION_TYPES)
ELSE(WIN32)
  STRING(REGEX REPLACE ";" ":" KWWidgets_PATHS "${KWWidgets_PATHS}")
  CONFIGURE_FILE(
    ${KWWidgets_SOURCE_DIR}/Wrapping/Tcl/KWWidgetsSetupPaths.sh.in
    ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsSetupPaths.sh
    IMMEDIATE)
ENDIF(WIN32)
