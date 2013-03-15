cmake_minimum_required(VERSION 2.8.8)
project(CTHAdaptor)

find_package(ParaView REQUIRED COMPONENTS vtkPVPythonCatalyst)
include("${PARAVIEW_USE_FILE}")
set(CTHAdaptor_SRCS
  CTHAdaptor.cxx
  vtkCTHSource.cxx
  vtkCTHDataArray.cxx
)

add_library(CTHAdaptor ${CTHAdaptor_SRCS})
target_link_libraries(
  CTHAdaptor
  vtkPVPythonCatalyst)
