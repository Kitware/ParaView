add_executable(TestCatalystExtrasFilters
  TestCatalystExtrasFilters.cxx)
target_link_libraries(TestCatalystExtrasFilters
  vtkPVClientServerCoreCore
  vtkPVServerManagerApplication
  vtkPVServerManagerCore)

add_test(NAME TestCatalystExtrasFilters
  COMMAND TestCatalystExtrasFilters)

if (EXISTS ${PVPYTHON_EXE})
  add_test(NAME import-extras
    COMMAND ${PVPYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/use-extras.py)
  set_tests_properties(import-extras
    PROPERTIES
      FAIL_REGULAR_EXPRESSION "No proxy that matches:")
  add_test(NAME import-extras-io
    COMMAND ${PVPYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/use-io.py)
  set_tests_properties(import-extras-io
    PROPERTIES
      FAIL_REGULAR_EXPRESSION "No proxy that matches:")
endif ()
