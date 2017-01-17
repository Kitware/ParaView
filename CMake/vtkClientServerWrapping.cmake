#
#
#
include (vtkWrapClientServer)
#------------------------------------------------------------------------------
include(vtkModuleAPI)

# Adds client-server wrapping for a vtk-module. This uses vtkModuleAPI to load
# information about the indicated VTK module and then warps the headers and
# generates a library named ${module}CS.
macro(vtk_add_cs_wrapping module)
  if (NOT TARGET vtkWrapClientServer)
    message(FATAL_ERROR
      "Failed to locate vtkWrapClientServer target. ParaView targets "
      "may not have been imported correctly.")
  endif()

  if(NOT ${module}_EXCLUDE_FROM_WRAP_HIERARCHY)
    set(KIT_HIERARCHY_FILE ${${module}_WRAP_HIERARCHY_FILE})
  endif()

  pv_pre_wrap_vtk_mod_cs("${module}CS" "${module}")
  vtk_module_dep_includes(${module})
  vtk_add_library(${module}CS STATIC ${${module}CS_SRCS})
  target_link_libraries(${module}CS LINK_PUBLIC vtkClientServer LINK_PRIVATE ${module})
  set_property(TARGET ${module}CS APPEND
    PROPERTY INCLUDE_DIRECTORIES
    ${${module}_DEPENDS_INCLUDE_DIRS}
    ${${module}_INCLUDE_DIRS}
    ${vtkClientServer_INCLUDE_DIRS})
  if (NOT TARGET ${BARE_TARGET}PythonD OR
      NOT PARAVIEW_USE_UNIFIED_BINDINGS)
    set(NO_PYTHON_BINDINGS_AVAILABLE TRUE)
  endif ()
  if (NOT NO_PYTHON_BINDINGS_AVAILABLE)
    target_link_libraries(${module}CS
      LINK_PUBLIC
        ${module}PythonD
      LINK_PRIVATE
        vtkPythonInterpreter)
    set_property(TARGET ${module}CS APPEND
      PROPERTY INCLUDE_DIRECTORIES
      ${vtkPython_INCLUDE_DIRS}
      ${vtkPythonInterpreter_INCLUDE_DIRS})
  endif ()
  unset(NO_PYTHON_BINDINGS_AVAILABLE)
  if (NOT WIN32)
    set_property(TARGET ${module}CS APPEND
      PROPERTY COMPILE_FLAGS "-fPIC")
  endif()

  # add compile definition for auto init for modules that provide implementation
  if(${module}_IMPLEMENTS)
    set_property(TARGET ${module}CS PROPERTY COMPILE_DEFINITIONS
      "${module}_AUTOINIT=1(${module})")
  endif()
endmacro()

#------------------------------------------------------------------------------
macro(pv_find_vtk_header header include_dirs path)
  unset(${path})
  foreach(_dir ${include_dirs})
    if( EXISTS ${_dir}/${header} )
      set(${path} ${_dir}/${header})
      break()
    endif()
  endforeach()
endmacro()


#------------------------------------------------------------------------------
macro(pv_pre_wrap_vtk_mod_cs libname module)
  set(vtk${kit}CS_HEADERS)

  vtk_module_load(${module})
  vtk_module_headers_load(${module})

  foreach(class ${${module}_HEADERS})
    if(NOT ${module}_HEADER_${class}_WRAP_EXCLUDE)
      pv_find_vtk_header(${class}.h "${${module}_INCLUDE_DIRS}" pathfound)

      if(pathfound)
        if(${module}_HEADER_${class}_ABSTRACT)
          set_source_files_properties(${pathfound} PROPERTIES ABSTRACT 1)
        endif()
        list(APPEND ${module}CS_HEADERS ${pathfound})
      else()
        message(WARNING "Unable to find: ${class}")
      endif()
    endif()
  endforeach()

  # build hints file for the module
  if(VTK_WRAP_HINTS)
    set(SAVED_VTK_WRAP_HINTS ${VTK_WRAP_HINTS})
  endif()

  # variable storing VTK's hints combined with the module's hints
  set(COMBINED_HINTS "")

  if(VTK_WRAP_HINTS)
    file(READ ${VTK_WRAP_HINTS} COMBINED_HINTS)
  endif()

  set(hints_added FALSE)
  foreach(_dir ${${module}_INCLUDE_DIRS})
    if(EXISTS ${_dir}/hints)
      file(READ "${_dir}/hints" MODULE_HINTS)
      set(COMBINED_HINTS "${COMBINED_HINTS}\n${MODULE_HINTS}")
      set(hints_added TRUE)
    endif()
  endforeach()

  if(hints_added AND COMBINED_HINTS)
    # combined hints are generated only we we have more than the default hints
    # specified by VTK_WRAP_HINTS that need to be used.
    string(STRIP "${COMBINED_HINTS}" CMAKE_CONFIGURABLE_FILE_CONTENT)
    configure_file(
      ${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
      ${CMAKE_CURRENT_BINARY_DIR}/${module}_wrapping_hints @ONLY)
    set(VTK_WRAP_HINTS "${CMAKE_CURRENT_BINARY_DIR}/${module}_wrapping_hints")
  endif()

  VTK_WRAP_ClientServer("${libname}" "${module}CS_SRCS" "${${module}CS_HEADERS}")

  # restore VTK_WRAP_HINTS
  if(SAVED_VTK_WRAP_HINTS)
    set(VTK_WRAP_HINTS ${SAVED_VTK_WRAP_HINTS})
  endif()
endmacro()

#------------------------------------------------------------------------------
# use this to wrap an arbitrary list of source files. It detects the headers and
# CS wraps those. the result is a variable named ${libname}CS_SRCS with all the
# generated files
macro(pv_pre_wrap_sources_cs libname)
  set (__tmp_headers)
  foreach (src ${ARGN})
    get_filename_component(src_name "${src}" NAME_WE)
    get_filename_component(src_path "${src}" ABSOLUTE)
    get_filename_component(src_path "${src_path}" PATH)

    if (EXISTS "${src_path}/${src_name}.h")
      list (APPEND __tmp_headers "${src_path}/${src_name}.h")
    elseif (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${src_name}.h")
      list (APPEND __tmp_headers "${CMAKE_CURRENT_BINARY_DIR}/${src_name}.h")
    endif()
  endforeach()
  VTK_WRAP_ClientServer("${libname}" "${libname}CS_SRCS" "${__tmp_headers}")
endmacro()

#------------------------------------------------------------------------------
MACRO(PV_PRE_WRAP_VTK_CS libname kit ukit deps)

  SET(vtk${kit}CS_HEADERS)
  INCLUDE("${VTK_KITS_DIR}/vtk${kit}Kit.cmake")
  # FOREACH(class ${VTK_${ukit}_CLASSES})
  FOREACH(class ${VTK_${ukit}_HEADERS})
    SET(full_name "${VTK_${ukit}_HEADER_DIR}/${class}.h")
    IF("${class}" MATCHES "^(\\/|.\\/|.\\\\|.:\\/|.:\\\\)")
      # handle full paths
      SET(full_name "${class}.h")
    ENDIF()
    IF(NOT VTK_CLASS_WRAP_EXCLUDE_${class})
      IF(VTK_CLASS_ABSTRACT_${class})
        SET_SOURCE_FILES_PROPERTIES(${full_name} PROPERTIES ABSTRACT 1)
      ENDIF()
      SET(vtk${kit}CS_HEADERS ${vtk${kit}CS_HEADERS} ${full_name})
    ENDIF()
  ENDFOREACH()
  VTK_WRAP_ClientServer("${libname}" "vtk${kit}CS_SRCS" "${vtk${kit}CS_HEADERS}")
ENDMACRO()

#------------------------------------------------------------------------------
# Macro to create ClientServer wrappers classes in a single VTK kit.
MACRO(PV_WRAP_VTK_CS kit ukit deps)
  SET(KIT_CS_DEPS)
  PV_PRE_WRAP_VTK_CS("vtk${kit}CS" "${kit}" "${ukit}" "${deps}")
  PVVTK_ADD_LIBRARY(vtk${kit}CS ${vtk${kit}CS_SRCS})
  TARGET_LINK_LIBRARIES(vtk${kit}CS vtkClientServer vtk${kit})
  pv_set_link_interface_libs(vtk${kit}CS "")
  FOREACH(dep ${deps})
    #MESSAGE("Link vtk${kit}CS to vtk${dep}CS")
    TARGET_LINK_LIBRARIES(vtk${kit}CS vtk${dep}CS)
  ENDFOREACH()
  IF(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)
    IF(BUILD_SHARED_LIBS)
      IF(NOT PV_INSTALL_NO_LIBRARIES)
        INSTALL(TARGETS vtk${kit}CS
          EXPORT ${PV_INSTALL_EXPORT_NAME}
          RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT Runtime
          LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT Runtime
          ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development)
      ENDIF()
    ENDIF()
  ENDIF()
  IF(KIT_CS_DEPS)
    ADD_DEPENDENCIES(vtk${kit}CS ${KIT_CS_DEPS})
  ENDIF()
ENDMACRO()
