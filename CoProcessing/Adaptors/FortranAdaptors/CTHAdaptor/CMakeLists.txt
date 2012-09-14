project(CTHAdaptor)
cmake_minimum_required(VERSION 2.6)
#include_directories(${CoProcessor_SOURCE_DIR})
include_directories(${FortranAdaptor_SOURCE_DIR})
# pull in ParaView.
#find_package(ParaView)
#include(${PARAVIEW_USE_FILE})

set(CTHAdaptor_SRCS
  CTHAdaptor.cxx
  vtkCTHSource.cxx
  vtkCTHDataArray.cxx
)
add_library(CTHAdaptor ${CTHAdaptor_SRCS})
target_link_libraries(
  CTHAdaptor FortranAdaptor
)
