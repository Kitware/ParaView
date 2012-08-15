IF(NOT CMAKE_CROSSCOMPILING)
  add_executable(vtkWrapClientServer vtkWrapClientServer.c)
  target_link_libraries(vtkWrapClientServer vtkWrappingCore)

  set_property(SOURCE ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParseMain.c PROPERTY COMPILE_DEFINITIONS
    VTK_PARSE_VERSION="${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")

  # vtkParse.tab.c has #line markers that break #include with double quotes.
  set_property(TARGET vtkWrapClientServer APPEND
               PROPERTY INCLUDE_DIRECTORIES
                        ${ParaView_SOURCE_DIR}/VTK/Common/Core
                        ${ParaView_BINARY_DIR}/VTK/Common/Core
                        ${ParaView_SOURCE_DIR}/VTK/Wrapping)
               
  vtk_compile_tools_target(vtkWrapClientServer)
ENDIF(NOT CMAKE_CROSSCOMPILING)

#==========================================================================
# Wrap all modules ParaView/VTK knows about.
include (vtkClientServerWrapping)
foreach(_module ${VTK_MODULES_ENABLED})
  if (NOT ${_module}_EXCLUDE_FROM_WRAPPING)
    set_property(GLOBAL APPEND PROPERTY VTK_CS_WRAPPED ${_module})
    vtk_add_cs_wrapping(${_module})
  endif()
endforeach()
