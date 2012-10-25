cmake_minimum_required(VERSION 2.8)

set(STREAMINGTESTS
  Source.cxx
  Harness.cxx
  PieceCache.cxx
  Iteration.cxx
  Prioritization.cxx
  Refinement.cxx
)

## Create test source list
create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
    ${STREAMINGTESTS}
    EXTRA_INCLUDE vtkTestDriver.h
    )
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
remove(TestsToRun ${vtk-module}CxxTests.cxx)

## Add all executables
if (PARAVIEW_TEST_DIR AND PARAVIEW_DATA_ROOT)
  foreach(test ${TestsToRun})
    get_filename_component(t ${test} NAME_WE)
    add_test(NAME ${vtk-module}Cxx-${t}
      COMMAND ${vtk-module}CxxTests ${t}
              -T {PARAVIEW_TEST_DIR}
              -V ${PARAVIEW_DATA_ROOT}/Baseline/StreamingTest${t}.png)
    set_tests_properties(${vtk-module}Cxx-${t}
      PROPERTIES FAIL_REGULAR_EXPRESSION "not working"
                 LABELS "${${vtk-module}_TEST_LABELS}")
  endforeach()
endif()
