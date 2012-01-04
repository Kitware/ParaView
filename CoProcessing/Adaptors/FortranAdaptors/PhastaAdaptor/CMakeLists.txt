project(PhastaAdaptor)
cmake_minimum_required(VERSION 2.6)

include(FortranCInterface)

FortranCInterface_HEADER(PhastaAdaptorAPIMangling.h SYMBOLS
  createpointsandallocatecells insertblockofcells addfields)

include_directories(${FortranAdaptor_BINARY_DIR} ${FortranAdaptor_SOURCE_DIR}
  ${PhastaAdaptor_BINARY_DIR})

set(PhastaAdaptor_SRCS
  PhastaAdaptor.cxx
)
add_library(PhastaAdaptor ${PhastaAdaptor_SRCS})
target_link_libraries(
  PhastaAdaptor FortranAdaptor
)

