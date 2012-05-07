#
#
#
INCLUDE (${VTKCS_CONFIG_DIR}/vtkWrapClientServer.cmake)
#------------------------------------------------------------------------------
MACRO(CS_INITIALIZE_WRAP)
  SET(EXECUTABLE_OUTPUT_PATH ${VTKCS_BINARY_DIR}/bin CACHE PATH "Single output path for executable")
ENDMACRO(CS_INITIALIZE_WRAP)

include(vtkModuleAPI)

macro(pv_wrap_vtk_mod_cs module)
  pv_pre_wrap_vtk_mod_cs("${module}CS" "${module}")
  PVVTK_ADD_LIBRARY(${module}CS ${${module}CS_SRCS})
  target_link_libraries(${module}CS vtkClientServer ${module})

  # satisfy the auto init calls, this should done in VTK eventually  
  if( ${module} STREQUAL "vtkRenderingCore" )
    target_link_libraries(${module}CS 
      vtkRenderingOpenGL
      vtkInteractionStyle
      vtkRenderingFreeTypeOpenGL)
  endif() 
  if( ${module} STREQUAL "vtkRenderingVolume" )
    target_link_libraries(${module}CS vtkRenderingVolumeOpenGL)
  endif()
  if( ${module} STREQUAL "vtkRenderingFreeType" )
    target_link_libraries(${module}CS vtkRenderingFreeTypeOpenGL)
  endif()
  foreach(dep ${${module}_DEPENDS})
    if(NOT ${dep}_EXCLUDE_FROM_WRAPPING)
      target_link_libraries(${module}CS ${dep}CS)
    endif()
  endforeach()
  if(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)
    if(BUILD_SHARED_LIBS)
      if(NOT PV_INSTALL_NO_LIBRARIES)
        install(TARGETS ${module}CS
          EXPORT ${PV_INSTALL_EXPORT_NAME}
          RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
          LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
          ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
      endif()
    endif()
  endif()
endmacro()


#------------------------------------------------------------------------------
macro(pv_pre_wrap_vtk_mod_cs libname module)
  set(vtk${kit}CS_HEADERS)
  
  vtk_module_load(${module})
  vtk_module_classes_load(${module})
  
  foreach(class ${${module}_CLASSES})
    find_file(${class}_full_name "${class}.h" PATHS ${${module}_INCLUDE_DIRS} )
    set_property(CACHE ${class}_full_name PROPERTY TYPE INTERNAL)
    if(${class}_full_name)
      if(NOT ${module}_CLASS_${class}_WRAP_EXCLUDE)
        if(${module}_CLASS_${class}_ABSTRACT)
          set_source_files_properties(${${class}_full_name} PROPERTIES ABSTRACT 1)
        endif()
        list(APPEND ${module}CS_HEADERS ${${class}_full_name})
      endif()
    elseif()
      message(WARNING "Unable to find: ${class}")
    endif()  
  endforeach()
  
  VTK_WRAP_ClientServer("${libname}" "${module}CS_SRCS" "${${module}CS_HEADERS}")
endmacro()

#------------------------------------------------------------------------------
MACRO(PV_PRE_WRAP_VTK_CS libname kit ukit deps)

  SET(vtk${kit}CS_HEADERS)
  INCLUDE("${VTK_KITS_DIR}/vtk${kit}Kit.cmake")
  FOREACH(class ${VTK_${ukit}_CLASSES})
    SET(full_name "${VTK_${ukit}_HEADER_DIR}/${class}.h")
    IF("${class}" MATCHES "^(\\/|.\\/|.\\\\|.:\\/|.:\\\\)")
      # handle full paths
      SET(full_name "${class}.h")
    ENDIF("${class}" MATCHES "^(\\/|.\\/|.\\\\|.:\\/|.:\\\\)")
    IF(NOT VTK_CLASS_WRAP_EXCLUDE_${class})
      IF(VTK_CLASS_ABSTRACT_${class})
        SET_SOURCE_FILES_PROPERTIES(${full_name} PROPERTIES ABSTRACT 1)
      ENDIF(VTK_CLASS_ABSTRACT_${class})
      SET(vtk${kit}CS_HEADERS ${vtk${kit}CS_HEADERS} ${full_name})
    ENDIF(NOT VTK_CLASS_WRAP_EXCLUDE_${class})
  ENDFOREACH(class)
  VTK_WRAP_ClientServer("${libname}" "vtk${kit}CS_SRCS" "${vtk${kit}CS_HEADERS}")
ENDMACRO(PV_PRE_WRAP_VTK_CS kit ukit deps)
 
#------------------------------------------------------------------------------
# Macro to create ClientServer wrappers classes in a single VTK kit.
MACRO(PV_WRAP_VTK_CS kit ukit deps)
  SET(KIT_CS_DEPS)
  PV_PRE_WRAP_VTK_CS("vtk${kit}CS" "${kit}" "${ukit}" "${deps}")
  PVVTK_ADD_LIBRARY(vtk${kit}CS ${vtk${kit}CS_SRCS})
  TARGET_LINK_LIBRARIES(vtk${kit}CS vtkClientServer vtk${kit})
  FOREACH(dep ${deps})
    #MESSAGE("Link vtk${kit}CS to vtk${dep}CS")
    TARGET_LINK_LIBRARIES(vtk${kit}CS vtk${dep}CS)
  ENDFOREACH(dep)
  IF(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)
    IF(BUILD_SHARED_LIBS)
      IF(NOT PV_INSTALL_NO_LIBRARIES)
        INSTALL(TARGETS vtk${kit}CS
          EXPORT ${PV_INSTALL_EXPORT_NAME}
          RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
          LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
          ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
      ENDIF(NOT PV_INSTALL_NO_LIBRARIES)
    ENDIF(BUILD_SHARED_LIBS)
  ENDIF(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)
  IF(KIT_CS_DEPS)
    ADD_DEPENDENCIES(vtk${kit}CS ${KIT_CS_DEPS})
  ENDIF(KIT_CS_DEPS)
ENDMACRO(PV_WRAP_VTK_CS)
