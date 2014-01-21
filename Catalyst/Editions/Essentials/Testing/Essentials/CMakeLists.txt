add_executable(TestCatalystEssentialsFilters
  TestCatalystEssentialsFilters.cxx)
target_link_libraries(TestCatalystEssentialsFilters
  vtkPVClientServerCoreCore
  vtkPVServerManagerApplication
  vtkPVServerManagerCore)

add_test(NAME TestCatalystEssentialsFilters
  COMMAND TestCatalystEssentialsFilters)

if (EXISTS ${PVPYTHON_EXE})
  add_test(NAME import-essentials
    COMMAND ${PVPYTHON_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/use-essentials.py)
  set_tests_properties(import-essentials
    PROPERTIES
      FAIL_REGULAR_EXPRESSION "No proxy that matches:")
endif ()
