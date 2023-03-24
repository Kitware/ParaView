find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED COMPONENTS Core Widgets Test)
set(CMAKE_AUTOMOC 1)
vtk_module_test_executable(pqCoreBasicApp BasicApp.cxx BasicApp.h)
target_link_libraries(pqCoreBasicApp PRIVATE "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core" "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")
ExternalData_add_test("${_vtk_build_TEST_DATA_TARGET}"
  NAME pqCoreBasicApp
  COMMAND pqCoreBasicApp
          --dr
          "--test-directory=${_vtk_build_TEST_OUTPUT_DIRECTORY}"
          "--test-baseline=DATA{${_vtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Baseline/pqCoreBasicApp.png}"
          --exit
  )

set(MyTests
  LogViewerWidget.cxx
  KeySequences.cxx
)

set(MocSources
  LogViewerWidget.h
  KeySequences.h
)

create_test_sourcelist(Tests pqCoreTest.cxx ${MyTests})

vtk_module_test_executable(pqCoreTest ${Tests})
target_link_libraries(pqCoreTest PRIVATE "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core" "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets" "Qt${PARAVIEW_QT_MAJOR_VERSION}::Test")

foreach(test_file ${MyTests})
  get_filename_component(test "${test_file}" NAME_WE)
  add_test(
    NAME pqCore${test}
    COMMAND pqCoreTest ${test})
endforeach()
