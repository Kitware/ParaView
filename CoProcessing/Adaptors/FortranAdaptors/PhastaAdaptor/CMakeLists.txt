project(PhastaAdaptor)
cmake_minimum_required(VERSION 2.6)
#include_directories(${CoProcessor_SOURCE_DIR})
include_directories(${FortranAdaptor_SOURCE_DIR})
# pull in ParaView.
#find_package(ParaView)
#include(${PARAVIEW_USE_FILE})

set(PhastaAdaptor_SRCS
  PhastaAdaptor.cxx
)
add_library(PhastaAdaptor ${PhastaAdaptor_SRCS})
target_link_libraries(
  PhastaAdaptor FortranAdaptor
)

