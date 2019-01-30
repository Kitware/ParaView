find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC 1)
vtk_module_test_executable(pqPipelineApp FilteredPipelineBrowserApp.cxx FilteredPipelineBrowserApp.h)
target_link_libraries(pqPipelineApp PRIVATE Qt5::Core Qt5::Widgets)

#ADD_TEST(pqPipelineApp "${EXECUTABLE_OUTPUT_PATH}/pqPipelineApp" -dr "--test-directory=${PARAVIEW_TEST_DIR}")
