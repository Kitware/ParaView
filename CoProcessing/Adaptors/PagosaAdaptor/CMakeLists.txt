cmake_minimum_required(VERSION 2.8.8)
project(PagosaAdaptor)

set(STANDALONE_BUILD 0)
if(NOT TARGET vtkPVPythonCatalyst)
  find_package(ParaView REQUIRED COMPONENTS vtkPVCatalyst vtkPVPythonCatalyst)
  include(${PARAVIEW_USE_FILE})
  set(STANDALONE_BUILD 1)
else()
  include_directories(${CMAKE_SOURCE_DIR}/CoProcessing/Catalyst ${CMAKE_SOURCE_DIR}/CoProcessing/PythonCatalyst ${VTK_INCLUDE_DIRS})
endif()

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

if(PARAVIEW_INSTALL_DEVELOPMENT_FILES OR STANDALONE_BUILD)
  install(TARGETS PagosaAdaptor DESTINATION ${CMAKE_INSTALL_PREFIX}/lib COMPONENT Development)
  if(STANDALONE_BUILD)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/PagosaAdaptorAPIMangling.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  else()
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/PagosaAdaptorAPIMangling.h DESTINATION ${VTK_INSTALL_INCLUDE_DIR})
  endif()
endif()
