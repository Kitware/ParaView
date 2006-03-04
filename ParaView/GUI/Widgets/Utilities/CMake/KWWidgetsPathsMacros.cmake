# ---------------------------------------------------------------------------
# KWWidgets_GENERATE_SETUP_PATHS_SCRIPTS
# Generate a set of small setup scripts (.bat, .sh, .csh) that can be sourced
# to setup the environment variables required to run an executable built
# against the current project and some known third-party dependencies (VTK, 
# ITK, SOViewer, KWWidgets, etc.)
# 'output_path': location (dir) where to store the generated scripts
# This macro can take an optional parameter 'basename':
# 'basename': basename for generated scripts (extension of originals are kept)

MACRO(KWWidgets_GENERATE_SETUP_PATHS_SCRIPTS 
    output_path)

  IF(NOT "${ARGN}" STREQUAL "")
    SET(basename "${ARGN}")
  ELSE(NOT "${ARGN}" STREQUAL "")
    SET(basename "KWWidgetsSetupPaths")
  ENDIF(NOT "${ARGN}" STREQUAL "")

  KWWidgets_GENERATE_SETUP_PATHS(
    "${output_path}"
    "${basename}"
    1
    0 "" "")
  
ENDMACRO(KWWidgets_GENERATE_SETUP_PATHS_SCRIPTS)

# ---------------------------------------------------------------------------
# KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER
# Generate a lightweight C launcher for a *specific* executable.
# The launcher sets up all the environments variables (PATH, TCLLIBPATH,
# LD_LIBRARY_PATH, etc.) required by this executable and some known 
# third-party dependencies (VTK, ITK, SOV, KWWidgets, etc.), before launching
# the executable itself. Note that the path to the executable to launch is 
# hard-coded in the launcher: do not move the target exe around, or copy the
# launcher to your installation tree, if any (ultimately, all the DLLs needed
# by a project should/could be stored together in the installation tree, and a
# launcher should not be needed in that case).
# This macro also create the corresponding executable target.
# 'output_path': location (dir) where to store the generated launcher C source
# 'basename': basename for both the generated C source and launcher exe
# 'exe_dir': location of the executable to generate a C launcher for
# 'exe_name': name of the executable to generate a C launcher for
# Note that if 'exe_name' is empty, the launcher will run the executable passed
# as first parameter. 
# Note that if If 'exe_dir' is empty, the launcher will change its working
# dir to the launcher directory, and therefore attempt to locate 'exe_name'
# in that same directory.

MACRO(KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER
    output_path
    basename
    exe_dir exe_name)

  KWWidgets_GENERATE_SETUP_PATHS(
    "${output_path}"
    "${basename}"
    0
    1 "${exe_dir}" "${exe_name}")
  
ENDMACRO(KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER)

# ---------------------------------------------------------------------------
# KWWidgets_GENERATE_SETUP_PATHS
# Front-end to the macro that generate the scripts or launcher for all known
# configuration types. This macro collects and fixes some of the paths that
# still need to be fixed or hacked in third-party packages, then call
# the macro that loop over all the configuration types.
# 'output_path': location (dir) where to store the generated scripts/launcher
# 'basename': basename for generated scripts (extension of originals are kept)
# 'generate_scripts': if true generate setup path scripts (.bat, .sh, .csh)
# 'generate_launcher': if true generate C launcher
# 'exe_dir': location of the executable to generate a C launcher for
# 'exe_name': name of the executable to generate a C launcher for

MACRO(KWWidgets_GENERATE_SETUP_PATHS 
    output_path
    basename
    generate_scripts
    generate_launcher exe_dir exe_name)

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
    IF(WIN32)
      SET(ITK_RUNTIME_DIRS "${ITK_LIBRARY_DIRS}/../../bin")
    ELSE(WIN32)
      SET(ITK_RUNTIME_DIRS "${ITK_LIBRARY_DIRS}")
    ENDIF(WIN32)
    SET(ITK_CONFIGURATION_TYPES)
  ELSE("${name}" STREQUAL "InsightToolkit")
    SET(ITK_RUNTIME_DIRS ${ITK_LIBRARY_DIRS})
    SET(ITK_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})
  ENDIF("${name}" STREQUAL "InsightToolkit")

  # KWWidgets

  SET(KWWidgets_TCL_PATHS "${KWWidgets_TCL_PACKAGE_INDEX_DIR}")

  SET(KWWidgets_PYTHON_PATHS ${KWWidgets_PYTHON_PATHS} 
    "${KWWidgets_PYTHON_MODULE_DIR}")

  KWWidgets_GENERATE_SETUP_PATHS_FOR_ALL_CONFIGURATION_TYPES(
    "${output_path}"
    "${basename}"
    "${generate_scripts}"
    "${generate_launcher}" "${exe_dir}" "${exe_name}"
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
  
ENDMACRO(KWWidgets_GENERATE_SETUP_PATHS)

# ---------------------------------------------------------------------------
# KWWidgets_GENERATE_SETUP_PATHS_FOR_ALL_CONFIGURATION_TYPES
# Generate the scripts and/or launcher for all known configuration types. 
# This macro loops over all configuration types for each project (say Debug,
# Release, RelDebWithInfo), postfix all the path parameters that need to be
# postfixed then generate the scripts/launcher for each config type in the
#  appropriate config directory.
# 'output_path': location (dir) where to store the generated scripts/launcher
# 'basename': basename for generated scripts (extension of originals are kept)
# 'generate_scripts': if true generate setup path scripts (.bat, .sh, .csh)
# 'generate_launcher': if true generate C launcher
# 'exe_dir': location of the executable to generate a C launcher for
# 'exe_name': name of the executable to generate a C launcher for

MACRO(KWWidgets_GENERATE_SETUP_PATHS_FOR_ALL_CONFIGURATION_TYPES 
    output_path
    basename
    generate_scripts
    generate_launcher exe_dir exe_name
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

    # Scripts are generated first, then executable (below)

    FOREACH(config ${CMAKE_CONFIGURATION_TYPES})

      KWWidgets_GENERATE_SETUP_PATHS_FOR_ONE_CONFIGURATION_TYPE(
        "${output_path}/${config}"
        "${basename}"
        "${config}"
        "${generate_scripts}"
        0 "" ""
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
      
    ENDFOREACH(config)

    # Executable to launch. We can not create one C file per config type
    # since we can't have the same executable target points to different
    # source files (one per each config type). Use the CMAKE_INTDIR
    # substitution instead: at compile time, CMAKE_INTDIR is replaced by
    # the right config type.

    KWWidgets_GENERATE_SETUP_PATHS_FOR_ONE_CONFIGURATION_TYPE(
      "${output_path}"
      "${basename}"
      "\"CMAKE_INTDIR\""
      0
      "${generate_launcher}" "${exe_dir}" "${exe_name}"
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

  ELSE(WIN32 AND CMAKE_CONFIGURATION_TYPES)

    KWWidgets_GENERATE_SETUP_PATHS_FOR_ONE_CONFIGURATION_TYPE(
      "${output_path}"
      "${basename}"
      ""
      "${generate_scripts}"
      "${generate_launcher}" "${exe_dir}" "${exe_name}"
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
ENDMACRO(KWWidgets_GENERATE_SETUP_PATHS_FOR_ALL_CONFIGURATION_TYPES)

# ---------------------------------------------------------------------------
# KWWidgets_GENERATE_SETUP_PATHS_FOR_ONE_CONFIGURATION_TYPE
# Generate the scripts and/or launcher for a specific config type. At this 
# point it is assumed all paths are correct and do not need any configuration
# type postfix (like Release/Debug, etc.)
# 'output_path': location (dir) where to store the generated scripts/launcher
# 'basename': basename for generated scripts (extension of originals are kept)
# 'config': the specific config type (Debug, Release, etc.)
# 'generate_scripts': if true generate setup path scripts (.bat, .sh, .csh)
# 'generate_launcher': if true generate C launcher
# 'exe_dir': location of the executable to generate a C launcher for
# 'exe_name': name of the executable to generate a C launcher for

MACRO(KWWidgets_GENERATE_SETUP_PATHS_FOR_ONE_CONFIGURATION_TYPE
    output_path
    basename
    config
    generate_scripts
    generate_launcher _exe_dir exe_name
    _vtk_lib_paths 
    _vtk_runtime_paths 
    _vtk_tcl_paths 
    vtk_python_paths
    _itk_lib_paths 
    _itk_runtime_paths 
    _sov_lib_paths 
    _sov_runtime_paths 
    _kwwidgets_lib_paths 
    _kwwidgets_runtime_paths 
    _kwwidgets_tcl_paths 
    kwwidgets_python_paths)

  SET(vtk_lib_paths ${_vtk_lib_paths})
  SET(vtk_runtime_paths ${_vtk_runtime_paths})
  SET(vtk_tcl_paths ${_vtk_tcl_paths})
    
  SET(itk_lib_paths ${_itk_lib_paths})
  SET(itk_runtime_paths ${_itk_runtime_paths})
    
  SET(sov_lib_paths ${_sov_lib_paths})
  SET(sov_runtime_paths ${_sov_runtime_paths})
    
  SET(kwwidgets_lib_paths ${_kwwidgets_lib_paths})
  SET(kwwidgets_runtime_paths ${_kwwidgets_runtime_paths})
  SET(kwwidgets_tcl_paths ${_kwwidgets_tcl_paths})
  
  SET(exe_dir ${_exe_dir})
    
  # Update some paths with the configuration type if needed

  IF(NOT "${config}" STREQUAL "" AND WIN32 AND CMAKE_CONFIGURATION_TYPES)

    # VTK

    IF(VTK_CONFIGURATION_TYPES)
      SET(vtk_lib_paths2)
      SET(vtk_runtime_paths2)
      SET(vtk_tcl_paths2)
      FOREACH(dir ${_vtk_lib_paths})
        SET(vtk_lib_paths2 ${vtk_lib_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      FOREACH(dir ${_vtk_runtime_paths})
        SET(vtk_runtime_paths2 ${vtk_runtime_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      FOREACH(dir ${_vtk_tcl_paths})
        SET(vtk_tcl_paths2 ${vtk_tcl_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      SET(vtk_lib_paths ${vtk_lib_paths2})
      SET(vtk_runtime_paths ${vtk_runtime_paths2})
      SET(vtk_tcl_paths ${vtk_tcl_paths2})
    ENDIF(VTK_CONFIGURATION_TYPES)

    # ITK

    IF(ITK_CONFIGURATION_TYPES)
      SET(itk_lib_paths2)
      SET(itk_runtime_paths2)
      FOREACH(dir ${_itk_lib_paths})
        SET(itk_lib_paths2 ${itk_lib_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      FOREACH(dir ${_itk_runtime_paths})
        SET(itk_runtime_paths2 ${itk_runtime_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      SET(itk_lib_paths ${itk_lib_paths2})
      SET(itk_runtime_paths ${itk_runtime_paths2})
    ENDIF(ITK_CONFIGURATION_TYPES)

    # SOV

    IF(SOV_CONFIGURATION_TYPES)
      SET(sov_lib_paths2)
      SET(sov_runtime_paths2)
      FOREACH(dir ${_sov_lib_paths})
        SET(sov_lib_paths2 ${sov_lib_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      FOREACH(dir ${_sov_runtime_paths})
        SET(sov_runtime_paths2 ${sov_runtime_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      SET(sov_lib_paths ${sov_lib_paths2})
      SET(sov_runtime_paths ${sov_runtime_paths2})
    ENDIF(SOV_CONFIGURATION_TYPES)

    # KWWidgets

    IF(KWWidgets_CONFIGURATION_TYPES)
      SET(kwwidgets_lib_paths2)
      SET(kwwidgets_runtime_paths2)
      SET(kwwidgets_tcl_paths2)
      FOREACH(dir ${_kwwidgets_lib_paths})
        SET(kwwidgets_lib_paths2 
          ${kwwidgets_lib_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      FOREACH(dir ${_kwwidgets_runtime_paths})
        SET(kwwidgets_runtime_paths2 
          ${kwwidgets_runtime_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      FOREACH(dir ${_kwwidgets_tcl_paths})
        SET(kwwidgets_tcl_paths2 
          ${kwwidgets_tcl_paths2} "${dir}/${config}")
      ENDFOREACH(dir)
      SET(kwwidgets_lib_paths ${kwwidgets_lib_paths2})
      SET(kwwidgets_runtime_paths ${kwwidgets_runtime_paths2})
      SET(kwwidgets_tcl_paths ${kwwidgets_tcl_paths2})
    ENDIF(KWWidgets_CONFIGURATION_TYPES)

    SET(exe_dir ${_exe_dir})
    IF(NOT "${exe_dir}" STREQUAL "")
      SET(exe_dir "${exe_dir}/${config}")
    ENDIF(NOT "${exe_dir}" STREQUAL "")

  ENDIF(NOT "${config}" STREQUAL "" AND WIN32 AND CMAKE_CONFIGURATION_TYPES)

  # Executable to launch

  IF(${generate_launcher})
    SET(EXECUTABLE)
    IF(NOT "${exe_dir}" STREQUAL "")
      SET(EXECUTABLE "${exe_dir}/")
    ENDIF(NOT "${exe_dir}" STREQUAL "")
    SET(EXECUTABLE "${EXECUTABLE}${exe_name}")
  ENDIF(${generate_launcher})

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
    ${EXECUTABLE_OUTPUT_PATH}
    ${LIBRARY_OUTPUT_PATH}
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

  IF(CMAKE_MAKE_PROGRAM)
    GET_FILENAME_COMPONENT(path "${CMAKE_MAKE_PROGRAM}" PATH)
    SET(KWWidgets_PATH_ENV ${KWWidgets_PATH_ENV} ${path})
  ENDIF(CMAKE_MAKE_PROGRAM)

  # For LD_LIBRARY_PATH or equivalent
  
  IF (WIN32)
    # there is no equivalent on Windows, use LD_LIBRARY_PATH as fake
    SET(SHARED_LIBRARY_PATH_VAR_NAME "LD_LIBRARY_PATH")
    SET(KWWidgets_SHARED_LIBRARY_PATH_ENV "")
  ELSE (WIN32)
    # Try to inherit the path variable name from VTK
    IF (VTK_RUNTIME_PATH_VAR_NAME)
      SET (SHARED_LIBRARY_PATH_VAR_NAME ${VTK_RUNTIME_PATH_VAR_NAME})
    ELSE (VTK_RUNTIME_PATH_VAR_NAME)
      SET (SHARED_LIBRARY_PATH_VAR_NAME "LD_LIBRARY_PATH")
      IF (APPLE)
        SET (SHARED_LIBRARY_PATH_VAR_NAME "DYLD_LIBRARY_PATH")
      ENDIF (APPLE)
    ENDIF (VTK_RUNTIME_PATH_VAR_NAME)
    SET(KWWidgets_SHARED_LIBRARY_PATH_ENV
        ${vtk_runtime_paths}
        ${itk_runtime_paths}
        ${sov_runtime_paths}
        ${kwwidgets_runtime_paths})
  ENDIF (WIN32)

  # For TCLLIBPATH (space separated)

  SET(KWWidgets_TCLLIBPATH_ENV)
  FOREACH(dir ${vtk_tcl_paths})
    SET(KWWidgets_TCLLIBPATH_ENV 
      "${KWWidgets_TCLLIBPATH_ENV} {${dir}}")
  ENDFOREACH(dir)
  FOREACH(dir ${kwwidgets_tcl_paths})
    SET(KWWidgets_TCLLIBPATH_ENV 
      "${KWWidgets_TCLLIBPATH_ENV} {${dir}}")
  ENDFOREACH(dir)
  
  # For PYTHONPATH
  
  SET(KWWidgets_PYTHONPATH_ENV 
    ${vtk_python_paths}
    ${vtk_runtime_paths}
    ${kwwidgets_python_paths}
    ${kwwidgets_runtime_paths}
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

    SET(KWWidgets_PATH_SEP ";")

    IF(${generate_scripts})
      CONFIGURE_FILE(
        ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.bat.in
        ${output_path}/${basename}.bat
        IMMEDIATE)
    ENDIF(${generate_scripts})

    IF(${generate_scripts})
      CONFIGURE_FILE(
        ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.cmake.in
        ${output_path}/${basename}.cmake
        IMMEDIATE @ONLY)
    ENDIF(${generate_scripts})

    IF(${generate_launcher})
      CONFIGURE_FILE(
        ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPathsLauncher.c.in
        ${output_path}/${basename}.c
        IMMEDIATE @ONLY)
    ENDIF(${generate_launcher})

    # For Cygwin PATH (colon separated, use cygdrive)
    # Yes, we could use the cygpath tool in the file directly

    STRING(REGEX REPLACE "(.):/" "/cygdrive/\\1/" 
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    SET(KWWidgets_PATH_SEP ":")

    STRING(REGEX REPLACE ";" ${KWWidgets_PATH_SEP}
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    # For Win32 TCLLIBPATH (space separated, no cygdrive)
    # Yes, we could use the cygpath tool in the file directly

    # For Cygwin PYTHONPATH (escaped semi-colon separated, no cygdrive)

    SET(KWWidgets_PYTHONPATH_SEP "\;")
    STRING(REGEX REPLACE ";" "\\\;" 
      KWWidgets_PYTHONPATH_ENV "${KWWidgets_PYTHONPATH_ENV}")

    # Configure the Cygwin bash/tcsh file

    IF(${generate_scripts})
      CONFIGURE_FILE(
        ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.sh.in
        ${output_path}/${basename}.sh
        IMMEDIATE)
    
      CONFIGURE_FILE(
        ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.csh.in
        ${output_path}/${basename}.csh
        IMMEDIATE)
    ENDIF(${generate_scripts})

  ELSE(WIN32)

    # For Unix PATH (colon separated)

    SET(KWWidgets_PATH_SEP ":")
    STRING(REGEX REPLACE ";" ${KWWidgets_PATH_SEP}
      KWWidgets_PATH_ENV "${KWWidgets_PATH_ENV}")

    # For Unix LD_LIBRARY_PATH (colon separated)

    SET(KWWidgets_SHARED_LIBRARY_PATH_SEP ":")
    STRING(REGEX REPLACE ";" ${KWWidgets_SHARED_LIBRARY_PATH_SEP}
      KWWidgets_SHARED_LIBRARY_PATH_ENV "${KWWidgets_SHARED_LIBRARY_PATH_ENV}")

    # For Unix TCLLIBPATH (space separated)

    # For Unix PYTHONPATH (colon separated)

    SET(KWWidgets_PYTHONPATH_SEP ":")
    STRING(REGEX REPLACE ";" ${KWWidgets_PYTHONPATH_SEP}
      KWWidgets_PYTHONPATH_ENV "${KWWidgets_PYTHONPATH_ENV}")

    # Configure the Unix bash/tcsh file

    IF(${generate_scripts})
      CONFIGURE_FILE(
        ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.sh.in
        ${output_path}/${basename}.sh
        IMMEDIATE)
      
      CONFIGURE_FILE(
        ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.csh.in
        ${output_path}/${basename}.csh
        IMMEDIATE)
    ENDIF(${generate_scripts})

    CONFIGURE_FILE(
      ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPaths.cmake.in
      ${output_path}/${basename}.cmake
      IMMEDIATE @ONLY)

    IF(${generate_launcher})
      CONFIGURE_FILE(
        ${KWWidgets_TEMPLATES_DIR}/KWWidgetsSetupPathsLauncher.c.in
        ${output_path}/${basename}.c
        IMMEDIATE @ONLY)
    ENDIF(${generate_launcher})

  ENDIF(WIN32)

  IF(${generate_launcher})
    ADD_EXECUTABLE(${basename} ${output_path}/${basename}.c)
  ENDIF(${generate_launcher})

ENDMACRO(KWWidgets_GENERATE_SETUP_PATHS_FOR_ONE_CONFIGURATION_TYPE)

