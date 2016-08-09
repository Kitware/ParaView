cmake_minimum_required(VERSION 3.3)
project(CatalystCxxVTKPipelineExample)

set(USE_CATALYST ON CACHE BOOL "Link the simulator with Catalyst")
if(USE_CATALYST)
  # We have a C++ pipeline so we don't need to link with Python
  # or vtkPVPythonCatalyst since vtkPVCatalyst is sufficient.
  find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVCatalyst vtkPVVTKExtensionsDefault vtkPVClientServerCoreCore)
  include("${PARAVIEW_USE_FILE}")
  set(Adaptor_SRCS
    FEAdaptor.cxx
    vtkCPVTKPipeline.cxx
    )
  add_library(CxxVTKPipelineExampleAdaptor ${Adaptor_SRCS})
  target_link_libraries(CxxVTKPipelineExampleAdaptor vtkPVCatalyst vtkPVVTKExtensionsDefault vtkPVClientServerCoreCore)
  add_definitions("-DUSE_CATALYST")
  if(NOT PARAVIEW_USE_MPI)
    message(SEND_ERROR "ParaView must be built with MPI enabled")
  endif()
else()
  find_package(MPI REQUIRED)
  include_directories(${MPI_CXX_INCLUDE_PATH})
endif()


add_executable(CxxVTKPipelineExample FEDriver.cxx FEDataStructures.cxx)
if(USE_CATALYST)
  target_link_libraries(CxxVTKPipelineExample LINK_PRIVATE CxxVTKPipelineExampleAdaptor)
  include(vtkModuleMacros)
  include(vtkMPI)
  vtk_mpi_link(CxxVTKPipelineExample)
else()
  target_link_libraries(CxxVTKPipelineExample LINK_PRIVATE ${MPI_LIBRARIES})
endif()

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(NAME CxxVTKPipelineExampleTest COMMAND CxxVTKPipelineExample 10 output)
  set_tests_properties(CxxVTKPipelineExampleTest PROPERTIES LABELS "PARAVIEW;CATALYST")
endif()
