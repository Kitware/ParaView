INCLUDE_DIRECTORIES(${PARAVIEW_SOURCE_DIR}/Development)

ADD_EXECUTABLE(TestMultiBlock TestMultiBlock.cxx)
TARGET_LINK_LIBRARIES(TestMultiBlock vtkPVDevelopment)

ADD_TEST(TestMultiBlock ${CXX_TEST_PATH}/TestMultiBlock
      -D ${PARAVIEW_SOURCE_DIR}/Data
      -V Baseline/TestMultiBlock.png)

ADD_EXECUTABLE(TestHierarchicalBox TestHierarchicalBox.cxx)
TARGET_LINK_LIBRARIES(TestHierarchicalBox vtkPVDevelopment)

ADD_TEST(TestHierarchicalBox ${CXX_TEST_PATH}/TestHierarchicalBox
      -D ${PARAVIEW_SOURCE_DIR}/Data
      -V Baseline/TestHierarchicalBox.png)

ADD_EXECUTABLE(TestHierarchicalBoxPipeline TestHierarchicalBoxPipeline.cxx)
TARGET_LINK_LIBRARIES(TestHierarchicalBoxPipeline vtkPVDevelopment)

ADD_TEST(TestHierarchicalBoxPipeline ${CXX_TEST_PATH}/TestHierarchicalBoxPipeline
      -D ${PARAVIEW_SOURCE_DIR}/Data
      -V Baseline/TestHierarchicalBoxPipeline.png)

