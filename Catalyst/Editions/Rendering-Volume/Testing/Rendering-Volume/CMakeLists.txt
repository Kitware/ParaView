if (EXISTS ${PVPYTHON_EXE})
  add_test(NAME import-rendering-volume
    COMMAND ${PVPYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/use-rendering-volume.py)
  set_tests_properties(import-rendering-volume
    PROPERTIES
      FAIL_REGULAR_EXPRESSION "No proxy that matches:;Could not create object of type:")
endif ()
