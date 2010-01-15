IF (PARAVIEW_DATA_ROOT)

  include_directories(
    ${TestDriver_SOURCE_DIR}
    )
  
  ADD_EXECUTABLE(CoProcessingPythonScriptExample PythonScriptCoProcessingExample.cxx vtkPVCustomTestDriver.cxx)
  TARGET_LINK_LIBRARIES(CoProcessingPythonScriptExample vtkCoProcessor vtkCPTestDriver)
  
  ADD_TEST(CoProcessingTestPythonScript ${EXECUTABLE_OUTPUT_PATH}/CoProcessingPythonScriptExample ${CoProcessing_SOURCE_DIR}/CoProcessor/Testing/Cxx/PythonScriptTest.py)
  
  # below is for doing image comparisons
  # they are not done directly in the above python script due to the fact 
  # that they would make the python script rather ugly
  ADD_EXECUTABLE(CoProcessingCompareImagesTester CompareImages.cxx)
  TARGET_LINK_LIBRARIES(CoProcessingCompareImagesTester vtkCommon vtkIO vtkRendering)
  
  ADD_TEST(CoProcessingPythonScriptGridPlot
    ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/CPGrid0.png 20 -V ${PARAVIEW_DATA_ROOT}/Baseline/CPGrid0.png -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx)
  
  ADD_TEST(CoProcessingPythonScriptPressurePlot
    ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/CPPressure0.png 20 -V ${PARAVIEW_DATA_ROOT}/Baseline/CPPressure0.png -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx)
  
ENDIF (PARAVIEW_DATA_ROOT)

