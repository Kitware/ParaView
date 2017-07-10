if (EXISTS ${PVPYTHON_EXE})
  add_test(NAME import-rendering-base
    COMMAND ${PVPYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/use-rendering-base.py)
  set_tests_properties(import-rendering-base
    PROPERTIES
      FAIL_REGULAR_EXPRESSION "No proxy that matches:")
endif ()
