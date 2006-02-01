# Handle the subdirectories

SUBDIRS(Tcl)

IF(VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF(VTK_WRAP_PYTHON)

# Generate the setup path scripts

MACRO(KWWidgets_GENERATE_SETTUPPATHS
    dest_dir
    kwwidgets_lib_paths 
    kwwidgets_runtime_paths 
    kwwidgets_tcl_paths 
    kwwidgets_python_paths)

  GET_FILENAME_COMPONENT(TCL_TCLSH_PATH "${TCL_TCLSH}" PATH)
  GET_FILENAME_COMPONENT(PYTHON_EXECUTABLE_PATH "${PYTHON_EXECUTABLE}" PATH)

  # For PATH

  SET(KWWidgets_PATH_ENV 
    ${TCL_TCLSH_PATH} ${PYTHON_EXECUTABLE_PATH} 
    ${kwwidgets_runtime_paths} ${kwwidgets_lib_paths})

  # For TCLLIBPATH (space separated)

  SET(KWWidgets_TCLLIBPATH_ENV)
  FOREACH(dir ${kwwidgets_tcl_paths})
    SET(KWWidgets_TCLLIBPATH_ENV 
      "${KWWidgets_TCLLIBPATH_ENV} \"${dir}\"")
  ENDFOREACH(dir)
  
  # For PYTHONPATH
  
  SET(KWWidgets_PYTHONPATH_ENV 
    ${kwwidgets_python_paths} ${kwwidgets_lib_paths})

  IF(WIN32)

    # For Win32 PATH (semi-colon separated)

    # For Win32 TCLLIBPATH (space separated, no cygdrive)

    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      KWWidgets_TCLLIBPATH_ENV "${KWWidgets_TCLLIBPATH_ENV}")

    # For Win32 PYTHONPATH (semi-colon separated, no cygdrive)

    SET(KWWidgets_PYTHONPATH_SEP ";")
    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      KWWidgets_PYTHONPATH_ENV "${KWWidgets_PYTHONPATH_ENV}")

    # Configure the Win32 batch file

    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.bat.in
      ${dest_dir}/KWWidgetsSetupPaths.bat
      IMMEDIATE)

    # For Cygwin PATH (colon separated, use cygdrive)
    # Yes, we could use the cygpath tool in the file directly

    STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    STRING(REGEX REPLACE ";" ":" 
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    # For Win32 TCLLIBPATH (space separated, no cygdrive)
    # Yes, we could use the cygpath tool in the file directly

    # For Cygwin PYTHONPATH (escaped semi-colon separated, no cygdrive)

    SET(KWWidgets_PYTHONPATH_SEP "\;")
    STRING(REGEX REPLACE ";" "\\\;" 
      KWWidgets_PYTHONPATH_ENV "${KWWidgets_PYTHONPATH_ENV}")

    # Configure the Cygwin bash/tcsh file

    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.sh.in
      ${dest_dir}/KWWidgetsSetupPaths.sh
      IMMEDIATE)
    
    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.csh.in
      ${dest_dir}/KWWidgetsSetupPaths.csh
      IMMEDIATE)

  ELSE(WIN32)

    # For Unix PATH (colon separated)

    STRING(REGEX REPLACE ";" ":" 
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    # For Unix TCLLIBPATH (space separated)

    # For Unix PYTHONPATH (colon separated)

    SET(KWWidgets_PYTHONPATH_SEP ":")
    STRING(REGEX REPLACE ";" ":" 
      KWWidgets_PYTHONPATH_ENV "${KWWidgets_PYTHONPATH_ENV}")

    # Configure the Unix bash/tcsh file

    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.sh.in
      ${dest_dir}/KWWidgetsSetupPaths.sh
      IMMEDIATE)

    CONFIGURE_FILE(
      ${KWWidgets_SOURCE_DIR}/Wrapping/KWWidgetsSetupPaths.csh.in
      ${dest_dir}/KWWidgetsSetupPaths.csh
      IMMEDIATE)

  ENDIF(WIN32)
ENDMACRO(KWWidgets_GENERATE_SETTUPPATHS)

# Generate the all setup path scripts, i.e. for each configuration types

MACRO(KWWidgets_GENERATE_ALL_SETTUPPATHS 
    kwwidgets_lib_paths 
    kwwidgets_runtime_paths 
    kwwidgets_tcl_paths 
    kwwidgets_python_paths)

  IF(WIN32 AND CMAKE_CONFIGURATION_TYPES)

    # Update all paths with the configuration type

    FOREACH(config ${CMAKE_CONFIGURATION_TYPES})

      SET(kwwidgets_lib_paths2)
      FOREACH(dir ${kwwidgets_lib_paths})
        SET(kwwidgets_lib_paths2 
          ${kwwidgets_lib_paths2} "${dir}/${config}")
      ENDFOREACH(dir)

      SET(kwwidgets_runtime_paths2)
      FOREACH(dir ${kwwidgets_runtime_paths})
        SET(kwwidgets_runtime_paths2 
          ${kwwidgets_runtime_paths2} "${dir}/${config}")
      ENDFOREACH(dir)

      SET(kwwidgets_tcl_paths2)
      FOREACH(dir ${kwwidgets_tcl_paths})
        SET(kwwidgets_tcl_paths2 
          ${kwwidgets_tcl_paths2} "${dir}/${config}")
      ENDFOREACH(dir)

      KWWidgets_GENERATE_SETTUPPATHS(
        ${EXECUTABLE_OUTPUT_PATH}/${config}
        "${kwwidgets_lib_paths2}"
        "${kwwidgets_runtime_paths2} "
        "${kwwidgets_tcl_paths2}"
        "${kwwidgets_python_paths}")
      
    ENDFOREACH(config)

  ELSE(WIN32 AND CMAKE_CONFIGURATION_TYPES)

    KWWidgets_GENERATE_SETTUPPATHS(
      ${EXECUTABLE_OUTPUT_PATH}
      "${kwwidgets_lib_paths}"
      "${kwwidgets_runtime_paths}"
      "${kwwidgets_tcl_paths}"
      "${kwwidgets_python_paths}")

  ENDIF(WIN32 AND CMAKE_CONFIGURATION_TYPES)
ENDMACRO(KWWidgets_GENERATE_ALL_SETTUPPATHS)

# Let's Generate the setup path scripts

SET(KWWidgets_LIBRARY_PATHS 
  ${VTK_LIBRARY_DIRS} 
  ${ITK_LIBRARY_DIRS} 
  ${SOV_LIBRARY_DIRS} 
  ${LIBRARY_OUTPUT_PATH})
SET(KWWidgets_RUNTIME_PATHS 
  ${VTK_RUNTIME_DIRS} 
  ${SOV_RUNTIME_DIRS}
  ${EXECUTABLE_OUTPUT_PATH})
SET(KWWidgets_TCL_PATHS 
  "${VTK_DIR}/Wrapping/Tcl" "${KWWidgets_BINARY_DIR}/Wrapping/Tcl")
SET(KWWidgets_PYTHON_PATHS
  "${VTK_DIR}/Wrapping/Python" "${KWWidgets_BINARY_DIR}/Wrapping/Python")

KWWidgets_GENERATE_ALL_SETTUPPATHS(
  "${KWWidgets_LIBRARY_PATHS}"
  "${KWWidgets_RUNTIME_PATHS}"
  "${KWWidgets_TCL_PATHS}"
  "${KWWidgets_PYTHON_PATHS}")
