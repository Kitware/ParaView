cmake_minimum_required(VERSION 2.8.8)
project(CatalystCxxFullExample)

find_package(ParaView 3.98 REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")
set(Adaptor_SRCS
  FEAdaptor.cxx
  )
add_library(Adaptor ${Adaptor_SRCS})
target_link_libraries(Adaptor vtkPVPythonCatalyst)
add_definitions("-DUSE_CATALYST")

add_executable(FEDriver FEDriver.cxx)
target_link_libraries(FEDriver Adaptor)