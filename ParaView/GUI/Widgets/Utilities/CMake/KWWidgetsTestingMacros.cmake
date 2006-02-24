# ---------------------------------------------------------------------------
# KWWidgets_ADD_OUT_OF_SOURCE_TEST
# Add an out-of-source test 

MACRO(KWWidgets_ADD_OUT_OF_SOURCE_TEST 
    test_name 
    project_name 
    src_dir bin_dir 
    exe_name exe_options)

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

    ADD_TEST(${test_name} ${CMAKE_CTEST_COMMAND}
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
      "-DTCL_TCLSH:FILEPATH=${TCL_TCLSH}"
      "-DTK_WISH:FILEPATH=${TK_WISH}"
      "-DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}"
      --test-command "${CONFIGURATION_TYPE}${exe_name}" "${exe_options}")

  ENDIF(VTK_WRAP_TCL)

ENDMACRO(KWWidgets_ADD_OUT_OF_SOURCE_TEST)
