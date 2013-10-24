add_test(NAME import-base
  COMMAND ${PVPYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/import.py)
set_tests_properties(import-base
  PROPERTIES
    FAIL_REGULAR_EXPRESSION "No proxy that matches:")
