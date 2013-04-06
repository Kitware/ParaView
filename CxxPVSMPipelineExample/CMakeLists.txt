cmake_minimum_required(VERSION 2.8.8)
project(CxxPVSMPipelineExample)

set(USE_CATALYST ON CACHE BOOL "Link the simulator with Catalyst")
if(USE_CATALYST)
  # We have a C++ pipeline so we don't need to link with Python
  # or vtkPVPythonCatalyst since vtkPVCatalyst is sufficient.
  find_package(ParaView 3.98 REQUIRED COMPONENTS vtkPVCatalyst vtkPVVTKExtensionsDefault)
  include("${PARAVIEW_USE_FILE}")
  set(Adaptor_SRCS
    FEAdaptor.cxx
    vtkCPPVSMPipeline.cxx
    )
  add_library(Adaptor ${Adaptor_SRCS})
  target_link_libraries(Adaptor vtkPVCatalyst vtkPVVTKExtensionsDefault)
  add_definitions("-DUSE_CATALYST")
else()
  find_package(MPI REQUIRED)
  include_directories(${MPI_CXX_INCLUDE_PATH})
endif()


add_executable(FEDriver FEDriver.cxx FEDataStructures.cxx)
if(USE_CATALYST)
  target_link_libraries(FEDriver Adaptor)
else()
  target_link_libraries(FEDriver ${MPI_LIBRARIES})
endif()

