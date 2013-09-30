###############################################################################
# For python test that simply load the SM state.
#
# To load state files in python and perform regression testing
# add them here.
set (SMSTATE_FILES
  Arrow
  Axes
  CalcInput
  ExtractGrid
  TestPHT
  SPCTH2)
set (SMSTATE_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../SMStates")

IF (PARAVIEW_DATA_ROOT)
  FOREACH (tfile ${SMSTATE_FILES})
    ADD_TEST(NAME "${tfile}-ServerManagerPython"
      COMMAND $<TARGET_FILE:pvpython>
        --enable-bt
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages/paraview/smtesting.py"
        -D ${PARAVIEW_DATA_ROOT}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -V ${PARAVIEW_DATA_ROOT}/Baseline/${tfile}.png
        --state ${CMAKE_CURRENT_SOURCE_DIR}/../SMStates/${tfile}.pvsm
        )
    set_tests_properties(${tfile}-ServerManagerPython PROPERTIES LABELS "PARAVIEW")
  ENDFOREACH (tfile)

  # This is a test that saves a temporary SM state and uses that saved
  # state for regression testing.
  ADD_TEST(NAME "LoadSave-ServerManagerPython"
      COMMAND $<TARGET_FILE:pvpython>
        --enable-bt
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages/paraview/smtesting.py"
        -D ${PARAVIEW_DATA_ROOT}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -V ${PARAVIEW_DATA_ROOT}/Baseline/Arrow.png
        --state ${CMAKE_CURRENT_SOURCE_DIR}/../SMStates/Arrow.pvsm
        --use_saved_state
        )
  set_tests_properties(LoadSave-ServerManagerPython PROPERTIES LABELS "PARAVIEW")
ENDIF ()

###############################################################################
# For python scripts for testing.
#
include (TestNumPy)

# Add python script names here.
SET (PY_TESTS
  Animation
  ProxyPropertyLinks
  PythonPVLookupTables
  PythonPVSimpleCone
  PythonPVSimpleExII
  PythonPVSimpleSphere
  PythonSMTraceTest1
  PythonSMTraceTest2
  PythonAnimationTrack
  GhostCellsInMergeBlocks
#  Python2DRendering =======> REAL Scalar Bar Rendering bug
#  SMUndoRedo
#  SMCompoundProxyUndoRedo
  )
if (FOUND_NUMPY)
  list(APPEND PY_TESTS PythonSelection)
endif()

IF (PARAVIEW_DATA_ROOT)
  FOREACH (tfile ${PY_TESTS})
    ADD_TEST(NAME "${tfile}-ServerManagerPython"
      COMMAND $<TARGET_FILE:pvpython>
        --enable-bt
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
        -D ${PARAVIEW_DATA_ROOT}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -V ${PARAVIEW_DATA_ROOT}/Baseline/${tfile}.png
        -S ${SMSTATE_FILE_DIR}
        )
    set_tests_properties(${tfile}-ServerManagerPython PROPERTIES LABELS "PARAVIEW")
  ENDFOREACH(tfile)
ENDIF ()


SET(PY_TESTS_NO_BASELINE
  CellIntegrator
  CSVWriterReader
  IntegrateAttributes
  ProgrammableFilter
  ProgrammableFilterProperties
  ProxyManager
  VRMLSource
  MultiServer
  ValidateSources
)

if(BUILD_SHARED_LIBS
  AND PARAVIEW_BUILD_PLUGIN_SurfaceLIC
  AND PARAVIEW_BUILD_PLUGIN_SLACTools)
  list(APPEND PY_TESTS_NO_BASELINE Plugins)
endif()

include("TestNumPy")

IF ("1" STREQUAL ${HAS_NUMPY})
  SET(PY_TESTS_NO_BASELINE ${PY_TESTS_NO_BASELINE} PythonFilters)
ENDIF ("1" STREQUAL ${HAS_NUMPY})

IF (PARAVIEW_DATA_ROOT)
  FOREACH (tfile ${PY_TESTS_NO_BASELINE})
    ADD_TEST(NAME "${tfile}-ServerManagerPython"
      COMMAND $<TARGET_FILE:pvpython>
        --enable-bt
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
        -D ${PARAVIEW_DATA_ROOT}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -S ${SMSTATE_FILE_DIR}
        )
    set_tests_properties(${tfile}-ServerManagerPython PROPERTIES LABELS "PARAVIEW")
  ENDFOREACH(tfile)

  # This is necessary because PythonFilters generates and executes python scripts
  # on the fly which throws exceptions but cannot be caught by the host script
  # for some reason. Therefore, those exceptions are not considered as failures
  # by the ctest framework. Set this property causes ctest to parse the outputs of
  # the generated scripts for the regex "Error" which can cause a test failure.
  IF ("1" STREQUAL ${HAS_NUMPY})
    SET_TESTS_PROPERTIES(PythonFilters-ServerManagerPython PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  ENDIF ("1" STREQUAL ${HAS_NUMPY})
ENDIF ()
###############################################################################
# Add tests for pvbatch.

SET (PVBATCH_TESTS
  MultiView
  ParallelImageWriter
  ParallelSerialWriter
  Simple
  UserTransformOnRepresentation
)

IF ("1" STREQUAL ${HAS_NUMPY})
  # The following line is commented out to make sure PythonFilters-Batch does
  # not trash the DashBoard because it always fails. The solution, however, is
  # out of my control. When the related problem is fixed, please uncomment this
  # line to enable the Batch tests. The Batch tests are important because the
  # global_* functions can only be correctly tested in Batch mode. Without the
  # environment, they behave just like their un-global counterparts.
  # SET(PVBATCH_TESTS ${PVBATCH_TESTS} PythonFilters)
ENDIF ("1" STREQUAL ${HAS_NUMPY})

IF (PARAVIEW_DATA_ROOT)
  FOREACH (tfile ${PVBATCH_TESTS})
    IF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)

      ADD_TEST(NAME ${tfile}-Batch
        COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
          $<TARGET_FILE:pvbatch>
          --enable-bt
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline/${tfile}.png
          -S ${SMSTATE_FILE_DIR}
          )

      ADD_TEST(NAME ${tfile}-SymmetricBatch
        COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
          $<TARGET_FILE:pvbatch>
          --symmetric
          --enable-bt
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline/${tfile}.png
          -S ${SMSTATE_FILE_DIR})
    ELSE (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
      ADD_TEST(NAME ${tfile}-Batch
        COMMAND $<TARGET_FILE:pvbatch>
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline/${tfile}.png
          -S ${SMSTATE_FILE_DIR})

      ADD_TEST(NAME ${tfile}-SymmetricBatch
        COMMAND $<TARGET_FILE:pvbatch>
          --enable-bt
          --symmetric
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PARAVIEW_DATA_ROOT}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PARAVIEW_DATA_ROOT}/Baseline/${tfile}.png
          -S ${SMSTATE_FILE_DIR})
    ENDIF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
    set_tests_properties(
      ${tfile}-Batch
      ${tfile}-SymmetricBatch
      PROPERTIES LABELS "PARAVIEW")
  ENDFOREACH (tfile)

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
    # SET_TESTS_PROPERTIES(PythonFilters-Batch PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
    # SET_TESTS_PROPERTIES(PythonFilters-SymmetricBatch PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  ENDIF ("1" STREQUAL ${HAS_NUMPY})
ENDIF ()

if (PARAVIEW_USE_MPI AND VTK_MPIRUN_EXE AND NOT WIN32)
  add_test(NAME TestMPI4PY-SymmetricBatch
          COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS}
                  ${VTK_MPI_NUMPROC_FLAG} ${VTK_MPI_MAX_NUMPROCS} ${VTK_MPI_PREFLAGS}
                  $<TARGET_FILE:pvbatch>
                  --enable-bt
                  --symmetric
                  ${CMAKE_CURRENT_SOURCE_DIR}/TestMPI4PY.py)
    set_tests_properties(TestMPI4PY-SymmetricBatch PROPERTIES LABELS "PARAVIEW")
endif()

# SavePythonState test
if (PARAVIEW_BUILD_QT_GUI)
  if(BUILD_SHARED_LIBS)
    add_test(NAME SavePythonState
      COMMAND ${CMAKE_COMMAND}
      -DPARAVIEW_EXECUTABLE:FILEPATH=$<TARGET_FILE:paraview>
      -DPVPYTHON_EXECUTABLE:FILEPATH=$<TARGET_FILE:pvpython>
      -DPARAVIEW_DATA_ROOT:PATH=${PARAVIEW_DATA_ROOT}
      -DTEST_DRIVER:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/SavePythonState.py
      -DTEST_SCRIPT:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/SavePythonState.xml
      -P ${CMAKE_CURRENT_SOURCE_DIR}/SavePythonState.cmake)
    set_tests_properties(SavePythonState PROPERTIES LABELS "PARAVIEW")
  endif()
endif()
