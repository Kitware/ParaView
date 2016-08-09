cmake_minimum_required(VERSION 3.3)
project(NPICAdaptor)

find_package(ParaView REQUIRED COMPONENTS vtkPVCatalyst)
include("${PARAVIEW_USE_FILE}")

set(NPICAdaptor_SRCS NPICAdaptor.cxx)
add_library(NPICAdaptor ${NPICAdaptor_SRCS})
target_link_libraries(NPICAdaptor vtkPVCatalyst)
