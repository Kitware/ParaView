# Handle the subdirectories

SUBDIRS(Tcl)

IF(VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF(VTK_WRAP_PYTHON)

# Configure some helper file to setup paths

GET_FILENAME_COMPONENT(KWWidgets_PATHS "${TCL_TCLSH}" PATH)
SET(KWWidgets_PATHS ${KWWidgets_PATHS} 
  ${VTK_LIBRARY_DIRS} ${VTK_RUNTIME_DIRS} 
  ${LIBRARY_OUTPUT_PATH} ${EXECUTABLE_OUTPUT_PATH})
SET(KWWidgets_LIBRARY_PATHS ${VTK_LIBRARY_DIRS} ${LIBRARY_OUTPUT_PATH})
SET(VTK_TCL_PATH "${VTK_DIR}/Wrapping/Tcl")
SET(KWWidgets_TCL_PATH "${KWWidgets_BINARY_DIR}/Wrapping/Tcl")
SET(VTK_PYTHON_PATH "${VTK_DIR}/Wrapping/Python")
SET(KWWidgets_PYTHON_PATH "${KWWidgets_BINARY_DIR}/Wrapping/Python")

IF(WIN32)
  IF(CMAKE_CONFIGURATION_TYPES)
    FOREACH(config ${CMAKE_CONFIGURATION_TYPES})
      GET_FILENAME_COMPONENT(KWWidgets_PATHS "${TCL_TCLSH}" PATH)
      SET(KWWidgets_LIBRARY_PATHS)
      FOREACH(dir ${VTK_LIBRARY_DIRS})
        SET(KWWidgets_PATHS ${KWWidgets_PATHS} "${dir}/${config}")
        SET(KWWidgets_LIBRARY_PATHS ${KWWidgets_LIBRARY_PATHS}
          "${dir}/${config}")
      ENDFOREACH(dir)
      FOREACH(dir ${VTK_RUNTIME_DIRS})
        SET(KWWidgets_PATHS ${KWWidgets_PATHS} "${dir}/${config}")
      ENDFOREACH(dir)
      SET(KWWidgets_PATHS ${KWWidgets_PATHS} 
        "${LIBRARY_OUTPUT_PATH}/${config}" 
        "${EXECUTABLE_OUTPUT_PATH}/${config}")
      SET(KWWidgets_LIBRARY_PATHS ${KWWidgets_LIBRARY_PATHS}
        "${LIBRARY_OUTPUT_PATH}/${config}")
      SET(VTK_TCL_PATH "${VTK_DIR}/Wrapping/Tcl/${config}")
      SET(KWWidgets_TCL_PATH "${KWWidgets_BINARY_DIR}/Wrapping/Tcl/${config}")
      STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
        VTK_PYTHON_PATH "${VTK_PYTHON_PATH}")
      STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
        KWWidgets_PYTHON_PATH "${KWWidgets_PYTHON_PATH}")
      CONFIGURE_FILE(
        ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.bat.in
        ${EXECUTABLE_OUTPUT_PATH}/${config}/KWWidgetsSetupPaths.bat
        IMMEDIATE)
      # For Cygwin
      STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
        KWWidgets_PATHS "${KWWidgets_PATHS}")
      STRING(REGEX REPLACE ";" ":" KWWidgets_PATHS "${KWWidgets_PATHS}")
      STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
        KWWidgets_LIBRARY_PATHS "${KWWidgets_LIBRARY_PATHS}")
      STRING(REGEX REPLACE ";" ":" KWWidgets_LIBRARY_PATHS
        "${KWWidgets_LIBRARY_PATHS}")
      STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
        VTK_TCL_PATH "${VTK_TCL_PATH}")
      STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
        KWWidgets_TCL_PATH "${KWWidgets_TCL_PATH}")
      STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
        VTK_PYTHON_PATH "${VTK_PYTHON_PATH}")
      STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
        KWWidgets_PYTHON_PATH "${KWWidgets_PYTHON_PATH}")
      CONFIGURE_FILE(
        ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.sh.in
        ${EXECUTABLE_OUTPUT_PATH}/${config}/KWWidgetsSetupPaths.sh
        IMMEDIATE)
      CONFIGURE_FILE(
        ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.csh.in
        ${EXECUTABLE_OUTPUT_PATH}/${config}/KWWidgetsSetupPaths.csh
        IMMEDIATE)
    ENDFOREACH(config)
  ELSE(CMAKE_CONFIGURATION_TYPES)
    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      VTK_PYTHON_PATH "${VTK_PYTHON_PATH}")
    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      KWWidgets_PYTHON_PATH "${KWWidgets_PYTHON_PATH}")
    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.bat.in
      ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsSetupPaths.bat
      IMMEDIATE)
    # For Cygwin
    STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
      KWWidgets_PATHS "${KWWidgets_PATHS}")
    STRING(REGEX REPLACE ";" ":" KWWidgets_PATHS "${KWWidgets_PATHS}")
    STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
      KWWidgets_LIBRARY_PATHS "${KWWidgets_LIBRARY_PATHS}")
    STRING(REGEX REPLACE ";" ":" KWWidgets_LIBRARY_PATHS
      "${KWWidgets_LIBRARY_PATHS}")
    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      VTK_TCL_PATH "${VTK_TCL_PATH}")
    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      KWWidgets_TCL_PATH "${KWWidgets_TCL_PATH}")
    STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
      VTK_PYTHON_PATH "${VTK_PYTHON_PATH}")
    STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
      KWWidgets_PYTHON_PATH "${KWWidgets_PYTHON_PATH}")
    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.sh.in
      ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsSetupPaths.sh
      IMMEDIATE)
    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.csh.in
      ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsSetupPaths.csh
      IMMEDIATE)
  ENDIF(CMAKE_CONFIGURATION_TYPES)
ELSE(WIN32)
  STRING(REGEX REPLACE ";" ":" KWWidgets_PATHS "${KWWidgets_PATHS}")
  STRING(REGEX REPLACE ";" ":" KWWidgets_LIBRARY_PATHS "${KWWidgets_LIBRARY_PATHS}")
  CONFIGURE_FILE(
    ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.sh.in
    ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsSetupPaths.sh
    IMMEDIATE)
  CONFIGURE_FILE(
    ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.csh.in
    ${EXECUTABLE_OUTPUT_PATH}/KWWidgetsSetupPaths.csh
    IMMEDIATE)
ENDIF(WIN32)
