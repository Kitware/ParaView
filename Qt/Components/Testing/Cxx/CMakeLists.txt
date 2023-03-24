find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC 1)
vtk_module_test_executable(pqPipelineApp FilteredPipelineBrowserApp.cxx FilteredPipelineBrowserApp.h)
target_link_libraries(pqPipelineApp PRIVATE "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core" "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")
#ADD_TEST(pqPipelineApp "${EXECUTABLE_OUTPUT_PATH}/pqPipelineApp" -dr "--test-directory=${PARAVIEW_TEST_DIR}")

set(tests_sources
  TabbedMultiViewWidgetFilteringApp.cxx)
create_test_sourcelist(tests pqComponentsTest.cxx ${tests_sources})
vtk_module_test_executable(pqComponentsTest ${tests})
target_link_libraries(pqComponentsTest PRIVATE "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core" "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")

foreach(test_file IN LISTS tests_sources)
  get_filename_component(test "${test_file}" NAME_WE)
  add_test(
    NAME pqComponentsTest${test}
    COMMAND pqComponentsTest ${test} --exit)
endforeach()
