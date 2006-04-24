# ---------------------------------------------------------------------------
# KWWidgets_GET_CMAKE_BUILD_TYPE
# Get CMAKE_BUILD_TYPE

MACRO(KWWidgets_GET_CMAKE_BUILD_TYPE varname)

  SET(cmake_build_type_found "${CMAKE_BUILD_TYPE}")
  IF(CMAKE_CONFIGURATION_TYPES)
    IF(NOT cmake_build_type_found)
      FOREACH(var ${CMAKE_CONFIGURATION_TYPES})
        IF(NOT cmake_build_type_found)
          SET(cmake_build_type_found "${var}")
        ENDIF(NOT cmake_build_type_found)
      ENDFOREACH(var)
    ENDIF(NOT cmake_build_type_found)
  ENDIF(CMAKE_CONFIGURATION_TYPES)
  
  SET(${varname} ${cmake_build_type_found})

ENDMACRO(KWWidgets_GET_CMAKE_BUILD_TYPE)

# ---------------------------------------------------------------------------
# KWWidgets_GET_FULL_PATH_TO_EXECUTABLE
# Get full path to exec

MACRO(KWWidgets_GET_FULL_PATH_TO_EXECUTABLE exe_name varname)

  GET_FILENAME_COMPONENT(exe_name_name "${exe_name}" NAME)
  IF("${exe_name_name}" STREQUAL "${exe_name}")
    IF(EXECUTABLE_OUTPUT_PATH)
      SET(exe_dir "${EXECUTABLE_OUTPUT_PATH}/")
    ELSE(EXECUTABLE_OUTPUT_PATH)
      SET(exe_dir "${CMAKE_CURRENT_BINARY_DIR}/")
    ENDIF(EXECUTABLE_OUTPUT_PATH)
  ELSE("${exe_name_name}" STREQUAL "${exe_name}")
    SET(exe_dir)
  ENDIF("${exe_name_name}" STREQUAL "${exe_name}")

  KWWidgets_GET_CMAKE_BUILD_TYPE(DEFAULT_CMAKE_BUILD_TYPE)
  IF(CMAKE_CONFIGURATION_TYPES)
    SET(CONFIGURATION_TYPE "${DEFAULT_CMAKE_BUILD_TYPE}/")
  ELSE(CMAKE_CONFIGURATION_TYPES)
    SET(CONFIGURATION_TYPE)
  ENDIF(CMAKE_CONFIGURATION_TYPES)

  SET(${varname} "${exe_dir}${CONFIGURATION_TYPE}${exe_name}")

ENDMACRO(KWWidgets_GET_FULL_PATH_TO_EXECUTABLE)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_WITH_LAUNCHER
# Add specific distribution-related C test

MACRO(KWWidgets_ADD_TEST_WITH_LAUNCHER
    test_name
    exe_name)

  # If we are building the test from the library itself, use the
  # unique launcher created by the library, instead of creating
  # a specific launcher when building out-of-source.
  
  IF(KWWidgets_SOURCE_DIR)
    SET(LAUNCHER_EXE_NAME "KWWidgetsSetupPathsLauncher")
    KWWidgets_GET_FULL_PATH_TO_EXECUTABLE(${exe_name} exe_path)
    ADD_TEST(${test_name} 
      ${EXECUTABLE_OUTPUT_PATH}/${LAUNCHER_EXE_NAME} ${exe_path} ${ARGN})
  ELSE(KWWidgets_SOURCE_DIR)
    INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsPathsMacros.cmake")
    SET(LAUNCHER_EXE_NAME "${exe_name}Launcher")
    KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER(
      "${CMAKE_CURRENT_BINARY_DIR}" "${LAUNCHER_EXE_NAME}" 
      "${EXECUTABLE_OUTPUT_PATH}" "${exe_name}")
    ADD_TEST(${test_name} 
      ${EXECUTABLE_OUTPUT_PATH}/${LAUNCHER_EXE_NAME} ${ARGN})
  ENDIF(KWWidgets_SOURCE_DIR)

ENDMACRO(KWWidgets_ADD_TEST_WITH_LAUNCHER)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_FROM_EXAMPLE
# Add specific distribution-related test

MACRO(KWWidgets_ADD_TEST_FROM_EXAMPLE 
    test_name
    exe_name exe_options
    out_of_source_exe_name out_of_source_exe_options)

  ADD_TEST(${test_name} 
    ${EXECUTABLE_OUTPUT_PATH}/${exe_name} ${exe_options} ${ARGN})

  IF(KWWidgets_SOURCE_DIR AND KWWidgets_TEST_OUT_OF_SOURCE)
    KWWidgets_ADD_OUT_OF_SOURCE_TEST(
      ${test_name}OoS
      ${PROJECT_NAME}
      "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}OoS"
      ${out_of_source_exe_name} ${out_of_source_exe_options} ${ARGN})
  ENDIF(KWWidgets_SOURCE_DIR AND KWWidgets_TEST_OUT_OF_SOURCE)

ENDMACRO(KWWidgets_ADD_TEST_FROM_EXAMPLE)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_FROM_C_EXAMPLE
# Add specific distribution-related C test

MACRO(KWWidgets_ADD_TEST_FROM_C_EXAMPLE 
    test_name
    exe_name)

  # Try to find the full path to the test executable

  KWWidgets_GET_FULL_PATH_TO_EXECUTABLE(${exe_name} exe_path)

  # If we are building the test from the library itself, use the
  # unique launcher created by the library, instead of creating
  # a specific launcher when building out-of-source.
  
  SET(LAUNCHER_EXE_NAME "${exe_name}Launcher")

  IF(KWWidgets_SOURCE_DIR)
    KWWidgets_ADD_TEST_FROM_EXAMPLE(${test_name} 
     "KWWidgetsSetupPathsLauncher" "${exe_path}"
     ${LAUNCHER_EXE_NAME} ""
     "--test")
  ELSE(KWWidgets_SOURCE_DIR)
    # No need to create a launcher (supposed to be done by the example already)
    KWWidgets_ADD_TEST_FROM_EXAMPLE(${test_name} 
      ${LAUNCHER_EXE_NAME} ""
      ${LAUNCHER_EXE_NAME} ""
      "--test")
  ENDIF(KWWidgets_SOURCE_DIR)

ENDMACRO(KWWidgets_ADD_TEST_FROM_C_EXAMPLE)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_FROM_TCL_EXAMPLE
# Add specific distribution-related Tcl test

MACRO(KWWidgets_ADD_TEST_FROM_TCL_EXAMPLE 
    test_name
    script_name)

  IF(KWWidgets_BUILD_SHARED_LIBS AND VTK_WRAP_TCL AND TCL_TCLSH)

    GET_FILENAME_COMPONENT(name_we "${script_name}" NAME_WE)
    SET(LAUNCHER_EXE_NAME "${name_we}TclLauncher")

    # If we are building the test from the library itself, use the
    # unique launcher created by the library, instead of creating
    # a specific launcher when building out-of-source.

    IF(KWWidgets_SOURCE_DIR)
      KWWidgets_ADD_TEST_FROM_EXAMPLE(${test_name} 
        "KWWidgetsSetupPathsLauncher" "${TCL_TCLSH}"
        ${LAUNCHER_EXE_NAME} ""
        "${script_name}" "--test")
    ELSE(KWWidgets_SOURCE_DIR)
      INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsPathsMacros.cmake")
      KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER(
        "${CMAKE_CURRENT_BINARY_DIR}" "${LAUNCHER_EXE_NAME}" "" "${TCL_TCLSH}")
      KWWidgets_ADD_TEST_FROM_EXAMPLE(${test_name} 
        ${LAUNCHER_EXE_NAME} "" 
        ${LAUNCHER_EXE_NAME} "" 
        "${script_name}" "--test")
    ENDIF(KWWidgets_SOURCE_DIR)

  ENDIF(KWWidgets_BUILD_SHARED_LIBS AND VTK_WRAP_TCL AND TCL_TCLSH)

ENDMACRO(KWWidgets_ADD_TEST_FROM_TCL_EXAMPLE)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_FROM_PYTHON_EXAMPLE
# Add specific distribution-related Python test

MACRO(KWWidgets_ADD_TEST_FROM_PYTHON_EXAMPLE 
    test_name
    script_name)

  IF(KWWidgets_BUILD_SHARED_LIBS AND VTK_WRAP_PYTHON AND PYTHON_EXECUTABLE)

    GET_FILENAME_COMPONENT(name_we "${script_name}" NAME_WE)
    SET(LAUNCHER_EXE_NAME "${name_we}PythonLauncher")

    # If we are building the test from the library itself, use the
    # unique launcher created by the library, instead of creating
    # a specific launcher when building out-of-source.

    IF(KWWidgets_SOURCE_DIR)
      KWWidgets_ADD_TEST_FROM_EXAMPLE(${test_name} 
        "KWWidgetsSetupPathsLauncher" "${PYTHON_EXECUTABLE}"
        ${LAUNCHER_EXE_NAME} ""
        "${script_name}" "--test")
    ELSE(KWWidgets_SOURCE_DIR)
      INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsPathsMacros.cmake")
      KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER(
        "${CMAKE_CURRENT_BINARY_DIR}" "${LAUNCHER_EXE_NAME}" "" "${PYTHON_EXECUTABLE}")
      KWWidgets_ADD_TEST_FROM_EXAMPLE(${test_name} 
        ${LAUNCHER_EXE_NAME} "" 
        ${LAUNCHER_EXE_NAME} "" 
        "${script_name}" "--test")
    ENDIF(KWWidgets_SOURCE_DIR)

  ENDIF(KWWidgets_BUILD_SHARED_LIBS AND VTK_WRAP_PYTHON AND PYTHON_EXECUTABLE)

ENDMACRO(KWWidgets_ADD_TEST_FROM_PYTHON_EXAMPLE)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_OUT_OF_SOURCE_TEST
# Add an out-of-source test 

MACRO(KWWidgets_ADD_OUT_OF_SOURCE_TEST 
    test_name 
    project_name 
    src_dir bin_dir 
    exe_name)

  IF(VTK_WRAP_TCL)

    KWWidgets_GET_CMAKE_BUILD_TYPE(DEFAULT_CMAKE_BUILD_TYPE)
    IF(CMAKE_CONFIGURATION_TYPES)
      SET(CONFIGURATION_TYPE "${DEFAULT_CMAKE_BUILD_TYPE}/")
    ELSE(CMAKE_CONFIGURATION_TYPES)
      SET(CONFIGURATION_TYPE)
    ENDIF(CMAKE_CONFIGURATION_TYPES)

    ADD_TEST("${test_name}" ${CMAKE_CTEST_COMMAND}
      --build-and-test "${src_dir}" "${bin_dir}"
      --build-generator ${CMAKE_GENERATOR}
      --build-makeprogram ${CMAKE_MAKE_PROGRAM}
      --build-project ${project_name}
      --build-config ${DEFAULT_CMAKE_BUILD_TYPE}
      --build-options 
      "-DKWWidgets_DIR:PATH=${KWWidgets_BINARY_DIR}" 
      "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
      "-DSOV_DIR:PATH=${SOV_DIR}"
      "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
      "-DBUILD_TESTING:BOOL=ON"
      "-DTCL_TCLSH:FILEPATH=${TCL_TCLSH}"
      "-DTK_WISH:FILEPATH=${TK_WISH}"
      "-DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}"
      "-DGETTEXT_INTL_LIBRARY:FILEPATH=${GETTEXT_INTL_LIBRARY}"
      "-DGETTEXT_INCLUDE_DIR:PATH=${GETTEXT_INCLUDE_DIR}"
      "-DGETTEXT_MSGCAT_EXECUTABLE:FILEPATH=${GETTEXT_MSGCAT_EXECUTABLE}"
      "-DGETTEXT_MSGCONV_EXECUTABLE:FILEPATH=${GETTEXT_MSGCONV_EXECUTABLE}"
      "-DGETTEXT_MSGFMT_EXECUTABLE:FILEPATH=${GETTEXT_MSGFMT_EXECUTABLE}"
      "-DGETTEXT_MSGINIT_EXECUTABLE:FILEPATH=${GETTEXT_MSGINIT_EXECUTABLE}"
      "-DGETTEXT_MSGMERGE_EXECUTABLE:FILEPATH=${GETTEXT_MSGMERGE_EXECUTABLE}"
      "-DGETTEXT_XGETTEXT_EXECUTABLE:FILEPATH=${GETTEXT_XGETTEXT_EXECUTABLE}"
      --test-command "${CONFIGURATION_TYPE}${exe_name}" ${ARGN})

    IF(KWWidgets_TEST_INSTALLATION AND CMAKE_INSTALL_PREFIX)
      ADD_TEST("${test_name}wInst" ${CMAKE_CTEST_COMMAND}
        --build-and-test "${src_dir}" "${bin_dir}wInst"
        --build-generator ${CMAKE_GENERATOR}
        --build-makeprogram ${CMAKE_MAKE_PROGRAM}
        --build-project ${project_name}
        --build-config ${DEFAULT_CMAKE_BUILD_TYPE}
        --build-options 
        "-DKWWidgets_DIR:PATH=${CMAKE_INSTALL_PREFIX}/lib/KWWidgets" 
        "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
        "-DSOV_DIR:PATH=${SOV_DIR}"
        "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
        "-DBUILD_TESTING:BOOL=ON"
        "-DTCL_TCLSH:FILEPATH=${TCL_TCLSH}"
        "-DTK_WISH:FILEPATH=${TK_WISH}"
        "-DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}"
        "-DGETTEXT_INTL_LIBRARY:FILEPATH=${GETTEXT_INTL_LIBRARY}"
        "-DGETTEXT_INCLUDE_DIR:PATH=${GETTEXT_INCLUDE_DIR}"
        "-DGETTEXT_MSGCAT_EXECUTABLE:FILEPATH=${GETTEXT_MSGCAT_EXECUTABLE}"
        "-DGETTEXT_MSGCONV_EXECUTABLE:FILEPATH=${GETTEXT_MSGCONV_EXECUTABLE}"
        "-DGETTEXT_MSGFMT_EXECUTABLE:FILEPATH=${GETTEXT_MSGFMT_EXECUTABLE}"
        "-DGETTEXT_MSGINIT_EXECUTABLE:FILEPATH=${GETTEXT_MSGINIT_EXECUTABLE}"
        "-DGETTEXT_MSGMERGE_EXECUTABLE:FILEPATH=${GETTEXT_MSGMERGE_EXECUTABLE}"
        "-DGETTEXT_XGETTEXT_EXECUTABLE:FILEPATH=${GETTEXT_XGETTEXT_EXECUTABLE}"
        --test-command "${CONFIGURATION_TYPE}${exe_name}" ${ARGN})
    ENDIF(KWWidgets_TEST_INSTALLATION AND CMAKE_INSTALL_PREFIX)

  ENDIF(VTK_WRAP_TCL)

ENDMACRO(KWWidgets_ADD_OUT_OF_SOURCE_TEST)
