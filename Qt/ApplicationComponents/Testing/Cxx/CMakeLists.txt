if(PARAVIEW_USE_PYTHON AND VTK_PYTHON_FULL_THREADSAFE)
  vtk_module_test_executable(pqApplicationComponentsTestVtkPythonScopeGilEnsurer TestVtkPythonScopeGilEnsurer.cxx ${MOC_SRCS})
  ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
    NAME pqApplicationComponentsTestVtkPythonScopeGilEnsurer
    COMMAND pqApplicationComponentsTestVtkPythonScopeGilEnsurer)
endif()

find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED COMPONENTS Core Widgets)
set(tests_sources
  TestCategoryCustomizations.cxx)
create_test_sourcelist(tests pqApplicationComponentsTest.cxx ${tests_sources})
vtk_module_test_executable(pqApplicationComponentsTest ${tests})
target_link_libraries(pqApplicationComponentsTest
  PRIVATE
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core"
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")

foreach (test_file IN LISTS tests_sources)
  get_filename_component(test "${test_file}" NAME_WE)
  add_test(
    NAME "pqApplicationComponents${test}"
    COMMAND pqApplicationComponentsTest ${test} --dr)
endforeach ()
