project(NPICAdaptor)
cmake_minimum_required(VERSION 2.6)
#include_directories(${CoProcessor_SOURCE_DIR})
include_directories(${FortranAdaptor_SOURCE_DIR})
# pull in ParaView.
#find_package(ParaView)
#include(${PARAVIEW_USE_FILE})

set(NPICAdaptor_SRCS
  NPICAdaptor.cxx
)
add_library(NPICAdaptor ${NPICAdaptor_SRCS})
target_link_libraries(
  NPICAdaptor FortranAdaptor
)
