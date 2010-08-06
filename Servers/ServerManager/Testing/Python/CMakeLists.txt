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
  
# In CMake version 2.8 we can used a much nicer $<CONFIGURATION> arguments for specifying 
# the configuration type of the build For now I have added this new functionality to these 
# batch tests but it will only work on dashboards using 2.8. This check can be removed when
# ParaView officially requires CMake 2.8."
SET(GENERATOR_EXPRESSIONS_SUPPORTED)
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.8)
  SET(GENERATOR_EXPRESSIONS_SUPPORTED OFF)
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.8)
  SET(GENERATOR_EXPRESSIONS_SUPPORTED ON)
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.8)

SET(PVPYTHON_COMMAND ${EXECUTABLE_OUTPUT_PATH}/pvpython)
SET(PVBATCH_COMMAND ${EXECUTABLE_OUTPUT_PATH}/pvbatch)

# We don't want the <CONFIGURATION> as part of the path on non windows systems.
IF(WIN32 AND GENERATOR_EXPRESSIONS_SUPPORTED)
  SET(PVPYTHON_COMMAND ${EXECUTABLE_OUTPUT_PATH}/\$<CONFIGURATION>/pvpython)
  SET(PVBATCH_COMMAND ${EXECUTABLE_OUTPUT_PATH}/\$<CONFIGURATION>/pvbatch)
ENDIF(WIN32 AND GENERATOR_EXPRESSIONS_SUPPORTED)


IF (PVServerManagerTestData AND GENERATOR_EXPRESSIONS_SUPPORTED)
  FOREACH (tfile ${SMSTATE_FILES})
    ADD_TEST(NAME "${tfile}-ServerManagerPython"
      COMMAND ${PVPYTHON_COMMAND}
        ${CMAKE_CURRENT_SOURCE_DIR}/SMPythonTesting.py
        -D ${PVServerManagerTestData}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -V ${PVServerManagerTestData}/Baseline/${tfile}.png
        --state ${SMSTATE_FILE_DIR}/${tfile}.pvsm)
  ENDFOREACH (tfile)

  # This is a test that saves a temporary SM state and uses that saved
  # state for regression testing.
  ADD_TEST(NAME "LoadSave-ServerManagerPython"
      COMMAND ${PVPYTHON_COMMAND}
        ${CMAKE_CURRENT_SOURCE_DIR}/SMPythonTesting.py
        -D ${PVServerManagerTestData}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -V ${PVServerManagerTestData}/Baseline/Arrow.png
        --state ${SMSTATE_FILE_DIR}/Arrow.pvsm
        --use_saved_state)
ENDIF (PVServerManagerTestData AND GENERATOR_EXPRESSIONS_SUPPORTED)

###############################################################################
# For python scripts for testing.
#

# Add python script names here.
SET (PY_TESTS
  Animation
  ProxyPropertyLinks
  PythonPVSimpleCone
  PythonPVSimpleExII
  PythonPVSimpleSphere
  PythonSMTraceTest1
  PythonSMTraceTest2
#  SMUndoRedo
#  SMCompoundProxyUndoRedo
  )


IF (PVServerManagerTestData AND GENERATOR_EXPRESSIONS_SUPPORTED)
  FOREACH (tfile ${PY_TESTS})
    ADD_TEST(NAME "${tfile}-ServerManagerPython"
      COMMAND ${PVPYTHON_COMMAND}
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
        -D ${PVServerManagerTestData}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -V ${PVServerManagerTestData}/Baseline/${tfile}.png
        -S ${SMSTATE_FILE_DIR})
  ENDFOREACH(tfile)
ENDIF (PVServerManagerTestData AND GENERATOR_EXPRESSIONS_SUPPORTED)


SET(PY_TESTS_NO_BASELINE
  CellIntegrator
  CSVWriterReader
  IntegrateAttributes
  ProgrammableFilter
  ProxyManager
  SMPropertyAdaptor
  VRMLSource
)

include("TestNumPy.cmake")

IF ("1" STREQUAL ${HAS_NUMPY})
  SET(PY_TESTS_NO_BASELINE ${PY_TESTS_NO_BASELINE} PythonFilters)
ENDIF ("1" STREQUAL ${HAS_NUMPY})

IF (PVServerManagerTestData AND GENERATOR_EXPRESSIONS_SUPPORTED)
  FOREACH (tfile ${PY_TESTS_NO_BASELINE})
    ADD_TEST(NAME "${tfile}-ServerManagerPython"
      COMMAND ${PVPYTHON_COMMAND}
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
        -D ${PVServerManagerTestData}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -S ${SMSTATE_FILE_DIR})
  ENDFOREACH(tfile)

  # This is necessary because PythonFilters generates and executes python scripts
  # on the fly which throws exceptions but cannot be caught by the host script
  # for some reason. Therefore, those exceptions are not considered as failures
  # by the ctest framework. Set this property causes ctest to parse the outputs of
  # the generated scripts for the regex "Error" which can cause a test failure.
  IF ("1" STREQUAL ${HAS_NUMPY})
    SET_TESTS_PROPERTIES(PythonFilters-ServerManagerPython PROPERTIES FAIL_REGULAR_EXPRESSION "Error")
  ENDIF ("1" STREQUAL ${HAS_NUMPY})
ENDIF (PVServerManagerTestData AND GENERATOR_EXPRESSIONS_SUPPORTED)
###############################################################################
# Add tests for pvbatch.

SET (PVBATCH_TESTS
  Simple
  ParallelSerialWriter
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

IF (PVServerManagerTestData AND GENERATOR_EXPRESSIONS_SUPPORTED)
  FOREACH (tfile ${PVBATCH_TESTS})
    IF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)

      ADD_TEST(NAME ${tfile}-Batch
        COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
          ${PVBATCH_COMMAND}
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PVServerManagerTestData}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PVServerManagerTestData}/Baseline/${tfile}.png
          -S ${SMSTATE_FILE_DIR})

      ADD_TEST(NAME ${tfile}-SymmetricBatch
        COMMAND ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
          ${PVBATCH_COMMAND}
          --symmetric
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PVServerManagerTestData}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PVServerManagerTestData}/Baseline/${tfile}.png
          -S ${SMSTATE_FILE_DIR})
    ELSE (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
      ADD_TEST(NAME ${tfile}-Batch
        COMMAND ${PVBATCH_COMMAND}
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PVServerManagerTestData}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PVServerManagerTestData}/Baseline/${tfile}.png
          -S ${SMSTATE_FILE_DIR})

      ADD_TEST(NAME ${tfile}-SymmetricBatch
        COMMAND ${PVBATCH_COMMAND}
          --symmetric
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${PVServerManagerTestData}
          -T ${ParaView_BINARY_DIR}/Testing/Temporary
          -V ${PVServerManagerTestData}/Baseline/${tfile}.png
          -S ${SMSTATE_FILE_DIR})
    ENDIF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
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
ENDIF (PVServerManagerTestData AND GENERATOR_EXPRESSIONS_SUPPORTED)

