include_directories(
  ${TestDriver_SOURCE_DIR})

set(CP_LABELS PARAVIEW CATALYST)

#------------------------------------------------------------------------------
vtk_module_test_executable(CoProcessingPythonScriptExample
  PythonScriptCoProcessingExample.cxx
  vtkPVCustomTestDriver.cxx)

#------------------------------------------------------------------------------
# Generate the image compare tool.
# below is for doing image comparisons
vtk_module_test_executable(CoProcessingCompareImagesTester CompareImages.cxx)

#------------------------------------------------------------------------------
# a simple test to see if the input is changing, i.e. that the initial
# pipeline is having it's trivial producer updated with a new grid
add_test(NAME CoProcessingTestInput
  COMMAND pvpython ${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
  ${CMAKE_CURRENT_SOURCE_DIR}/TestInput.py 4
  )
set_tests_properties(CoProcessingTestInput PROPERTIES LABELS "${CP_LABELS}")



# the CoProcessingTestPythonScript needs to be run with ${MPIEXEC} if
# CoProcessingPythonScriptExample was built with MPI because certain
# machines only allow running MPI programs with the proper ${MPIEXEC}

# just test if a given Catalyst Python script can import another script
if (NOT PARAVIEW_USE_MPI)
  add_test(NAME CoProcessingImport
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cpimport.py
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
else()
  add_test(NAME CoProcessingImport
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cpimport.py
    -DUSE_MPI:BOOL=TRUE
    -DMPIEXEC:FILEPATH=${MPIEXEC}
    -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
    -DMPIEXEC_NUMPROCS=2
    -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
    -DVTK_MPI_POSTFLAGS:STRING=${VTK_MPI_POSTFLAGS}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
endif()

set_tests_properties(CoProcessingImport PROPERTIES LABELS "${CP_LABELS}")

# test if we can use a Python programmable filter in a Catalyst Python script
if (NOT PARAVIEW_USE_MPI)
  add_test(NAME CoProcessingProgrammableFilter
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cpprogrammablefilter.py
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
else()
  add_test(NAME CoProcessingProgrammableFilter
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cpprogrammablefilter.py
    -DUSE_MPI:BOOL=TRUE
    -DMPIEXEC:FILEPATH=${MPIEXEC}
    -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
    -DMPIEXEC_NUMPROCS=2
    -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
    -DVTK_MPI_POSTFLAGS:STRING=${VTK_MPI_POSTFLAGS}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
endif()

set_tests_properties(CoProcessingProgrammableFilter PROPERTIES LABELS "${CP_LABELS}")
#-  add_test(NAME CoProcessingProgrammableFilter
#-    COMMAND CoProcessingProgrammableFilter
#-    ${CMAKE_CURRENT_SOURCE_DIR}/cpprogrammablefilter.py)









if (NOT PARAVIEW_USE_MPI)
  add_test(NAME CoProcessingTestPythonScript
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
    -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
    -DCOPROCESSING_DATA_DIR:PATH=${PARAVIEW_TEST_OUTPUT_BASELINE_DIR}
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTest.py
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
else()
  vtk_mpi_link(CoProcessingPythonScriptExample)
  add_test(NAME CoProcessingTestPythonScript
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
    -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
    -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
    -DCOPROCESSING_DATA_DIR:PATH=${PARAVIEW_TEST_OUTPUT_BASELINE_DIR}
    -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTest.py
    -DUSE_MPI:BOOL=TRUE
    -DMPIEXEC:FILEPATH=${MPIEXEC}
    -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
    -DMPIEXEC_NUMPROCS=1
    -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
    -DVTK_MPI_POSTFLAGS:STRING=${VTK_MPI_POSTFLAGS}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
endif()

set_tests_properties(CoProcessingTestPythonScript PROPERTIES LABELS "${CP_LABELS}")

if (PARAVIEW_USE_MPI)
  add_test(NAME PCoProcessingTestPythonScript
          COMMAND ${CMAKE_COMMAND}
          -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
          -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
          -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
          -DCOPROCESSING_DATA_DIR:PATH=${PARAVIEW_TEST_OUTPUT_BASELINE_DIR}
          -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTest.py
          -DUSE_MPI:BOOL=TRUE
          -DMPIEXEC:FILEPATH=${MPIEXEC}
          -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
          -DMPIEXEC_NUMPROCS=2
          -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
          -DVTK_MPI_POSTFLAGS:STRING=${VTK_MPI_POSTFLAGS}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
  # both the parallel and the serial tests create the same output images
  # so we run this one serially so that they don't interfere.
  set_tests_properties(PCoProcessingTestPythonScript PROPERTIES RUN_SERIAL ON)
  set_tests_properties(PCoProcessingTestPythonScript PROPERTIES LABELS "${CP_LABELS}")
endif()

if (PARAVIEW_BUILD_QT_GUI)
  # for now the full workflow test only works with the shared lib builds of
  # paraview since the static build will load all of the plugins automatically
  # which will add in extra properties to objects in the created script
  # which can't be used when running the 'simulation' since those same
  # plugins won't be loaded then.
  IF ( BUILD_SHARED_LIBS )
    add_test(NAME CoProcessingFullWorkflow
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:paraview>
      -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_DIR}
      -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Plugins/CatalystScriptGenerator/Testing/CatalystGUI.xml
      -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:pvpython>
      -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
      -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
      -DCOPROCESSING_DATA_DIR:PATH=${PARAVIEW_TEST_OUTPUT_BASELINE_DIR}
      -DCOPROCESSING_OUTPUTCHECK_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/outputcheck.py
      -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake)
    set_tests_properties(CoProcessingFullWorkflow PROPERTIES LABELS "${CP_LABELS}")
  ENDIF ()
endif()
