include_directories(
  ${TestDriver_SOURCE_DIR}
  )

IF (COPROCESSOR_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF (COPROCESSOR_USE_MPI)

IF (PARAVIEW_ENABLE_PYTHON)
  ADD_EXECUTABLE(CoProcessingPythonScriptExample PythonScriptCoProcessingExample.cxx vtkPVCustomTestDriver.cxx)
  TARGET_LINK_LIBRARIES(CoProcessingPythonScriptExample vtkCoProcessor vtkCPTestDriver)

  ADD_TEST(CoProcessingTestPythonScript ${EXECUTABLE_OUTPUT_PATH}/CoProcessingPythonScriptExample ${CoProcessing_SOURCE_DIR}/CoProcessor/Testing/Python/PythonScriptTest.py)

  IF (VTK_MPIRUN_EXE)
    ADD_TEST(PCoProcessingPythonScriptExample
      ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
      ${VTK_MPI_PREFLAGS}
      ${EXECUTABLE_OUTPUT_PATH}/CoProcessingPythonScriptExample
      ${CoProcessing_SOURCE_DIR}/CoProcessor/Testing/Python/PythonScriptTest2.py
      ${VTK_MPI_POSTFLAGS}
      )
  ENDIF (VTK_MPIRUN_EXE)


ENDIF (PARAVIEW_ENABLE_PYTHON)

# below is for doing image comparisons
# they are not done directly in the above python script due to the fact
# that they would make the python script rather ugly
ADD_EXECUTABLE(CoProcessingCompareImagesTester CompareImages.cxx)
TARGET_LINK_LIBRARIES(CoProcessingCompareImagesTester vtkCommon vtkIO vtkRendering)

IF (PARAVIEW_DATA_ROOT)
  IF (PARAVIEW_ENABLE_PYTHON)
    ADD_TEST(CoProcessingPythonScriptGridPlot
      ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester
      ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/CPGrid0.png 20 -V
      ${PARAVIEW_DATA_ROOT}/Baseline/CPGrid0.png
      -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx )

    ADD_TEST(CoProcessingPythonScriptPressurePlot
      ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester
      ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/CPPressure0.png 20
      -V ${PARAVIEW_DATA_ROOT}/Baseline/CPPressure0.png
      -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx)

    SET_TESTS_PROPERTIES(CoProcessingTestPythonScript CoProcessingPythonScriptGridPlot
      CoProcessingPythonScriptPressurePlot PROPERTIES RUN_SERIAL ON)
    IF (VTK_MPIRUN_EXE)
      ADD_TEST(PCoProcessingPythonScriptGridPlot
        ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester
        ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/PCPGrid0.png 20
        -V ${PARAVIEW_DATA_ROOT}/Baseline/CPGrid0.png
        -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx)

      ADD_TEST(PCoProcessingPythonScriptPressurePlot
        ${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester
        ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/PCPPressure0.png 20
        -V ${PARAVIEW_DATA_ROOT}/Baseline/CPPressure0.png
        -T ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx)
    ENDIF (VTK_MPIRUN_EXE)

    # here is the full workflow for coprocessing where we generate
    # a script, run a simulation driver code, and then check the output
    IF (PARAVIEW_BUILD_PLUGIN_CoProcessingScriptGenerator)
      set(REAL_EXECUTABLE_PATH ${EXECUTABLE_OUTPUT_PATH})
      if(Q_WS_MAC)
        set(REAL_EXECUTABLE_PATH ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.app/Contents/MacOS)
      endif(Q_WS_MAC)

      file(WRITE
"${CoProcessing_BINARY_DIR}/CoProcessingFullWorkflowTest.cmake"
"set(fullexe \"${REAL_EXECUTABLE_PATH}/paraview\")
if(NOT EXISTS \${fullexe})
  set(fullexe \"${REAL_EXECUTABLE_PATH}/\${cfg}/paraview.exe\")
endif()
FILE(REMOVE \"${CMAKE_BINARY_DIR}/Testing/Temporary/cptest.py ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/image_0.png ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/filename_0.pvtp ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/filename_0_0.vtp\" )

set(ARGS \"-dr;--test-directory=${CMAKE_BINARY_DIR}/Testing/Temporary;--test-script=${CMAKE_SOURCE_DIR}/Plugins/CoProcessingScriptGenerator/Testing/CoProcessingGUI.xml;--exit\")
execute_process(COMMAND \${fullexe} \${ARGS} RESULT_VARIABLE rv)
if(NOT rv EQUAL 0)
  message(FATAL_ERROR \"ParaView return value was \${rv}\")
endif()

set(fullexe \"${REAL_EXECUTABLE_PATH}/pvpython\")
if(NOT EXISTS \${fullexe})
  set(fullexe \"${REAL_EXECUTABLE_PATH}/\${cfg}/pvpython.exe\")
endif()
if(NOT EXISTS \${fullexe})
  set(fullexe \"${EXECUTABLE_OUTPUT_PATH}/pvpython\")
endif()
if(NOT EXISTS \${fullexe})
  message(FATAL_ERROR \"'\${fullexe}' does not exist\")
endif()

set(ARGS \"${CoProcessing_SOURCE_DIR}/CoProcessor/Testing/Python/waveletdriver.py;${CMAKE_BINARY_DIR}/Testing/Temporary/cptest.py;1\")
execute_process(COMMAND \${fullexe} \${ARGS} RESULT_VARIABLE failed)
if(failed)
  message(FATAL_ERROR \"pvpython return value was = '\${failed}' \")
endif()

set(fullexe \"${REAL_EXECUTABLE_PATH}/CoProcessingCompareImagesTester\")
if(NOT EXISTS \${fullexe})
  set(fullexe \"${REAL_EXECUTABLE_PATH}/\${cfg}/CoProcessingCompareImagesTester.exe\")
endif()
if(NOT EXISTS \${fullexe})
  set(fullexe \"${EXECUTABLE_OUTPUT_PATH}/CoProcessingCompareImagesTester\")
endif()
if(NOT EXISTS \${fullexe})
  message(FATAL_ERROR \"'\${fullexe}' does not exist\")
endif()

set(ARGS \"${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/image_0.png;20;-V;${PARAVIEW_DATA_ROOT}/Baseline/CPFullWorkflow.png;-T;${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx\")
execute_process(COMMAND \${fullexe} \${ARGS} RESULT_VARIABLE failed)
if(failed)
  message(FATAL_ERROR \"CoProcessingCompareImageTester return value was = '\${failed}' \")
endif()

set(fullexe \"${REAL_EXECUTABLE_PATH}/vtkpython\")
if(NOT EXISTS \${fullexe})
  set(fullexe \"${REAL_EXECUTABLE_PATH}/\${cfg}/vtkpython.exe\")
endif()
if(NOT EXISTS \${fullexe})
  set(fullexe \"${EXECUTABLE_OUTPUT_PATH}/vtkpython\")
endif()
if(NOT EXISTS \${fullexe})
  message(FATAL_ERROR \"'\${fullexe}' does not exist\")
endif()

set(ARGS \"${CoProcessing_SOURCE_DIR}/CoProcessor/Testing/Python/outputcheck.py;${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/filename_0.pvtp\")
execute_process(COMMAND \${fullexe} \${ARGS} RESULT_VARIABLE failed)
if(failed)
  message(FATAL_ERROR \"vtkpython return value was = '\${failed}' \")
endif()
" )

      add_test(NAME CoProcessingFullWorkflow COMMAND ${CMAKE_COMMAND}
        -Dcfg=$<CONFIGURATION> -P "${CoProcessing_BINARY_DIR}/CoProcessingFullWorkflowTest.cmake")
    ENDIF (PARAVIEW_BUILD_PLUGIN_CoProcessingScriptGenerator)

  ENDIF (PARAVIEW_ENABLE_PYTHON)

ENDIF (PARAVIEW_DATA_ROOT)

