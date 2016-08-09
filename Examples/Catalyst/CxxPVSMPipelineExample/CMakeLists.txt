cmake_minimum_required(VERSION 3.3)
project(CatalystCxxPVSMPipelineExample)

set(USE_CATALYST ON CACHE BOOL "Link the simulator with Catalyst")
if(USE_CATALYST)
  # We have a C++ pipeline so we don't need to link with Python
  # or vtkPVPythonCatalyst since vtkPVCatalyst is sufficient.
  find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVCatalyst vtkPVVTKExtensionsDefault)
  include("${PARAVIEW_USE_FILE}")
  set(Adaptor_SRCS
    FEAdaptor.cxx
    vtkCPPVSMPipeline.cxx
    )
  add_library(CxxPVSMPipelineExampleAdaptor ${Adaptor_SRCS})
  target_link_libraries(CxxPVSMPipelineExampleAdaptor vtkPVCatalyst vtkPVVTKExtensionsDefault)
  add_definitions("-DUSE_CATALYST")
  if(NOT PARAVIEW_USE_MPI)
    message(SEND_ERROR "ParaView must be built with MPI enabled")
  endif()
else()
  find_package(MPI REQUIRED)
  include_directories(${MPI_CXX_INCLUDE_PATH})
endif()

add_executable(CxxPVSMPipelineExample FEDriver.cxx FEDataStructures.cxx)
if(USE_CATALYST)
  target_link_libraries(CxxPVSMPipelineExample LINK_PRIVATE CxxPVSMPipelineExampleAdaptor)
  include(vtkModuleMacros)
  include(vtkMPI)
  vtk_mpi_link(CxxPVSMPipelineExample)
else()
  target_link_libraries(CxxPVSMPipelineExample LINK_PRIVATE ${MPI_LIBRARIES})
endif()

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(NAME CxxPVSMPipelineExampleTest COMMAND CxxPVSMPipelineExample 5 output)
  set_tests_properties(CxxPVSMPipelineExampleTest PROPERTIES LABELS "PARAVIEW;CATALYST")
endif()
