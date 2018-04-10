cmake_minimum_required(VERSION 3.3)
project(CxxHyperTreeGridExample)

find_package(ParaView 5.5 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")
if(NOT PARAVIEW_USE_MPI)
  message(SEND_ERROR "ParaView must be built with MPI enabled")
endif()

add_executable(CxxHyperTreeGridExample FEDriver.cxx FEAdaptor.cxx)
target_link_libraries(CxxHyperTreeGridExample LINK_PRIVATE vtkPVPythonCatalyst vtkParallelMPI)
include(vtkModuleMacros)
include(vtkMPI)
vtk_mpi_link(CxxHyperTreeGridExample)

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(NAME CxxHyperTreeGridExampleTest COMMAND CxxHyperTreeGridExample ${CMAKE_CURRENT_SOURCE_DIR}/../SampleScripts/gridwriter.py)
  set_tests_properties(CxxHyperTreeGridExampleTest PROPERTIES LABELS "PARAVIEW;CATALYST")
endif()
