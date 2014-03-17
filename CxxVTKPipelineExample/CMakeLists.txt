cmake_minimum_required(VERSION 2.8.8)
project(CxxVTKPipelineExample)

set(USE_CATALYST ON CACHE BOOL "Link the simulator with Catalyst")
if(USE_CATALYST)
  # We have a C++ pipeline so we don't need to link with Python
  # or vtkPVPythonCatalyst since vtkPVCatalyst is sufficient.
  find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVCatalyst vtkPVVTKExtensionsDefault vtkPVClientServerCoreCore vtkParallelMPI)
  include("${PARAVIEW_USE_FILE}")
  set(Adaptor_SRCS
    FEAdaptor.cxx
    vtkCPVTKPipeline.cxx
    )
  add_library(Adaptor ${Adaptor_SRCS})
  target_link_libraries(Adaptor vtkPVCatalyst vtkPVVTKExtensionsDefault vtkPVClientServerCoreCore)
  add_definitions("-DUSE_CATALYST")
  if(NOT PARAVIEW_USE_MPI)
    message(SEND_ERROR "ParaView must be built with MPI enabled")
  endif()
else()
  find_package(MPI REQUIRED)
  include_directories(${MPI_CXX_INCLUDE_PATH})
endif()


add_executable(FEDriver FEDriver.cxx FEDataStructures.cxx)
if(USE_CATALYST)
  target_link_libraries(FEDriver Adaptor ${VTK_LIBRARIES})
else()
  target_link_libraries(FEDriver ${MPI_LIBRARIES})
endif()

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(CxxVTKPipelineExampleTest FEDriver 10 output)
endif()
