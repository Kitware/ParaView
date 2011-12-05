project(FortranAdaptor)
cmake_minimum_required(VERSION 2.6)
include_directories(${CoProcessor_SOURCE_DIR})

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

# Attempt to detect if we have a Fortran Compiler available.
include(${ParaView_SOURCE_DIR}/SuperBuild/CheckFortran.cmake)

include_directories(${FortranAdaptor_BINARY_DIR})

if(CMAKE_Fortran_COMPILER)
  ENABLE_LANGUAGE(Fortran)
  include(FortranCInterface)

  FortranCInterface_HEADER(FortranAdaptorAPIMangling.h SYMBOLS
    coprocessorinitialize coprocessorfinalize requestdatadescription
    needtocreategrid coprocess)

  set(COPROCESSOR_MANGLE_FORTRAN 1)
endif()

CONFIGURE_FILE("${CoProcessing_SOURCE_DIR}/Adaptors/FortranAdaptors/CPUseFortran.h.in"
               "${FortranAdaptor_BINARY_DIR}/CPUseFortran.h"
               @ONLY IMMEDIATE)

set(FortranAdaptor_SRCS
  FortranAdaptorAPI.cxx
)

add_library(FortranAdaptor ${FortranAdaptor_SRCS})
target_link_libraries(
  FortranAdaptor vtkCoProcessor
)

if(CMAKE_Fortran_COMPILER)
  option(BUILD_PHASTA_ADAPTOR "Build Phasta Adaptor" OFF)
  if(BUILD_PHASTA_ADAPTOR)
    add_subdirectory(PhastaAdaptor)
  endif(BUILD_PHASTA_ADAPTOR)
endif()

option(BUILD_NPIC_ADAPTOR "Build NPIC Adaptor" OFF)
if(BUILD_NPIC_ADAPTOR)
  add_subdirectory(NPICAdaptor)
endif(BUILD_NPIC_ADAPTOR)