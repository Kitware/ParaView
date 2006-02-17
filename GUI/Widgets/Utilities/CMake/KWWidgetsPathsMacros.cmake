# ---------------------------------------------------------------------------
# KWWidgets_GENERATE_ONE_SETUP_PATHS
# Generate one set of setup path scripts

MACRO(KWWidgets_GENERATE_ONE_SETUP_PATHS
    dest_dir
    vtk_lib_paths 
    vtk_runtime_paths 
    vtk_tcl_paths 
    vtk_python_paths
    itk_lib_paths 
    itk_runtime_paths 
    sov_lib_paths 
    sov_runtime_paths 
    kwwidgets_lib_paths 
    kwwidgets_runtime_paths 
    kwwidgets_tcl_paths 
    kwwidgets_python_paths)

  # For PATH

  SET(KWWidgets_PATH_ENV 
    ${vtk_runtime_paths}
    ${vtk_lib_paths}
    ${itk_runtime_paths}
    ${itk_lib_paths}
    ${sov_runtime_paths}
    ${sov_lib_paths}
    ${kwwidgets_runtime_paths}
    ${kwwidgets_lib_paths}
    )

  # If we have no TCL_LIBRARY or TCL_TCLSH, then we are probably being invoked
  # from an out-of-source example that is using either an installed VTK or
  # an installed KWWidgets. None of those projects export TCL_* variables
  # when they are installed. Let's try to find Tclsh at least.

  IF (NOT TCL_LIBRARY AND NOT TCL_TCLSH)
    INCLUDE(${CMAKE_ROOT}/Modules/FindTclsh.cmake)
  ENDIF (NOT TCL_LIBRARY AND NOT TCL_TCLSH)
  
  IF(TCL_LIBRARY)
    GET_FILENAME_COMPONENT(path "${TCL_LIBRARY}" PATH)
    SET(KWWidgets_PATH_ENV ${KWWidgets_PATH_ENV} ${path} "${path}/../bin")
  ENDIF(TCL_LIBRARY)

  IF(TK_LIBRARY)
    GET_FILENAME_COMPONENT(path "${TK_LIBRARY}" PATH)
    SET(KWWidgets_PATH_ENV ${KWWidgets_PATH_ENV} ${path} "${path}/../bin")
  ENDIF(TK_LIBRARY)

  IF(TCL_TCLSH)
    GET_FILENAME_COMPONENT(path "${TCL_TCLSH}" PATH)
    SET(KWWidgets_PATH_ENV ${KWWidgets_PATH_ENV} ${path})
  ENDIF(TCL_TCLSH)

  IF(PYTHON_EXECUTABLE)
    GET_FILENAME_COMPONENT(path "${PYTHON_EXECUTABLE}" PATH)
    SET(KWWidgets_PATH_ENV ${KWWidgets_PATH_ENV} ${path})
  ENDIF(PYTHON_EXECUTABLE)

  # For TCLLIBPATH (space separated)

  SET(KWWidgets_TCLLIBPATH_ENV)
  FOREACH(dir ${vtk_tcl_paths})
    SET(KWWidgets_TCLLIBPATH_ENV 
      "${KWWidgets_TCLLIBPATH_ENV} \"${dir}\"")
  ENDFOREACH(dir)
  FOREACH(dir ${kwwidgets_tcl_paths})
    SET(KWWidgets_TCLLIBPATH_ENV 
      "${KWWidgets_TCLLIBPATH_ENV} \"${dir}\"")
  ENDFOREACH(dir)
  
  # For PYTHONPATH
  
  SET(KWWidgets_PYTHONPATH_ENV 
    ${vtk_python_paths}
    ${vtk_lib_paths}
    ${kwwidgets_python_paths}
    ${kwwidgets_lib_paths}
    )

  IF(WIN32)

    # For Win32 PATH (semi-colon separated, no cygdrive)

    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    # For Win32 TCLLIBPATH (space separated, no cygdrive)

    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      KWWidgets_TCLLIBPATH_ENV "${KWWidgets_TCLLIBPATH_ENV}")

    # For Win32 PYTHONPATH (semi-colon separated, no cygdrive)

    SET(KWWidgets_PYTHONPATH_SEP ";")
    STRING(REGEX REPLACE "/cygdrive/(.)/" "\\1:/" 
      KWWidgets_PYTHONPATH_ENV "${KWWidgets_PYTHONPATH_ENV}")

    # Configure the Win32 batch file

    SET(KWWidgets_SEP ";")

    CONFIGURE_FILE(
      ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.bat.in
      ${dest_dir}/KWWidgetsSetupPaths.bat
      IMMEDIATE)

    STRING(REGEX REPLACE "\"" "\\\\\"" 
      KWWidgets_TCLLIBPATH_ENV_ESCAPED "${KWWidgets_TCLLIBPATH_ENV}")

    CONFIGURE_FILE(
      ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.cmake.in
      ${dest_dir}/KWWidgetsSetupPaths.cmake
      IMMEDIATE @ONLY)

    # For Cygwin PATH (colon separated, use cygdrive)
    # Yes, we could use the cygpath tool in the file directly

    STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    SET(KWWidgets_SEP ":")

    STRING(REGEX REPLACE ";" ${KWWidgets_SEP}
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    # For Win32 TCLLIBPATH (space separated, no cygdrive)
    # Yes, we could use the cygpath tool in the file directly

    # For Cygwin PYTHONPATH (escaped semi-colon separated, no cygdrive)

    SET(KWWidgets_PYTHONPATH_SEP "\;")
    STRING(REGEX REPLACE ";" "\\\;" 
      KWWidgets_PYTHONPATH_ENV "${KWWidgets_PYTHONPATH_ENV}")

    # Configure the Cygwin bash/tcsh file

    CONFIGURE_FILE(
      ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.sh.in
      ${dest_dir}/KWWidgetsSetupPaths.sh
      IMMEDIATE)
    
    CONFIGURE_FILE(
      ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.csh.in
      ${dest_dir}/KWWidgetsSetupPaths.csh
      IMMEDIATE)

  ELSE(WIN32)

    # For Unix PATH (colon separated)

    SET(KWWidgets_SEP ":")

    STRING(REGEX REPLACE ";" ${KWWidgets_SEP}
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    # For Unix TCLLIBPATH (space separated)

    # For Unix PYTHONPATH (colon separated)

    SET(KWWidgets_PYTHONPATH_SEP ":")
    STRING(REGEX REPLACE ";" ${KWWidgets_PYTHONPATH_SEP}
      KWWidgets_PYTHONPATH_ENV "${KWWidgets_PYTHONPATH_ENV}")

    # Configure the Unix bash/tcsh file

    CONFIGURE_FILE(
      ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.sh.in
      ${dest_dir}/KWWidgetsSetupPaths.sh
      IMMEDIATE)

    CONFIGURE_FILE(
      ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.csh.in
      ${dest_dir}/KWWidgetsSetupPaths.csh
      IMMEDIATE)

    STRING(REGEX REPLACE "\"" "\\\\\"" 
      KWWidgets_TCLLIBPATH_ENV_ESCAPED "${KWWidgets_TCLLIBPATH_ENV}")

    CONFIGURE_FILE(
      ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.cmake.in
      ${dest_dir}/KWWidgetsSetupPaths.cmake
      IMMEDIATE @ONLY)

  ENDIF(WIN32)
ENDMACRO(KWWidgets_GENERATE_ONE_SETUP_PATHS)

# ---------------------------------------------------------------------------
# KWWidgets_GENERATE_ALL_SETUP_PATHS
# Generate all sets of setup path scripts, i.e. one set for each configuration
# types, if configuration types are supported

MACRO(KWWidgets_GENERATE_ALL_SETUP_PATHS 
    output_path
    vtk_lib_paths 
    vtk_runtime_paths 
    vtk_tcl_paths 
    vtk_python_paths
    itk_lib_paths 
    itk_runtime_paths 
    sov_lib_paths 
    sov_runtime_paths 
    kwwidgets_lib_paths 
    kwwidgets_runtime_paths 
    kwwidgets_tcl_paths 
    kwwidgets_python_paths)

  IF(WIN32 AND CMAKE_CONFIGURATION_TYPES)

    # Update all paths with the configuration type

    FOREACH(config ${CMAKE_CONFIGURATION_TYPES})

      # VTK

      SET(vtk_lib_paths2)
      SET(vtk_runtime_paths2)
      SET(vtk_tcl_paths2)
      IF(VTK_CONFIGURATION_TYPES)
        FOREACH(dir ${vtk_lib_paths})
          SET(vtk_lib_paths2 ${vtk_lib_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
        FOREACH(dir ${vtk_runtime_paths})
          SET(vtk_runtime_paths2 ${vtk_runtime_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
        FOREACH(dir ${vtk_tcl_paths})
          SET(vtk_tcl_paths2 ${vtk_tcl_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
      ELSE(VTK_CONFIGURATION_TYPES)
        SET(vtk_lib_paths2 ${vtk_lib_paths})
        SET(vtk_runtime_paths2 ${vtk_runtime_paths})
        SET(vtk_tcl_paths2 ${vtk_tcl_paths})
      ENDIF(VTK_CONFIGURATION_TYPES)

      # ITK

      SET(itk_lib_paths2)
      SET(itk_runtime_paths2)
      IF(ITK_CONFIGURATION_TYPES)
        FOREACH(dir ${itk_lib_paths})
          SET(itk_lib_paths2 ${itk_lib_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
        FOREACH(dir ${itk_runtime_paths})
          SET(itk_runtime_paths2 ${itk_runtime_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
      ELSE(ITK_CONFIGURATION_TYPES)
        SET(itk_lib_paths2 ${itk_lib_paths})
        SET(itk_runtime_paths2 ${itk_runtime_paths})
      ENDIF(ITK_CONFIGURATION_TYPES)

      # SOV

      SET(sov_lib_paths2)
      SET(sov_runtime_paths2)
      IF(SOV_CONFIGURATION_TYPES)
        FOREACH(dir ${sov_lib_paths})
          SET(sov_lib_paths2 ${sov_lib_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
        FOREACH(dir ${sov_runtime_paths})
          SET(sov_runtime_paths2 ${sov_runtime_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
      ELSE(SOV_CONFIGURATION_TYPES)
        SET(sov_lib_paths2 ${sov_lib_paths})
        SET(sov_runtime_paths2 ${sov_runtime_paths})
      ENDIF(SOV_CONFIGURATION_TYPES)

      # KWWidgets

      SET(kwwidgets_lib_paths2)
      SET(kwwidgets_runtime_paths2)
      SET(kwwidgets_tcl_paths2)
      IF(KWWidgets_CONFIGURATION_TYPES)
        FOREACH(dir ${kwwidgets_lib_paths})
          SET(kwwidgets_lib_paths2 
            ${kwwidgets_lib_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
        FOREACH(dir ${kwwidgets_runtime_paths})
          SET(kwwidgets_runtime_paths2 
            ${kwwidgets_runtime_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
        FOREACH(dir ${kwwidgets_tcl_paths})
          SET(kwwidgets_tcl_paths2 
            ${kwwidgets_tcl_paths2} "${dir}/${config}")
        ENDFOREACH(dir)
      ELSE(KWWidgets_CONFIGURATION_TYPES)
        SET(kwwidgets_lib_paths2 ${kwwidgets_lib_paths})
        SET(kwwidgets_runtime_paths2 ${kwwidgets_runtime_paths})
        SET(kwwidgets_tcl_paths2 ${kwwidgets_tcl_paths})
      ENDIF(KWWidgets_CONFIGURATION_TYPES)

      # Generate

      KWWidgets_GENERATE_ONE_SETUP_PATHS(
        ${output_path}/${config}
        "${vtk_lib_paths2}" 
        "${vtk_runtime_paths2}" 
        "${vtk_tcl_paths2}" 
        "${vtk_python_paths}"
        "${itk_lib_paths2}" 
        "${itk_runtime_paths2}" 
        "${sov_lib_paths2}" 
        "${sov_runtime_paths2}" 
        "${kwwidgets_lib_paths2}"
        "${kwwidgets_runtime_paths2}"
        "${kwwidgets_tcl_paths2}"
        "${kwwidgets_python_paths}")
      
    ENDFOREACH(config)

  ELSE(WIN32 AND CMAKE_CONFIGURATION_TYPES)

    KWWidgets_GENERATE_ONE_SETUP_PATHS(
      ${output_path}
      "${vtk_lib_paths}" 
      "${vtk_runtime_paths}" 
      "${vtk_tcl_paths}" 
      "${vtk_python_paths}"
      "${itk_lib_paths}" 
      "${itk_runtime_paths}" 
      "${sov_lib_paths}" 
      "${sov_runtime_paths}" 
      "${kwwidgets_lib_paths}"
      "${kwwidgets_runtime_paths}"
      "${kwwidgets_tcl_paths}"
      "${kwwidgets_python_paths}")

  ENDIF(WIN32 AND CMAKE_CONFIGURATION_TYPES)
ENDMACRO(KWWidgets_GENERATE_ALL_SETUP_PATHS)

# ---------------------------------------------------------------------------
# KWWidgets_GENERATE_DEFAULT_SETUP_PATHS
# Generate all sets of setup path scripts for the default paths configured
# at the moment.

MACRO(KWWidgets_GENERATE_DEFAULT_SETUP_PATHS 
    output_path)

  # VTK

  SET(VTK_TCL_PATHS "${VTK_TCL_HOME}")

  IF(VTK_INSTALL_PREFIX)
    IF(WIN32)
      SET(VTK_PYTHON_PATHS "${VTK_TCL_HOME}/../site-packages")
    ELSE(WIN32)
      IF(PYTHON_EXECUTABLE)
        EXEC_PROGRAM("${PYTHON_EXECUTABLE}" ARGS "-V" OUTPUT_VARIABLE version)
        STRING(REGEX REPLACE "^(Python )([0-9]\\.[0-9])(.*)$" "\\2" 
          major_minor "${version}")
        SET(VTK_PYTHON_PATHS 
          "${VTK_TCL_HOME}/../python${major_minor}/site-packages")
      ELSE(PYTHON_EXECUTABLE)
        SET(VTK_PYTHON_PATHS "${VTK_TCL_HOME}/../python2.4/site-packages")
      ENDIF(PYTHON_EXECUTABLE)
    ENDIF(WIN32)
  ELSE(VTK_INSTALL_PREFIX)
    SET(VTK_PYTHON_PATHS "${VTK_TCL_HOME}/../Python")
  ENDIF(VTK_INSTALL_PREFIX)

  # ITK
  # Try to find out if ITK is installed
  GET_FILENAME_COMPONENT(name "${ITK_LIBRARY_DIRS}" NAME)
  IF("${name}" STREQUAL "InsightToolkit")
    SET(ITK_RUNTIME_DIRS "${ITK_LIBRARY_DIRS}/../../bin")
    SET(ITK_CONFIGURATION_TYPES)
  ELSE("${name}" STREQUAL "InsightToolkit")
    SET(ITK_RUNTIME_DIRS ${ITK_LIBRARY_DIRS})
    SET(ITK_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})
  ENDIF("${name}" STREQUAL "InsightToolkit")

  # KWWidgets

  SET(KWWidgets_TCL_PATHS "${KWWidgets_TCL_PACKAGE_INDEX_DIR}")

  SET(KWWidgets_PYTHON_PATHS ${KWWidgets_PYTHON_PATHS} 
    "${KWWidgets_PYTHON_MODULE_DIR}")
  
  KWWidgets_GENERATE_ALL_SETUP_PATHS(
    "${output_path}"
    "${VTK_LIBRARY_DIRS}"
    "${VTK_RUNTIME_DIRS}"
    "${VTK_TCL_PATHS}"
    "${VTK_PYTHON_PATHS}"
    "${ITK_LIBRARY_DIRS}"
    "${ITK_RUNTIME_DIRS}"
    "${SOV_LIBRARY_DIRS}"
    "${SOV_RUNTIME_DIRS}"
    "${KWWidgets_LIBRARY_DIRS}"
    "${KWWidgets_RUNTIME_DIRS}"
    "${KWWidgets_TCL_PATHS}"
    "${KWWidgets_PYTHON_PATHS}")
  
ENDMACRO(KWWidgets_GENERATE_DEFAULT_SETUP_PATHS)
