cmake_minimum_required(VERSION 3.3)
project(CxxGhostCellsExample)

# For this example we use the vtkExtentTranslator to compute the partitioning
# and ghost cells so ParaView is always required.
find_package(ParaView 5.4 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")

add_executable(CxxGhostCellsExample FEDriver.cxx FEDataStructures.cxx)
target_link_libraries(CxxGhostCellsExample LINK_PRIVATE vtkPVPythonCatalyst vtkParallelMPI)
include(vtkModuleMacros)
include(vtkMPI)
vtk_mpi_link(CxxGhostCellsExample)

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(NAME CxxGhostCellsExampleTest COMMAND CxxGhostCellsExample -s ${CMAKE_CURRENT_SOURCE_DIR}/../SampleScripts/gridwriter.py)
  set_tests_properties(CxxGhostCellsExampleTest PROPERTIES LABELS "PARAVIEW;CATALYST")
endif()
