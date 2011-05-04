include_directories(
  ${TestDriver_SOURCE_DIR}
  )

IF (COPROCESSOR_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF (COPROCESSOR_USE_MPI)

IF (PARAVIEW_ENABLE_PYTHON)
  ADD_EXECUTABLE(CoProcessingPythonScriptExample PythonScriptCoProcessingExample.cxx vtkPVCustomTestDriver.cxx)
  TARGET_LINK_LIBRARIES(CoProcessingPythonScriptExample vtkCoProcessor vtkCPTestDriver)

  ADD_TEST(CoProcessingTestPythonScript ${EXECUTABLE_OUTPUT_PATH}/CoProcessingPythonScriptExample ${CoProcessing_SOURCE_DIR}/CoProcessor/Testing/Cxx/PythonScriptTest.py)


  IF (VTK_MPIRUN_EXE)
    ADD_TEST(PCoProcessingPythonScriptExample
      ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
      ${VTK_MPI_PREFLAGS}
      ${EXECUTABLE_OUTPUT_PATH}/CoProcessingPythonScriptExample
      ${CoProcessing_SOURCE_DIR}/CoProcessor/Testing/Cxx/PythonScriptTest2.py
      ${VTK_MPI_POSTFLAGS}
      )
  ENDIF (VTK_MPIRUN_EXE)


ENDIF (PARAVIEW_ENABLE_PYTHON)

  # below is for doing image comparisons
  # they are not done directly in the above python script due to the fact
  # that they would make the python script rather ugly
IF (PARAVIEW_DATA_ROOT)
  ADD_EXECUTABLE(CoProcessingCompareImagesTester CompareImages.cxx)
  TARGET_LINK_LIBRARIES(CoProcessingCompareImagesTester vtkCommon vtkIO vtkRendering)

  IF (PARAVIEW_ENABLE_PYTHON)
    ADD_TEST(CoProcessingPythonScriptGridPlot
      ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/CPGrid0.png 20 -V ${PARAVIEW_DATA_ROOT}/Baseline/CPGrid0.png -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx)

    ADD_TEST(CoProcessingPythonScriptPressurePlot
      ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/CPPressure0.png 20 -V ${PARAVIEW_DATA_ROOT}/Baseline/CPPressure0.png -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx)

    SET_TESTS_PROPERTIES(CoProcessingTestPythonScript CoProcessingPythonScriptGridPlot
      CoProcessingPythonScriptPressurePlot PROPERTIES RUN_SERIAL ON)
    IF (VTK_MPIRUN_EXE)
      ADD_TEST(PCoProcessingPythonScriptGridPlot
        ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/PCPGrid0.png 20 -V ${PARAVIEW_DATA_ROOT}/Baseline/CPGrid0.png -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx)

      ADD_TEST(PCoProcessingPythonScriptPressurePlot
        ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/PCPPressure0.png 20 -V ${PARAVIEW_DATA_ROOT}/Baseline/CPPressure0.png -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx)
    ENDIF (VTK_MPIRUN_EXE)

  ENDIF (PARAVIEW_ENABLE_PYTHON)

ENDIF (PARAVIEW_DATA_ROOT)

