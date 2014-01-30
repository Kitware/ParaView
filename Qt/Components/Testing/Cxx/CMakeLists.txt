if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(MOC_SRCS FilteredPipelineBrowserApp.h)
else()
  QT4_WRAP_CPP(MOC_SRCS FilteredPipelineBrowserApp.h)
endif()
vtk_module_test_executable(pqPipelineApp FilteredPipelineBrowserApp.cxx FilteredPipelineBrowserApp.h ${MOC_SRCS})
if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
  set_target_properties(pqPipelineApp PROPERTIES
    COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
endif()

#ADD_TEST(pqPipelineApp "${EXECUTABLE_OUTPUT_PATH}/pqPipelineApp" -dr "--test-directory=${PARAVIEW_TEST_DIR}")
#set_tests_properties(pqPipelineApp PROPERTIES LABELS "PARAVIEW")
