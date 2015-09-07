#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
set (Module_SRCS
  vtkInitializationHelper.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkInitializationHelper
  ABSTRACT
  )

# OpenGL2 specific definitions
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  add_definitions(-DVTKGL2)
endif()

vtk_module_library(vtkPVServerManagerApplication ${Module_SRCS})

#------------------------------------------------------------------------------
# Generate ServerManager (xml and CS stream) initialization header.
#------------------------------------------------------------------------------
set(vtk-module-init-functions)
set(vtk-module-init-calls)
set(_dependencies)
set(module_sm_xmls)

# Create list of the modules we are going to client server wrap.
# The user can specify a list in PARAVIEW_CS_MODULES
if (PARAVIEW_CS_MODULES)
  # use the user provide variable
  set(PARAVIEW_CURRENT_CS_MODULES ${PARAVIEW_CS_MODULES} CACHE INTERNAL "List of modules to CS wrap")
else()
  # Default to VTK_MODULES_ENABLED but need to filter the list first ( its
  # used else where without the guards below )
  set(_tmp_list)
  foreach (_module ${VTK_MODULES_ENABLED})
    if (NOT ${_module}_EXCLUDE_FROM_WRAPPING AND
        NOT ${_module}_IS_TEST AND
        NOT ${_module}_THIRD_PARTY)
      list(APPEND _tmp_list ${_module})
    endif()
  endforeach()
  set(PARAVIEW_CURRENT_CS_MODULES ${_tmp_list} CACHE INTERNAL "List of modules to CS wrap")
endif()

foreach (_module ${PARAVIEW_CURRENT_CS_MODULES})
  if (NOT ${_module}_EXCLUDE_FROM_WRAPPING AND
      NOT ${_module}_IS_TEST AND
      NOT ${_module}_THIRD_PARTY AND
        (NOT _module STREQUAL "vtkPVServerManagerApplication") AND
        (NOT _module MATCHES ".*Catalyst.*"))
    #message("${_module} == Adding")
    set(vtk-module-init-functions
      "${vtk-module-init-functions}extern \"C\" void ${_module}CS_Initialize(vtkClientServerInterpreter*);\n")
    set(vtk-module-init-calls
      "${vtk-module-init-calls}  ${_module}CS_Initialize(interp);\n")

    list(APPEND _dependencies ${_module}CS)
  endif()

  # if module provides server manager xmls, process them.
  get_property(module_xmls GLOBAL PROPERTY ${_module}_SERVERMANAGER_XMLS)
  if (module_xmls)
    foreach(xml_path IN LISTS module_xmls)
      list(APPEND module_sm_xmls ${xml_path})
    endforeach()
  endif()
endforeach()

# Add link-dependencies on all wrapping classes.
target_link_libraries(vtkPVServerManagerApplication
  LINK_PRIVATE ${_dependencies}
  LINK_PUBLIC vtkPVServerManagerCore)

#------------------------------------------------------------------------------
# Setup SM xmls. 
#------------------------------------------------------------------------------
SET(requestedResourceFiles
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/3d_widgets.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/filters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/internal_writers.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/readers.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/rendering.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/sources.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/utilities.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/views_and_representations.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/writers.xml
  ${module_sm_xmls}
  )
if (PARAVIEW_USE_MPI)
  list(APPEND requestedResourceFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/proxies_mpi.xml)
endif()
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  list(APPEND requestedResourceFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/proxies_opengl2.xml)
else()
  # Old OpenGL.
  list(APPEND requestedResourceFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/proxies_opengl1.xml)
endif()

#------------------------------------------------------------------------------
list(FIND VTK_MODULES_ENABLED "vtkIOXdmf3" vtkioxdmf3_index)
if (vtkioxdmf3_index GREATER 0)
  list(APPEND requestedResourceFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/proxies_xdmf3.xml)
endif()

list(FIND VTK_MODULES_ENABLED "vtkIOADIOS" vtkioadios_index)
if (vtkioadios_index GREATER 0)
  list(APPEND requestedResourceFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/proxies_adios.xml)
endif()
#------------------------------------------------------------------------------

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND requestedResourceFiles
      ${CMAKE_CURRENT_SOURCE_DIR}/Resources/pythonfilter.xml
  )
endif()
set(resourceFiles)
foreach(requestedResourceFile IN LISTS requestedResourceFiles)
  if (EXISTS "${requestedResourceFile}")
    list(APPEND resourceFiles "${requestedResourceFile}")
  endif ()
endforeach()
set (PARAVIEW_SERVERMANAGER_XMLS ${resourceFiles}
    CACHE INTERNAL "Server Manager XMLs")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h
  DEPENDS ${resourceFiles} kwProcessXML
  COMMAND kwProcessXML
       ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h
       vtkSMDefaultModules Interface GetInterfaces
       ${resourceFiles}
  COMMENT "Generating headers for ServerManager XMLs")
set_source_files_properties(vtkInitializationHelper.cxx
  PROPERTIES OBJECT_DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h)


set (xml_init_code "")
foreach(rf ${resourceFiles})
  string(REGEX REPLACE "^.*/(.*).(xml|pvsm)$" "\\1" moduleName "${rf}")
  set (oneModule
"  init_string =  vtkSMDefaultModules${moduleName}GetInterfaces();
  xmls.push_back(init_string);
  delete[] init_string; init_string = NULL;")
  set(xml_init_code
    "${xml_init_code}\n${oneModule}")
endforeach()

#------------------------------------------------------------------------------
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkPVInitializer.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVInitializer.h
  @ONLY)
#------------------------------------------------------------------------------
