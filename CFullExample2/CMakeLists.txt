cmake_minimum_required(VERSION 2.8.8)
project(CatalystCFullExample2 C)

set(USE_CATALYST ON CACHE BOOL "Link the simulator with Catalyst")
if(USE_CATALYST)
  # we only need C++ compilers if we're building the Adaptor
  enable_language(CXX)
  find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVPythonCatalyst vtkParallelMPI)
  include("${PARAVIEW_USE_FILE}")
  set(Adaptor_SRCS
    FEAdaptor.cxx
    )
  add_library(Adaptor ${Adaptor_SRCS})
  target_link_libraries(Adaptor vtkPVPythonCatalyst)
  add_definitions("-DUSE_CATALYST")
  if(NOT PARAVIEW_USE_MPI)
    message(SEND_ERROR "ParaView must be built with MPI enabled")
  endif()
else()
  find_package(MPI REQUIRED)
  include_directories(${MPI_C_INCLUDE_PATH})
endif()

add_executable(FEDriver FEDriver.c FEDataStructures.c)
if(USE_CATALYST)
  target_link_libraries(FEDriver Adaptor ${VTK_LIBRARIES})
else()
  target_link_libraries(FEDriver ${MPI_LIBRARIES})
endif()

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(CFullExample2Test FEDriver ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
