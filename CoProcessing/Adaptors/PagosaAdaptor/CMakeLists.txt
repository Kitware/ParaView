cmake_minimum_required(VERSION 2.8.8)
project(PagosaAdaptor)

find_package(ParaView REQUIRED COMPONENTS vtkPVCatalyst vtkPVPythonCatalyst)
include(${PARAVIEW_USE_FILE})

enable_language(Fortran)
include(FortranCInterface)

FortranCInterface_HEADER(PagosaAdaptorAPIMangling.h SYMBOLS
  setcoprocessorgeometry setcoprocessorfield
  setmarkergeometry
  setmarkerscalarfield setmarkervectorfield setmarkertensorfield)

include_directories(${PagosaAdaptor_BINARY_DIR})

set(PagosaAdaptor_SRCS
  PagosaAdaptor.cxx
)
add_library(PagosaAdaptor ${PagosaAdaptor_SRCS})
target_link_libraries(
  PagosaAdaptor vtkPVCatalyst vtkPVPythonCatalyst
)
