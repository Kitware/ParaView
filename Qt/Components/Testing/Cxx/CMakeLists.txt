include(ParaViewQt)
pv_find_package_qt(qt_targets REQUIRED QUIET
                QT4_COMPONENTS QtCore QtGui
                QT5_COMPONENTS Core Widgets)
pv_qt_wrap_cpp(MOC_SRCS FilteredPipelineBrowserApp.h)
vtk_module_test_executable(pqPipelineApp FilteredPipelineBrowserApp.cxx FilteredPipelineBrowserApp.h ${MOC_SRCS})
target_link_libraries(pqPipelineApp LINK_PRIVATE ${qt_targets})

#ADD_TEST(pqPipelineApp "${EXECUTABLE_OUTPUT_PATH}/pqPipelineApp" -dr "--test-directory=${PARAVIEW_TEST_DIR}")
#set_tests_properties(pqPipelineApp PROPERTIES LABELS "PARAVIEW")
