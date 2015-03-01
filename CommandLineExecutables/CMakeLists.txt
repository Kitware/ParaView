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

# Needed to have the Python version numbers available in paraview-config.
set(extra_modules)
if (PARAVIEW_ENABLE_PYTHON)
  find_package(PythonInterp REQUIRED)
  set(extra_modules vtkUtilitiesPythonInitializer)
endif ()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/vtkModules.h"
"#include \"vtkModuleDependencies.h\"

struct vtkModuleDependency
{
  const char* ModuleName;
  const char** LibraryNames;
  const char** Depends;
  bool Python;
} vtkModules[] = {
")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/vtkModuleDependencies.h"
"#include <cstdlib>

")
foreach (vtk_module IN LISTS VTK_MODULES_ENABLED extra_modules)
  if (NOT vtk_module STREQUAL last_module)
    set(has_python "false")
    if (TARGET ${vtk_module}PythonD)
      set(has_python "true")
    endif ()
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/vtkModules.h"
"  { \"${vtk_module}\", ${vtk_module}Libraries, ${vtk_module}Depends, ${has_python} },
")

    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/vtkModuleDependencies.h"
"const char* ${vtk_module}Libraries[] = { ")
    foreach (vtk_module_library IN LISTS "${vtk_module}_LIBRARIES")
      if (vtk_module_library STREQUAL "optimized" OR
          vtk_module_library STREQUAL "debug")
        unset(vtk_module_library_name)
      elseif (vtk_module_library MATCHES "^-l")
        string(SUBSTRING "${vtk_module_library}" 2 -1 vtk_module_library)
        find_library(vtk_module_library_name "${vtk_module_library}")
      elseif (IS_ABSOLUTE "${vtk_module_library}")
        set(vtk_module_library_name "${vtk_module_library}")
      elseif (vtk_module_library)
        set(type)
        get_property(type TARGET "${vtk_module_library}" PROPERTY TYPE)
        if (type STREQUAL "INTERFACE_LIBRARY")
          get_property(vtk_module_library TARGET "${vtk_module_library}" PROPERTY INTERFACE_LINK_LIBRARIES)
        endif ()
        set(vtk_module_library_name)
        get_property(vtk_module_library_name TARGET "${vtk_module_library}" PROPERTY OUTPUT_NAME)
        if (NOT vtk_module_library_name)
          set(vtk_module_library_name "${vtk_module_library}")
        endif ()
      endif ()
      if (vtk_module_library_name)
        file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/vtkModuleDependencies.h"
"\"${vtk_module_library_name}\", ")
      endif ()
    endforeach ()

    if (TARGET ${vtk_module}_KIT)
      file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/vtkModuleDependencies.h"
        "\"${${vtk_module}_KIT}\", ")
    elseif (TARGET ${vtk_module})
      get_property(target_type TARGET "${vtk_module}" PROPERTY TYPE)
      if (target_type MATCHES "_LIBRARY$")
        if (target_type STREQUAL "INTERFACE_LIBRARY")
          get_property(vtk_module_library TARGET "${vtk_module}" PROPERTY INTERFACE_LINK_LIBRARIES)
        else ()
          set(vtk_module_library "${vtk_module}")
        endif ()
        set(vtk_module_library_name)
        get_property(vtk_module_library_name TARGET "${vtk_module_library}" PROPERTY OUTPUT_NAME)
        file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/vtkModuleDependencies.h"
"\"${vtk_module_library_name}\", ")
      endif ()
    endif ()

    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/vtkModuleDependencies.h"
"NULL };
const char* ${vtk_module}Depends[] = { ")
    foreach (vtk_module_depends IN LISTS ${vtk_module}_DEPENDS)
      file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/vtkModuleDependencies.h"
"\"${vtk_module_depends}\", ")
    endforeach ()
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/vtkModuleDependencies.h"
"NULL };
")
    set(last_module ${vtk_module})
  endif ()
endforeach ()
file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/vtkModules.h"
"  { NULL, NULL, NULL, false }
};
")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/paraview-config.cxx.in"
  "${CMAKE_CURRENT_BINARY_DIR}/paraview-config.cxx"
  @ONLY)
pv_add_executable(paraview-config
  "${CMAKE_CURRENT_BINARY_DIR}/paraview-config.cxx")
target_link_libraries(paraview-config
  vtksys)

if (NOT TARGET vtkPVServerManagerApplication)
  message(STATUS "vtkPVServerManagerApplication module is not enabled. "
    "Command line executables will not be built.")
  return()
endif()

vtk_module_dep_includes(vtkPVServerManagerApplication)
include_directories(${vtkPVServerManagerApplication_DEPENDS_INCLUDE_DIRS}
                    ${vtkPVServerManagerApplication_INCLUDE_DIRS})

set(STATIC_LINK_PLUGINS)
if (NOT BUILD_SHARED_LIBS)
  set(STATIC_LINK_PLUGINS vtkPVStaticPluginsInit)
endif()
if (PARAVIEW_USE_STATIC_NUMPY)
  find_library(UTIL_LIB util)

  set(extra_python_libs)
  foreach (lib "${UTIL_LIB}")
    if (lib)
      list(APPEND extra_python_libs
        "${lib}")
    endif ()
  endforeach ()

  find_library(BLAS_LIB blas)
  find_library(GSL_LIB gsl)
  find_library(GSLCBLAS_LIB gslcblas)
  find_library(LAPACK_LIB lapack)

  set(extra_numpy_libs)
  foreach (libvar BLAS_LIB GSL_LIB GSLCBLAS_LIB LAPACK_LIB)
    if (${libvar})
      list(APPEND extra_numpy_libs
        "${${libvar}}")
      add_definitions(-DHAVE_NUMPY_${libvar})
    endif ()
  endforeach ()
  add_definitions(-DHAVE_NUMPY_STATIC)

  list(APPEND STATIC_LINK_PLUGINS
    ${extra_python_libs} "${PYTHON_LIBRARY}"
    ${extra_numpy_libs} "${STATIC_NUMPY_LIBRARY}")
endif()


# Cray ATP support
set(PARAVIEW_USE_ATP OFF CACHE BOOL
  "Link to Cray ATP library to enable debug features on Cray Systems")
mark_as_advanced(PARAVIEW_USE_ATP)
if (PARAVIEW_USE_ATP)
  find_package(ATP)
endif()
foreach (name pvserver pvdataserver pvrenderserver)
  pv_add_executable(${name} ${name}.cxx)
  target_link_libraries(${name}
    vtkPVServerManagerApplication
    ${STATIC_LINK_PLUGINS})
  if (ATP_FOUND)
    target_link_libraries(${name} ${ATP_LIBRARIES})
    set_target_properties(${name} PROPERTIES LINK_FLAGS "${ATP_LINK_FLAGS}")
  endif()
endforeach()

if (PARAVIEW_ENABLE_PYTHON)
  foreach (name pvbatch pvpython)
    pv_add_executable(${name} ${name}.cxx)
    target_link_libraries(${name}
      vtkPVServerManagerApplication
      vtkUtilitiesPythonInitializer
      ${VTK_PYTHON_LIBRARIES}
      ${STATIC_LINK_PLUGINS})
    if (ATP_FOUND)
      target_link_libraries(${name} ${ATP_LIBRARIES})
      set_target_properties(${name} PROPERTIES LINK_FLAGS "${ATP_LINK_FLAGS}")
    endif()
  endforeach()

  # add python module dependencies to the server-executables for static builds.
  foreach (name pvserver pvdataserver pvrenderserver)
    target_link_libraries(${name}
        vtkUtilitiesPythonInitializer)
  endforeach()

endif()
