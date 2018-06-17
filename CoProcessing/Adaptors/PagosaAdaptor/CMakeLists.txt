cmake_minimum_required(VERSION 3.3)
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

set_property(GLOBAL APPEND PROPERTY VTK_TARGETS PagosaAdaptor)

if(STANDALONE_BUILD)
  install(TARGETS PagosaAdaptor EXPORT PagosaAdaptorTargets
    DESTINATION lib
    COMPONENT Development)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/PagosaAdaptorAPIMangling.h
    DESTINATION include)
  export(TARGETS PagosaAdaptor FILE
    PagosaAdaptor.cmake)
#  export(EXPORT PagosaAdaptorTargets
#    FILE ${PagosaAdaptor_BINARY_DIR}/PagosaAdaptorTargets.cmake)
elseif(PARAVIEW_INSTALL_DEVELOPMENT_FILES)
  install(TARGETS PagosaAdaptor EXPORT ${PV_INSTALL_EXPORT_NAME}
    DESTINATION lib
    COMPONENT Development)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/PagosaAdaptorAPIMangling.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR})
endif()
