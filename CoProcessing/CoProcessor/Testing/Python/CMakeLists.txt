if (NOT PARAVIEW_DATA_ROOT)
  return()
endif()

include_directories(
  ${TestDriver_SOURCE_DIR})

vtk_module_test()

#------------------------------------------------------------------------------
add_executable(CoProcessingPythonScriptExample
  PythonScriptCoProcessingExample.cxx
  vtkPVCustomTestDriver.cxx)
target_link_libraries(CoProcessingPythonScriptExample
  vtkCoProcessorImplementation vtkCPTestDriver)

#------------------------------------------------------------------------------
# Generate the image compare tool.
# below is for doing image comparisons
add_executable(CoProcessingCompareImagesTester CompareImages.cxx)
target_link_libraries(CoProcessingCompareImagesTester
  vtkTestingRendering
  vtkRenderingOpenGL
  vtkRenderingFreeTypeOpenGL
  vtkInteractionStyle
  vtkIOImage)

if (PARAVIEW_USE_MPI)
  # FIXME: not sure this is the right fix, w.o. this I am getting some missing
  # ...AutoInit build errors.
  target_link_libraries(CoProcessingCompareImagesTester vtkIOMPIImage)
endif()


add_test(NAME CoProcessingTestPythonScript
         COMMAND ${CMAKE_COMMAND}
          -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
          -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_DIR}
          -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
          -DCOPROCESSING_DATA_DIR:PATH=${PARAVIEW_DATA_ROOT}
          -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTest.py
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)

set(CP_LABELS PARAVIEW CATALYST)
set_tests_properties(CoProcessingTestPythonScript PROPERTIES LABELS "${CP_LABELS}")

if (PARAVIEW_USE_MPI)
  add_test(NAME PCoProcessingTestPythonScript
          COMMAND ${CMAKE_COMMAND}
          -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CoProcessingPythonScriptExample>
          -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_DIR}
          -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
          -DCOPROCESSING_DATA_DIR:PATH=${PARAVIEW_DATA_ROOT}
          -DCOPROCESSING_TEST_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/PythonScriptTest.py
          -DUSE_MPI:BOOL=TRUE
          -DMPIEXEC:FILEPATH=${MPIEXEC}
          -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
          -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
          -DVTK_MPI_POSTFLAGS:STRING=${VTK_MPI_POSTFLAGS}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingTestPythonScript.cmake)
  # both the parallel and the serial tests create the same output images
  # so we run this one serially so that they don't interfere.
  set_tests_properties(PCoProcessingTestPythonScript PROPERTIES RUN_SERIAL ON)
  set_tests_properties(PCoProcessingTestPythonScript PROPERTIES LABELS "${CP_LABELS}")
endif()

if (PARAVIEW_BUILD_QT_GUI)
  add_test(NAME CoProcessingFullWorkflow
           COMMAND ${CMAKE_COMMAND}
           -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:paraview>
           -DCOPROCESSING_TEST_DIR:PATH=${PARAVIEW_TEST_DIR}
           -DPARAVIEW_TEST_XML:FILEPATH=${ParaView_SOURCE_DIR}/Plugins/CoProcessingScriptGenerator/Testing/CoProcessingGUI.xml
           -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:pvpython>
           -DCOPROCESSING_DRIVER_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/waveletdriver.py
           -DCOPROCESSING_IMAGE_TESTER:FILEPATH=$<TARGET_FILE:CoProcessingCompareImagesTester>
           -DCOPROCESSING_DATA_DIR:PATH=${PARAVIEW_DATA_ROOT}
           -DCOPROCESSING_OUTPUTCHECK_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/outputcheck.py
           -P ${CMAKE_CURRENT_SOURCE_DIR}/CoProcessingFullWorkflowTest.cmake)
  set_tests_properties(CoProcessingFullWorkflow PROPERTIES LABELS "${CP_LABELS}")
endif()
