cmake_minimum_required(VERSION 3.3)
project(CatalystMPISubCommunicatorExample)

find_package(ParaView 4.2 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")
if(NOT PARAVIEW_USE_MPI)
  message(SEND_ERROR "ParaView must be built with MPI enabled")
endif()

add_executable(MPISubCommunicatorExample FEDriver.cxx FEDataStructures.cxx FEAdaptor.cxx)
target_link_libraries(MPISubCommunicatorExample LINK_PRIVATE vtkPVPythonCatalyst vtkParallelMPI)
include(vtkModuleMacros)
include(vtkMPI)
vtk_mpi_link(MPISubCommunicatorExample)

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(NAME MPISubCommunicatorExampleTest COMMAND MPISubCommunicatorExample
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
  set_tests_properties(MPISubCommunicatorExampleTest PROPERTIES LABELS "PARAVIEW;CATALYST")
endif()
