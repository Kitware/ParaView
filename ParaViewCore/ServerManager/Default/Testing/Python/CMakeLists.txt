paraview_test_load_data(""
  "blow data.myvtk"
  blow.vtk
  bot2.wrl
  can.ex2
  disk_out_ref.ex2
  dualSphereAnimation4.pvd
  elements.vtu
  "iron protein.vtk"
  multicomb_0.vts
  poisson_3dall_2.xmf
  PythonProgrammableFilterParameters.xml
  quadraticTetra01.vtu
  sonic.pht
  SPCTH/ball_and_box.spcth
  TestRepresentationTypePlugin.xml
  )
paraview_test_load_data_dirs(""
  dualSphereAnimation)

###############################################################################
# For python scripts for testing.
#
# Add python script names here.
set(PY_TESTS
  AnimationCache.py,NO_VALID
  Animation.py
  AxesGridTestGridLines.py
  CellIntegrator.py,NO_VALID
  ColorAttributeTypeBackwardsCompatibility.py,NO_VALID
  CSVWriterReader.py,NO_VALID
  GhostCellsInMergeBlocks.py
  IntegrateAttributes.py,NO_VALID
  MultiServer.py,NO_VALID
  ProgrammableFilterProperties.py,NO_VALID
  ProgrammableFilter.py,NO_VALID
  ProxyBackwardsCompatibilityTest.py,NO_VALID
  ProxyManager.py,NO_VALID
  ProxyPropertyLinks.py
  PythonAnimationTrack.py
  PythonProgrammableFilterParameters.py,NO_VALID
  PythonPVSimpleCone.py
  PythonPVSimpleExII.py
  PythonPVSimpleSphere.py
  PythonSMTraceTest1.py
  PythonSMTraceTest2.py,NO_VALID
  PythonTestBenchmark.py,NO_VALID
  ReaderReload.py,NO_VALID
  RepresentationTypeHint.py,NO_VALID
  SaveAnimation.py
  SaveScreenshot.py,NO_VALID
  ScalarBarActorBackwardsCompatibility.py,NO_VALID
  ValidateSources.py,NO_VALID
  VRMLSource.py,NO_VALID
  )

include(FindPythonModules)
find_python_module(numpy numpy_found)
if (numpy_found)
  list(APPEND PY_TESTS PythonSelection.py)
endif ()

if (BUILD_SHARED_LIBS
  AND PARAVIEW_BUILD_PLUGIN_SurfaceLIC
  AND PARAVIEW_BUILD_PLUGIN_SLACTools)
  list(APPEND PY_TESTS
    Plugins.py,NO_VALID)
endif ()

# This test makes use of "Point Gaussian" representation, only available with OpenGL2
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
    list(APPEND PY_TESTS PointGaussianProperties.py)
endif()

set(SMSTATE_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(${vtk-module}_ARGS
  -S "${SMSTATE_FILE_DIR}")
paraview_add_test_python(
  JUST_VALID
  ${PY_TESTS}
  )

###############################################################################
# Add tests for pvbatch.

set(PVBATCH_TESTS
  AnnotationVisibility.py
  LinePlotInScripts.py,NO_VALID
  MultiView.py
  ParallelImageWriter.py,NO_VALID
  ParallelSerialWriter.py
  SaveScreenshot.py,NO_VALID
  Simple.py
  UserTransformOnRepresentation.py
  )

# Saving animation currently doesn't work in symmetric mode.
# paraview/paraview#17329
set(PVBATCH_NO_SYMMETRIC_TESTS
  SaveAnimation.py
  )
IF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
  set(${vtk-module}_NUMPROCS 2)
  paraview_add_test_pvbatch_mpi(
    JUST_VALID
    ${PVBATCH_TESTS}
    ${PVBATCH_NO_SYMMETRIC_TESTS}
    )
  set(PARAVIEW_PVBATCH_ARGS
    --symmetric)
  set(vtk_test_prefix Symmetric)
  paraview_add_test_pvbatch_mpi(
    JUST_VALID
    ${PVBATCH_TESTS}
    )
  set(PARAVIEW_PVBATCH_ARGS)
  set(vtk_test_prefix)
  set(${vtk-module}_NUMPROCS)
else ()
  paraview_add_test_pvbatch(
    JUST_VALID
    ${PVBATCH_TESTS}
    ${PVBATCH_NO_SYMMETRIC_TESTS}
    )
  set(PARAVIEW_PVBATCH_ARGS
    --symmetric)
  set(vtk_test_prefix Symmetric)
  paraview_add_test_pvbatch(
    JUST_VALID
    ${PVBATCH_TESTS}
    )
  set(PARAVIEW_PVBATCH_ARGS)
  set(vtk_test_prefix)
endif ()

if (numpy_found)
  # This is necessary because PythonFilters generates and executes python scripts
  # on the fly which throws exceptions but cannot be caught by the host script
  # for some reason. Therefore, those exceptions are not considered as failures
  # by the ctest framework. Set this property causes ctest to parse the outputs of
  # the generated scripts for the regex "Error" which can cause a test failure.

  # The following lines are commented out to make sure PythonFilters-Batch does
  # not trash the DashBoard because it always fails. The solution, however, is
  # out of my control. When the related problem is fixed, please uncomment this
  # line.
  # set_tests_properties(${vtk-module}Python-MPI-PythonFilters PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  # set_tests_properties(${vtk-module}Python-MPI-SymmetricPythonFilters PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  # set_tests_properties(${vtk-module}Python-PythonFilters PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  # set_tests_properties(${vtk-module}Python-SymmetricPythonFilters PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
endif()

if (PARAVIEW_USE_MPI AND VTK_MPIRUN_EXE AND NOT WIN32)
  set(PARAVIEW_PVBATCH_ARGS
    --symmetric)
  paraview_add_test_pvbatch_mpi(
    NO_DATA NO_OUTPUT NO_VALID
    TestCleanArrays.py
    TestMPI4PY.py
    ParallelPythonImport.py
    )
  set(PARAVIEW_PVBATCH_ARGS)
endif()

# Python state tests. Each test executes an XML test in the ParaView UI, saves
# the state as a Python state file, runs the Python state file script in
# pvpython, then checks that the same image is generated in both the UI and
# pvpython. Each <TEST> list here requires a <TEST>.xml that generates the
# trace and a <TEST>.png file for comparison to a baseline.
set(paraview_python_state_tests
  SavePythonState
  ScalarOpacityFunctionPythonState
)
foreach(python_state_test ${paraview_python_state_tests})
  if (PARAVIEW_BUILD_QT_GUI)
    if(BUILD_SHARED_LIBS)
      add_test(NAME ${python_state_test}
        COMMAND ${CMAKE_COMMAND}
        -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:paraview>
        -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:pvpython>
        -DPARAVIEW_TEST_OUTPUT_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
        -DPARAVIEW_TEST_OUTPUT_BASELINE_DIR:PATH=${PARAVIEW_TEST_OUTPUT_BASELINE_DIR}
        -DTEST_DRIVER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/PythonStateTestDriver.py
        -DTEST_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/${python_state_test}.xml
        -DTEMPORARY_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
        -DPYTHON_STATE_TEST_NAME:STRING=${python_state_test}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/PythonStateTestDriver.cmake)
      set_tests_properties(${python_state_test} PROPERTIES LABELS "PARAVIEW")
    endif()
  endif()
endforeach()
