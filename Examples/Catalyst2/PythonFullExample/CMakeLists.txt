cmake_minimum_required(VERSION 3.21)
project(PythonFullExampleV2)

include (GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

# Since this example uses MPI, find and link against it.
find_package(MPI COMPONENTS C)
if (NOT MPI_FOUND)
  message(STATUS
    "Skipping example: ${PROJECT_NAME} requires MPI.")
  return ()
endif ()

#------------------------------------------------------------------------------
option(USE_CATALYST "Build example with Catalyst enabled" ON)
if (USE_CATALYST)

  find_package(catalyst
    REQUIRED
    PATHS "${ParaView_DIR}/catalyst")
  
  find_package(Python3 REQUIRED)
  include(CTest)
  if (BUILD_TESTING)
    set(python_driver "${CMAKE_CURRENT_SOURCE_DIR}/fedriver.py")
    set(num_ranks 4)
    set(mpi_prefix
        ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${num_ranks})

    # once we update to cmake >= 3.25 we should use --modify PYTHONPATH=path_list_prepend:${CATALYST_PYTHONPATH}
    add_test(
      NAME PythonFullExampleV2::SimplePipeline
      COMMAND  ${CMAKE_COMMAND} -E env "PYTHONPATH=${CATALYST_PYTHONPATH}:$ENV{PYTHONPATH}" "CATALYST_IMPLEMENTATION_PATHS=${ParaView_CATALYST_DIR}"
               ${mpi_prefix}
               ${Python3_EXECUTABLE} ${python_driver} "${CMAKE_CURRENT_SOURCE_DIR}/../SampleScripts/gridwriter.py" "${CMAKE_CURRENT_SOURCE_DIR}/second.py"
    )

  endif()
  # configure a script for running without using ctest
  configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/run.sh
  @ONLY)
  
endif()
