#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================

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
    "PARAVIEW_MPI_MAX_NUMPROCS=2")
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
