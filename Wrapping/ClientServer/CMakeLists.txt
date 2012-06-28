IF(NOT CMAKE_CROSSCOMPILING)
 
  # vtkParse.tab.c has #line markers that break #include with double quotes.
  INCLUDE_DIRECTORIES(${ParaView_SOURCE_DIR}/VTK/Wrapping)

  vtk_add_executable(vtkWrapClientServer
                vtkWrapClientServer.c
                ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParse.tab.c
                ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParsePreprocess.c
                ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParseMain.c)

  set_property(SOURCE ${ParaView_SOURCE_DIR}/VTK/Wrapping/vtkParseMain.c PROPERTY COMPILE_DEFINITIONS
    VTK_PARSE_VERSION="${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")


  EXPORT(TARGETS vtkWrapClientServer FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
  IF(COMPILE_TOOLS_TARGET)
    ADD_DEPENDENCIES(${COMPILE_TOOLS_TARGET} vtkWrapClientServer)
  ENDIF(COMPILE_TOOLS_TARGET)

  IF(NOT PV_INSTALL_NO_DEVELOPMENT)
    INSTALL(
        TARGETS vtkWrapClientServer
        DESTINATION ${VTK_INSTALL_RUNTIME_DIR}
        COMPONENT Development)
  ENDIF(NOT PV_INSTALL_NO_DEVELOPMENT)

ENDIF(NOT CMAKE_CROSSCOMPILING)

#==========================================================================
# Wrap all modules ParaView/VTK knows about.
include (vtkClientServerWrapping)
foreach(_module ${VTK_MODULES_ENABLED})
  if (NOT ${_module}_EXCLUDE_FROM_WRAPPING)
    pv_wrap_vtk_mod_cs(${_module})
  endif()
endforeach()
