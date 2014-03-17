cmake_minimum_required(VERSION 2.8.8)
project(CatalystCxxOverlappoingAMRExample)

find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")
set(Adaptor_SRCS
  FEAdaptor.cxx
  )
add_library(Adaptor ${Adaptor_SRCS})
target_link_libraries(Adaptor vtkPVPythonCatalyst)

add_executable(FEDriver FEDriver.cxx)
target_link_libraries(FEDriver Adaptor)

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(CxxOverlappingAMRExampleTest FEDriver ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
endif()
