cmake_minimum_required(VERSION 3.3)
project(PhastaAdaptor)

find_package(ParaView REQUIRED COMPONENTS vtkPVCatalyst)
include("${PARAVIEW_USE_FILE}")

enable_language(Fortran)
include(FortranCInterface)

FortranCInterface_HEADER(PhastaAdaptorAPIMangling.h SYMBOLS
  createpointsandallocatecells insertblockofcells addfields)

include_directories(${PhastaAdaptor_BINARY_DIR})

set(PhastaAdaptor_SRCS
  PhastaAdaptor.cxx
)
add_library(PhastaAdaptor ${PhastaAdaptor_SRCS})
target_link_libraries(
  PhastaAdaptor vtkPVCatalyst
)
