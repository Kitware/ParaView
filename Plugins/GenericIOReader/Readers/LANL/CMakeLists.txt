add_library(LANL_GenericIO STATIC
  GIO/CRC64.h
  GIO/GenericIO.cxx
  GIO/GenericIO.h
  utils/gioData.h
  utils/log.h
  utils/octree.h
  utils/strConvert.h
  utils/timer.h)

target_include_directories(LANL_GenericIO
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_compile_definitions(LANL_GenericIO
  PUBLIC
    # This plugin depends on a customized snapshot of GenericIO.
    LANL_GENERICIO_NO_MPI
  PRIVATE
    # Compression not supported yet
    LANL_GENERICIO_NO_COMPRESSION
    # Required by GenericIO target.
    __STDC_CONSTANT_MACROS)
set_property(TARGET LANL_GenericIO
  PROPERTY
    POSITION_INDEPENDENT_CODE ON)

_vtk_module_apply_properties(LANL_GenericIO)
_vtk_module_install(LANL_GenericIO)
