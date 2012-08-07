IF(NOT CMAKE_CROSSCOMPILING)
 
  # vtkParse.tab.c has #line markers that break #include with double quotes.
  INCLUDE_DIRECTORIES(${ParaView_SOURCE_DIR}/VTK/Wrapping)

  add_executable(vtkWrapClientServer
                vtkWrapClientServer.c
                ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParse.tab.c
                ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParsePreprocess.c
                ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParseMain.c)

  set_property(SOURCE ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParseMain.c PROPERTY COMPILE_DEFINITIONS
    VTK_PARSE_VERSION="${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
  vtk_compile_tools_target(vtkWrapClientServer)
ENDIF(NOT CMAKE_CROSSCOMPILING)

#==========================================================================
# Wrap all modules ParaView/VTK knows about.
include (vtkClientServerWrapping)
foreach(_module ${VTK_MODULES_ENABLED})
  if (NOT ${_module}_EXCLUDE_FROM_WRAPPING)
    pv_wrap_vtk_mod_cs(${_module})
  endif()
endforeach()
