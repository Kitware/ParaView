# Attempt to detect if we have a Fortran Compiler available.
include(${ParaView_SOURCE_DIR}/SuperBuild/CheckFortran.cmake)

if(CMAKE_Fortran_COMPILER)
  ENABLE_LANGUAGE(Fortran)
  include(FortranCInterface)

  FortranCInterface_HEADER(FortranAdaptorAPIMangling.h SYMBOLS
    coprocessorinitialize coprocessorfinalize requestdatadescription
    needtocreategrid coprocess)

  set(COPROCESSOR_MANGLE_FORTRAN 1)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CPUseFortran.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/CPUseFortran.h"
               @ONLY IMMEDIATE)

vtk_module_library(${vtk-module}
  FortranAdaptorAPI.cxx
  CPUseFortran.h)

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
