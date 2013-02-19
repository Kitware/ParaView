create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestDataEncoder.cxx
  EXTRA_INCLUDE vtkTestDriver.h)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests})
set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

if (PARAVIEW_DATA_ROOT)
  foreach (test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME ${vtk-module}-${TName}
             COMMAND ${vtk-module}CxxTests ${TName} -D ${PARAVIEW_DATA_ROOT})
    set_tests_properties(${vtk-module}-${TName} PROPERTIES LABELS "PARAVIEW;PARAVIEWWEB")
  endforeach()
endif()
