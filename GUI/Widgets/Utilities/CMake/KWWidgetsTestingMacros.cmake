# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_WITH_LAUNCHER
# Add specific distribution-related C test

MACRO(KWWidgets_ADD_TEST_WITH_LAUNCHER
    test_name
    exe_name)

  INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsPathsMacros.cmake")
  SET(LAUNCHER_EXE_NAME "${exe_name}Launcher")
  KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER(
    "${CMAKE_CURRENT_BINARY_DIR}" "${LAUNCHER_EXE_NAME}" 
    "${EXECUTABLE_OUTPUT_PATH}" "${exe_name}")

  ADD_TEST(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${LAUNCHER_EXE_NAME} ${ARGN})

ENDMACRO(KWWidgets_ADD_TEST_WITH_LAUNCHER)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_FROM_EXAMPLE
# Add specific distribution-related test

MACRO(KWWidgets_ADD_TEST_FROM_EXAMPLE 
    test_name
    exe_name)

  ADD_TEST(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${exe_name} ${ARGN})

  IF(KWWidgets_SOURCE_DIR AND KWWidgets_TEST_OUT_OF_SOURCE)
    KWWidgets_ADD_OUT_OF_SOURCE_TEST(
      ${test_name}OutOfSource
      ${PROJECT_NAME}
      "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}OutOfSource"
      ${exe_name} ${ARGN})
  ENDIF(KWWidgets_SOURCE_DIR AND KWWidgets_TEST_OUT_OF_SOURCE)

ENDMACRO(KWWidgets_ADD_TEST_FROM_EXAMPLE)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_FROM_C_EXAMPLE
# Add specific distribution-related C test

MACRO(KWWidgets_ADD_TEST_FROM_C_EXAMPLE 
    test_name
    exe_name)

  KWWidgets_ADD_TEST_FROM_EXAMPLE(${test_name} ${exe_name} --test)

ENDMACRO(KWWidgets_ADD_TEST_FROM_C_EXAMPLE)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_FROM_TCL_EXAMPLE
# Add specific distribution-related Tcl test

MACRO(KWWidgets_ADD_TEST_FROM_TCL_EXAMPLE 
    test_name
    script_name)

  IF(KWWidgets_BUILD_SHARED_LIBS AND VTK_WRAP_TCL AND TCL_TCLSH)

    INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsPathsMacros.cmake")
    GET_FILENAME_COMPONENT(name_we "${script_name}" NAME_WE)
    SET(LAUNCHER_EXE_NAME "${name_we}TclLauncher")
    KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER(
      "${CMAKE_CURRENT_BINARY_DIR}" "${LAUNCHER_EXE_NAME}" 
      "" "${TCL_TCLSH}")
    
    KWWidgets_ADD_TEST_FROM_EXAMPLE(
      ${test_name} ${LAUNCHER_EXE_NAME} "\"${script_name}\"" "--test")

  ENDIF(KWWidgets_BUILD_SHARED_LIBS AND VTK_WRAP_TCL AND TCL_TCLSH)

ENDMACRO(KWWidgets_ADD_TEST_FROM_TCL_EXAMPLE)

# ---------------------------------------------------------------------------
# KWWidgets_ADD_TEST_FROM_PYTHON_EXAMPLE
# Add specific distribution-related Python test

MACRO(KWWidgets_ADD_TEST_FROM_PYTHON_EXAMPLE 
    test_name
    script_name)

  IF(KWWidgets_BUILD_SHARED_LIBS AND VTK_WRAP_PYTHON AND PYTHON_EXECUTABLE)

    INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsPathsMacros.cmake")
    GET_FILENAME_COMPONENT(name_we "${script_name}" NAME_WE)
    SET(LAUNCHER_EXE_NAME "${name_we}PythonLauncher")
    KWWidgets_GENERATE_SETUP_PATHS_LAUNCHER(
      "${CMAKE_CURRENT_BINARY_DIR}" "${LAUNCHER_EXE_NAME}" 
      "" "${PYTHON_EXECUTABLE}")

    KWWidgets_ADD_TEST_FROM_EXAMPLE(
      ${test_name} ${LAUNCHER_EXE_NAME} "\"${script_name}\"" "--test")

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
    
    SET(DEFAULT_CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
    IF(CMAKE_CONFIGURATION_TYPES)
      IF(NOT DEFAULT_CMAKE_BUILD_TYPE)
        FOREACH(var ${CMAKE_CONFIGURATION_TYPES})
          IF(NOT DEFAULT_CMAKE_BUILD_TYPE)
            SET(DEFAULT_CMAKE_BUILD_TYPE "${var}")
          ENDIF(NOT DEFAULT_CMAKE_BUILD_TYPE)
        ENDFOREACH(var)
      ENDIF(NOT DEFAULT_CMAKE_BUILD_TYPE)
      SET(CONFIGURATION_TYPE "${DEFAULT_CMAKE_BUILD_TYPE}/")
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
      --test-command "${CONFIGURATION_TYPE}${exe_name}" ${ARGN})

    IF(KWWidgets_TEST_INSTALLATION AND CMAKE_INSTALL_PREFIX)
      ADD_TEST("${test_name}UsingInst" ${CMAKE_CTEST_COMMAND}
        --build-and-test "${src_dir}" "${bin_dir}UsingInst"
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
        --test-command "${CONFIGURATION_TYPE}${exe_name}" ${ARGN})
    ENDIF(KWWidgets_TEST_INSTALLATION AND CMAKE_INSTALL_PREFIX)

  ENDIF(VTK_WRAP_TCL)

ENDMACRO(KWWidgets_ADD_OUT_OF_SOURCE_TEST)
