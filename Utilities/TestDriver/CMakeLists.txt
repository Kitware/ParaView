# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(PV_SETUP_SCRIPT ""
  CACHE STRING "The script to use for setup when testing client-server configurations.")
set(PV_SSH_FLAGS ""
  CACHE STRING "Flags for using SSH when testing client-server configurations.")
option(PV_TEST_USE_RANDOM_PORTS
  "Use random port numbers when testing client-server configurations." ON)
set(PV_TEST_INIT_COMMAND ""
  CACHE STRING "Command to run before a test begins. Multiple commands are separated by ';'.")
set(PV_TEST_CLEAN_COMMAND ""
  CACHE STRING "Command to run after a failed test to cleanup processes.  Example: \"killall -9 rsh paraview\"")
set(PV_TEST_CLIENT "localhost"
  CACHE STRING "Node which serves as the client node, used to connect from the server side in reverse connection mode.")
mark_as_advanced(
  PV_SETUP_SCRIPT
  PV_SSH_FLAGS
  PV_TEST_CLEAN_COMMAND
  PV_TEST_CLIENT
  PV_TEST_INIT_COMMAND
  PV_TEST_USE_RANDOM_PORTS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkSMTestDriverConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTestDriverConfig.h"
  ESCAPE_QUOTES
  @ONLY)

vtk_module_add_executable(ParaView::smTestDriver
  DEVELOPMENT
  vtkSMTestDriver.cxx
  vtkSMTestDriver.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTestDriverConfig.h")

set(mpi_defines)
if (PARAVIEW_USE_MPI)
  find_package(MPI QUIET REQUIRED COMPONENTS C)

  list(APPEND mpi_defines
    PARAVIEW_USE_MPI
    "PARAVIEW_MPIEXEC_EXECUTABLE=\"${MPIEXEC_EXECUTABLE}\""
    "PARAVIEW_MPI_NUMPROC_FLAG=\"${MPIEXEC_NUMPROC_FLAG}\""
    # FIXME: This is currently hard-coded to 2 since the maximum isn't
    # sensible. However, there is no command line flag to override the number
    # of processes to use for a given test, so instead, just use 2 for now.
    "PARAVIEW_MPI_MAX_NUMPROCS=2") # See ParaViewTesting.cmake@@paraview_add_test_mpi
    #"PARAVIEW_MPI_MAX_NUMPROCS=${MPIEXEC_MAX_NUMPROCS}")

  if (MPI_PREFLAGS)
    list(APPEND mpi_defines
      "PARAVIEW_MPI_PREFLAGS=\"${MPI_PREFLAGS}\"")
  endif ()
  if (MPI_POSTFLAGS)
    list(APPEND mpi_defines
      "PARAVIEW_MPI_POSTFLAGS=\"${MPI_POSTFLAGS}\"")
  endif ()
endif ()

if (mpi_defines)
  vtk_module_definitions(ParaView::smTestDriver
    PRIVATE
      ${mpi_defines})
endif ()
