paraview_test_load_data(""
  "blow data.myvtk"
  "iron protein.vtk"
  SPCTH/ball_and_box.spcth
  blow.vtk
  bot2.wrl
  can.ex2
  disk_out_ref.ex2
  elements.vtu
  multicomb_0.vts
  poisson_3dall_2.xmf
  quadraticTetra01.vtu
  sonic.pht
  )

###############################################################################
# For python scripts for testing.
#
include (TestNumPy)

# Add python script names here.
set(PY_TESTS
  Animation.py
  ProxyPropertyLinks.py
  PythonPVLookupTables.py
  PythonPVSimpleCone.py
  PythonPVSimpleExII.py
  PythonPVSimpleSphere.py
  PythonSMTraceTest1.py
  PythonSMTraceTest2.py,NO_VALID
  PythonAnimationTrack.py
  GhostCellsInMergeBlocks.py
  CellIntegrator.py,NO_VALID
  CSVWriterReader.py,NO_VALID
  IntegrateAttributes.py,NO_VALID
  ProgrammableFilter.py,NO_VALID
  ProgrammableFilterProperties.py,NO_VALID
  ProxyManager.py,NO_VALID
  VRMLSource.py,NO_VALID
  MultiServer.py,NO_VALID
  ValidateSources.py,NO_VALID
  ColorAttributeTypeBackwardsCompatibility.py,NO_VALID
  )
if (FOUND_NUMPY)
  list(APPEND PY_TESTS
    PythonSelection.py)
endif ()
if (BUILD_SHARED_LIBS
  AND PARAVIEW_BUILD_PLUGIN_SurfaceLIC
  AND PARAVIEW_BUILD_PLUGIN_SLACTools)
  list(APPEND PY_TESTS
    Plugins.py,NO_VALID)
endif ()

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
  MultiView.py
  ParallelImageWriter.py,NO_VALID
  ParallelSerialWriter.py
  Simple.py
  UserTransformOnRepresentation.py
  LinePlotInScripts.py,NO_VALID
  )

IF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
  set(${vtk-module}_NUMPROCS 2)
  paraview_add_test_pvbatch_mpi(
    JUST_VALID
    ${PVBATCH_TESTS}
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
  paraview_add_test_python(
    JUST_VALID
    ${PVBATCH_TESTS}
    )
  set(PARAVIEW_PVBATCH_ARGS
    --symmetric)
  set(vtk_test_prefix Symmetric)
  paraview_add_test_python(
    JUST_VALID
    ${PVBATCH_TESTS}
    )
  set(PARAVIEW_PVBATCH_ARGS)
  set(vtk_test_prefix)
endif ()

IF ("1" STREQUAL ${HAS_NUMPY})
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
ENDIF ("1" STREQUAL ${HAS_NUMPY})

if (PARAVIEW_USE_MPI AND VTK_MPIRUN_EXE AND NOT WIN32)
  set(PARAVIEW_PVBATCH_ARGS
    --symmetric)
  paraview_add_test_pvbatch_mpi(
    NO_DATA NO_OUTPUT NO_VALID
    TestMPI4PY.py
    )
  set(PARAVIEW_PVBATCH_ARGS)
endif()

# SavePythonState test
if (PARAVIEW_BUILD_QT_GUI)
  if(BUILD_SHARED_LIBS)
    add_test(NAME SavePythonState
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:paraview>
      -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:pvpython>
      -DPARAVIEW_TEST_OUTPUT_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
      -DPARAVIEW_TEST_OUTPUT_BASELINE_DIR:PATH=${PARAVIEW_TEST_OUTPUT_BASELINE_DIR}
      -DTEST_DRIVER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/SavePythonState.py
      -DTEST_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/SavePythonState.xml
      -DTEMPORARY_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/SavePythonState.cmake)
    set_tests_properties(SavePythonState PROPERTIES LABELS "PARAVIEW")
  endif()
endif()

if(PYTHON_EXECUTABLE AND PARAVIEW_USE_MPI AND BUILD_SHARED_LIBS AND UNIX AND NOT APPLE)
  # if we are building with MPI and Python enabled then we should
  # also be building mpi4py and can import that for this test
  add_test(ParallelPythonImport
    ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS}
    ${VTK_MPI_PREFLAGS}
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/ParallelPythonImport.py
    ${VTK_MPI_POSTFLAGS}
    )
  set_tests_properties(ParallelPythonImport PROPERTIES LABELS PARAVIEW
    ENVIRONMENT "PYTHONPATH=${VTK_PYTHONPATH}:${VTK_PYTHONPATH}/site-packages:${PYTHONPATH}" )
endif()
