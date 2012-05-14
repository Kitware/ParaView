# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")
QT4_WRAP_CPP(MOC_SRCS FilteredPipelineBrowserApp.h)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

ADD_EXECUTABLE(pqPipelineApp FilteredPipelineBrowserApp.cxx FilteredPipelineBrowserApp.h ${MOC_SRCS})
TARGET_LINK_LIBRARIES(pqPipelineApp
  pqCore
  pqComponents
  QtTesting
  vtkPVClientServerCore
  vtkPVServerManager
  ${QT_LIBRARIES} )
 
#ADD_TEST(pqPipelineApp "${EXECUTABLE_OUTPUT_PATH}/pqPipelineApp" -dr "--test-directory=${PARAVIEW_TEST_DIR}")
