cmake_minimum_required(VERSION 3.3)
project(CatalystCxxOverlappingAMRExample)

find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")
set(Adaptor_SRCS
  FEAdaptor.cxx
  )
add_library(CxxOverlappingAMRExampleAdaptor ${Adaptor_SRCS})
target_link_libraries(CxxOverlappingAMRExampleAdaptor vtkPVPythonCatalyst)

add_executable(CxxOverlappingAMRExample FEDriver.cxx)
target_link_libraries(CxxOverlappingAMRExample CxxOverlappingAMRExampleAdaptor)

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  include(CTest)
  add_test(NAME CxxOverlappingAMRExampleTest COMMAND CxxOverlappingAMRExample
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/feslicescript.py)
  set_tests_properties(CxxOverlappingAMRExampleTest PROPERTIES LABELS "PARAVIEW;CATALYST")
endif()
