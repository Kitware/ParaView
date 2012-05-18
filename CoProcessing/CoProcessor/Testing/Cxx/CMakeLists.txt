include_directories(
  ${TestDriver_SOURCE_DIR}
  )

IF (COPROCESSOR_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF (COPROCESSOR_USE_MPI)

# below is for doing image comparisons
# they are not done directly in the above python script due to the fact
# that they would make the python script rather ugly
ADD_EXECUTABLE(CoProcessingCompareImagesTester CompareImages.cxx)
TARGET_LINK_LIBRARIES(CoProcessingCompareImagesTester
  vtkTestingRendering
  vtkRenderingOpenGL
  vtkRenderingFreeTypeOpenGL
  vtkInteractionStyle)

# specify the real executable path since this gets
# put in different places on different architectures.
set(REAL_EXECUTABLE_PATH ${EXECUTABLE_OUTPUT_PATH})
set(PARAVIEW_EXECUTABLE_PATH ${EXECUTABLE_OUTPUT_PATH})
if(Q_WS_MAC)
  set(REAL_EXECUTABLE_PATH ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.app/Contents/MacOS)
  set(PARAVIEW_EXECUTABLE_PATH ${EXECUTABLE_OUTPUT_PATH}/paraview.app/Contents/MacOS)
  set(MAC_TEST_NAME "-Mac")
endif(Q_WS_MAC)


# Create a macro to make the cmake files for running coprocessing test
# MPIPREDEFS and MPIPOSTDEFS are optional and only used
# when running the test in parallel.
MACRO(CreateCoProcessingScriptTest TESTNAME PYTHONSCRIPT
    GRIDBASELINE PRESSUREBASELINE GRIDOUTPUT PRESSUREOUTPUT
    MPIPREDEFS MPIPOSTDEFS)
  file(WRITE
    "${CoProcessing_BINARY_DIR}/${TESTNAME}.cmake"
"set(fullexe \"${PARAVIEW_EXECUTABLE_PATH}/CoProcessingPythonScriptExample\")
if(NOT EXISTS \${fullexe})
  set(fullexe \"${PARAVIEW_EXECUTABLE_PATH}/\${cfg}/CoProcessingPythonScriptExample.exe\")
endif()
if(NOT EXISTS \${fullexe})
  message(FATAL_ERROR \"'\${fullexe}' does not exist\")
endif()

FILE(REMOVE \"${CMAKE_BINARY_DIR}/Testing/Temporary/${GRIDOUTPUT} ${CMAKE_BINARY_DIR}/Testing/Temporary/${PRESSUREOUTPUT}\" )

execute_process(COMMAND \${fullexe} ${CoProcessing_SOURCE_DIR}/CoProcessor/Testing/Python/${PYTHONSCRIPT} RESULT_VARIABLE rv)
if(NOT rv EQUAL 0)
  message(FATAL_ERROR \"Test executable return value was \${rv}\")
endif()

if(NOT EXISTS ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/${GRIDOUTPUT})
  message(FATAL_ERROR \"'${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/${GRIDOUTPUT}' was not created\")
endif()
if(NOT EXISTS ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/${PRESSUREOUTPUT})
  message(FATAL_ERROR \"'${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/${PRESSUREOUTPUT}' was not created\")
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

set(ARGS \"${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/${GRIDOUTPUT};20;-V;${PARAVIEW_DATA_ROOT}/Baseline/${GRIDBASELINE};-T;${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx\")
execute_process(COMMAND \${fullexe} \${ARGS} RESULT_VARIABLE failed)
if(failed)
  message(FATAL_ERROR \"Grid plot failure \")
endif()

set(ARGS \"${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/${PRESSUREOUTPUT};20;-V;${PARAVIEW_DATA_ROOT}/Baseline/${PRESSUREBASELINE};-T;${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx\")
execute_process(COMMAND \${fullexe} \${ARGS} RESULT_VARIABLE failed)
if(failed)
  message(FATAL_ERROR \"Grid plot failure \")
endif()
" )

  add_test(NAME ${TESTNAME} COMMAND ${CMAKE_COMMAND}
    -Dcfg=$<CONFIGURATION> -P "${CoProcessing_BINARY_DIR}/${TESTNAME}.cmake")

ENDMACRO(CreateCoProcessingScriptTest)

IF (PARAVIEW_ENABLE_PYTHON AND PARAVIEW_DATA_ROOT)
  ADD_EXECUTABLE(CoProcessingPythonScriptExample PythonScriptCoProcessingExample.cxx vtkPVCustomTestDriver.cxx)
  TARGET_LINK_LIBRARIES(CoProcessingPythonScriptExample vtkCoProcessor vtkCPTestDriver)

  CreateCoProcessingScriptTest(CoProcessingTestPythonScript PythonScriptTest.py
    CPGrid0.png CPPressure0.png CPGrid0.png CPPressure0.png "" "")


  IF (VTK_MPIRUN_EXE)
    CreateCoProcessingScriptTest(PCoProcessingTestPythonScript PythonScriptTest2.py
      CPGrid0.png CPPressure0.png PCPGrid0.png PCPPressure0.png
      "${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS} ${VTK_MPI_PREFLAGS}"
      "${VTK_MPI_POSTFLAGS}" )
  ENDIF (VTK_MPIRUN_EXE)


ENDIF (PARAVIEW_ENABLE_PYTHON AND PARAVIEW_DATA_ROOT)

# here is the full workflow for coprocessing where we generate
# a script, run simulation driver code, and then check the output
IF (PARAVIEW_DATA_ROOT AND PARAVIEW_BUILD_PLUGIN_CoProcessingScriptGenerator)
  file(WRITE
"${CoProcessing_BINARY_DIR}/CoProcessingFullWorkflowTest.cmake"
"set(fullexe \"${PARAVIEW_EXECUTABLE_PATH}/paraview\")
if(NOT EXISTS \${fullexe})
  set(fullexe \"${PARAVIEW_EXECUTABLE_PATH}/\${cfg}/paraview.exe\")
endif()
FILE(REMOVE \"${CMAKE_BINARY_DIR}/Testing/Temporary/cptest.py ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/image_0.png ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/filename_0.pvtp ${CoProcessing_BINARY_DIR}/CoProcessor/Testing/Cxx/filename_0_0.vtp\" )

set(ARGS \"-dr;--test-directory=${CMAKE_BINARY_DIR}/Testing/Temporary;--test-script=${CMAKE_SOURCE_DIR}/Plugins/CoProcessingScriptGenerator/Testing/CoProcessingGUI${MAC_TEST_NAME}.xml;--exit\")
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
  SET_TESTS_PROPERTIES(CoProcessingFullWorkflow PROPERTIES RUN_SERIAL ON)

ENDIF (PARAVIEW_DATA_ROOT AND PARAVIEW_BUILD_PLUGIN_CoProcessingScriptGenerator)