QT4_WRAP_CPP(MOC_SRCS FilteredPipelineBrowserApp.h)
vtk_module_test_executable(pqPipelineApp FilteredPipelineBrowserApp.cxx FilteredPipelineBrowserApp.h ${MOC_SRCS})

#ADD_TEST(pqPipelineApp "${EXECUTABLE_OUTPUT_PATH}/pqPipelineApp" -dr "--test-directory=${PARAVIEW_TEST_DIR}")
#set_tests_properties(pqPipelineApp PROPERTIES LABELS "PARAVIEW")
