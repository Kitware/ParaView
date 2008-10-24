# Requires that PVServerManagerTestData is set
# for any of the tests to be added.

###############################################################################
# For python test that simply load the SM state.
#
# To load state files in python and perform regression testing
# add them here.
SET (SMSTATE_FILES
  ${PYTHON_SMSTATE_FILES}
  )

IF (PVServerManagerTestData)
  FOREACH (tfile ${SMSTATE_FILES})
    ADD_TEST("${tfile}-ServerManagerPython"
      ${EXECUTABLE_OUTPUT_PATH}/pvpython
      ${CMAKE_CURRENT_SOURCE_DIR}/SMPythonTesting.py
      -D ${PVServerManagerTestData}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVServerManagerTestData}/Baseline/${tfile}.png
      --state ${SMSTATE_FILE_DIR}/${tfile}.pvsm)
  ENDFOREACH (tfile)

  # This is a test that saves a temporary SM state and uses that saved
  # state for regression testing.
  ADD_TEST("LoadSave-ServerManagerPython"
      ${EXECUTABLE_OUTPUT_PATH}/pvpython
      ${CMAKE_CURRENT_SOURCE_DIR}/SMPythonTesting.py
      -D ${PVServerManagerTestData}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVServerManagerTestData}/Baseline/Arrow.png
      --state ${SMSTATE_FILE_DIR}/Arrow.pvsm
      --use_saved_state)
ENDIF (PVServerManagerTestData)
  
###############################################################################
# For python scripts for testing.
#

# Add python script names here.
SET (PY_TESTS
  SMAnimation
  ProxyPropertyLinks
#  SMUndoRedo
#  SMCompoundProxyUndoRedo
  )


IF (PVServerManagerTestData)
  FOREACH (tfile ${PY_TESTS})
    ADD_TEST("${tfile}-ServerManagerPython"
      ${EXECUTABLE_OUTPUT_PATH}/pvpython
      ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
      -D ${PVServerManagerTestData}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVServerManagerTestData}/Baseline/${tfile}.png
      -S ${SMSTATE_FILE_DIR})
  ENDFOREACH(tfile)
ENDIF (PVServerManagerTestData)


SET(PY_TESTS_NO_BASELINE
  CellIntegrator
  CSVWriterReader
  IntegrateAttributes
  ProgrammableFilter
  ProxyManager
  SMPropertyAdaptor
  VRMLSource
)

IF (PVServerManagerTestData)
  FOREACH (tfile ${PY_TESTS_NO_BASELINE})
    ADD_TEST("${tfile}-ServerManagerPython"
      ${EXECUTABLE_OUTPUT_PATH}/pvpython
      ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
      -D ${PVServerManagerTestData}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -S ${SMSTATE_FILE_DIR})
  ENDFOREACH(tfile)
ENDIF (PVServerManagerTestData)
###############################################################################
# Add tests for pvbatch.

SET (PVBATCH_TESTS
  Simple
  ParallelSerialWriter
)

FOREACH (tfile ${PVBATCH_TESTS})
  IF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
    ADD_TEST(${tfile}-Batch
      ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
      ${EXECUTABLE_OUTPUT_PATH}/pvbatch
      ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
      -D ${PVServerManagerTestData}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVServerManagerTestData}/Baseline/${tfile}.png
      -S ${SMSTATE_FILE_DIR})

    ADD_TEST(${tfile}-SynchronousBatch
      ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
      ${EXECUTABLE_OUTPUT_PATH}/pvbatch
      --synchronous
      ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
      -D ${PVServerManagerTestData}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVServerManagerTestData}/Baseline/${tfile}.png
      -S ${SMSTATE_FILE_DIR})
  ELSE (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
    ADD_TEST(${tfile}-Batch
      ${EXECUTABLE_OUTPUT_PATH}/pvbatch
      ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
      -D ${PVServerManagerTestData}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVServerManagerTestData}/Baseline/${tfile}.png
      -S ${SMSTATE_FILE_DIR})

    ADD_TEST(${tfile}-SynchronousBatch
      ${EXECUTABLE_OUTPUT_PATH}/pvbatch
      --synchronous
      ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
      -D ${PVServerManagerTestData}
      -T ${ParaView_BINARY_DIR}/Testing/Temporary
      -V ${PVServerManagerTestData}/Baseline/${tfile}.png
      -S ${SMSTATE_FILE_DIR})
  ENDIF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
ENDFOREACH (tfile)

