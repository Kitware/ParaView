cmake_minimum_required(VERSION 2.8.8)
project(CatalystMPISubCommunicatorExample)

find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")
if(NOT PARAVIEW_USE_MPI)
  message(SEND_ERROR "ParaView must be built with MPI enabled")
endif()

add_executable(MPISubCommunicatorExample FEDriver.cxx FEDataStructures.cxx FEAdaptor.cxx)
target_link_libraries(MPISubCommunicatorExample LINK_PRIVATE vtkPVPythonCatalyst vtkParallelMPI)
include(vtkMPI)
vtk_mpi_link(MPISubCommunicatorExample)

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(MPISubCommunicatorExampleTest MPISubCommunicatorExample
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
