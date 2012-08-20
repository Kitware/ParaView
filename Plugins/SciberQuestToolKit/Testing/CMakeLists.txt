
# NOTES:
# on windows you need to run "ctest -C Debug"

include_directories(
  ${ParaView_SOURCE_DIR}/VTK/Common/Testing/Cxx/
  ${ParaView_SOURCE_DIR}/VTK/Rendering/Testing/Cxx/
  )

# get the test name and exec name
macro(sq_exec_name _name _test_name _exec_name)
  set(${_test_name} SciberQuestToolKit-${_name})
  set(CXX_TEST_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  if (WIN32)
    set(${_exec_name} "${CXX_TEST_PATH}/\${CTEST_CONFIGURATION_TYPE}/${${_test_name}}")
  else()
    set(${_exec_name} "${CXX_TEST_PATH}/${${_test_name}}")
  endif()
endmacro()

# add an exec
macro(sq_add_exec _name)
  set(TEST_NAME)
  set(TEST_EXEC)
  sq_exec_name(${_name} TEST_NAME TEST_EXEC)
  add_executable(${TEST_NAME} ${_name}.cpp TestUtils.cxx)
  if (PARAVIEW_USE_MPI AND (NOT WIN32))
    add_definitions(-DPARAVIEW_USE_MPI)
    target_link_libraries(${TEST_NAME} SciberQuestToolKit ${MPI_LIBRARIES})
  else()
    target_link_libraries(${TEST_NAME} SciberQuestToolKit)
  endif()
endmacro()

# configure a test
macro(sq_add_test _name _np)
  set(TEST_NAME)
  set(TEST_EXEC)
  sq_exec_name(${_name} TEST_NAME TEST_EXEC)
  set(DATA_PATH "${PARAVIEW_DATA_ROOT}")
  set(TEMP_PATH "${ParaView_BINARY_DIR}/Testing/Temporary")
  set(BASE_IMAGE "Baseline/SciberQuestToolKit-${_name}")
  if(${_np} LESS 1)
    add_test(
      ${TEST_NAME}-Serial
      ${TEST_EXEC}
      -D ${DATA_PATH}
      -T ${TEMP_PATH}
      -V ${BASE_IMAGE})
  else()
    add_test(
      ${TEST_NAME}-Parallel
      ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} ${_np} ${VTK_MPI_PREFLAGS}
      ${TEST_EXEC}
      -D ${DATA_PATH}
      -T ${TEMP_PATH}
      -V ${BASE_IMAGE})
  endif()
endmacro()

sq_add_exec(TestBOVIO)
sq_add_exec(TestKernelConvolution)
sq_add_exec(TestVortexFilter)
sq_add_exec(TestFieldTopologyMapper)
sq_add_exec(TestPoincareMapper)
sq_add_exec(TestFTLE)

if (PARAVIEW_DATA_ROOT)
  if (PARAVIEW_USE_MPI AND (NOT WIN32))
    # parallel invocations, these could run with any number of
    # processes.
    sq_add_test(TestBOVIO 4)
    sq_add_test(TestKernelConvolution 4)
    sq_add_test(TestVortexFilter 4)
    sq_add_test(TestFieldTopologyMapper 4)
    sq_add_test(TestPoincareMapper 4)
    sq_add_test(TestFTLE 4)
  else()
    # serial invocations, our reader needs mpi these tests use
    # an alternate reader when invoked serially.
    sq_add_test(TestKernelConvolution 0)
    sq_add_test(TestVortexFilter 0)
  endif()
endif()
