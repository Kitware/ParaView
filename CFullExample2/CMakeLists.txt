cmake_minimum_required(VERSION 2.8.8)
project(CatalystCFullExample C)

set(USE_CATALYST ON CACHE BOOL "Link the simulator with Catalyst")
if(USE_CATALYST)
  # we only need C++ compilers if we're building the Adaptor
  enable_language(CXX)
  find_package(ParaView 3.98 REQUIRED COMPONENTS vtkPVPythonCatalyst)
  include("${PARAVIEW_USE_FILE}")
  set(Adaptor_SRCS
    FEAdaptor.cxx
    )
  add_library(Adaptor ${Adaptor_SRCS})
  target_link_libraries(Adaptor vtkPVPythonCatalyst)
  add_definitions("-DUSE_CATALYST")
else()
  find_package(MPI REQUIRED)
  include_directories(${MPI_C_INCLUDE_PATH})
endif()


add_executable(FEDriver FEDriver.c FEDataStructures.c)
if(USE_CATALYST)
  target_link_libraries(FEDriver Adaptor)
else()
  target_link_libraries(FEDriver ${MPI_LIBRARIES})
endif()

