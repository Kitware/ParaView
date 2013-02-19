#------------------------------------------------------------------------------
# This file is based on vtkModuleTop.cmake. The role of this is to kick start
# the processing of all modules.
#
# When using internal-VTK, this simply adds the VTK subdir and lets VTK do it's
# thing after having ensured that VTK can find and detect the additional modules
# ParaView adds.
#
# When using external-VTK, this behaves a lot like vtkModuleTop.cmake,
# processing the modules that ParaView adds, ensuring we use modules from
# external VTK as applicable.
#------------------------------------------------------------------------------

# Figure out which languages are being wrapped, and add them to the list.
if(BUILD_TESTING)
  set(_test_languages "Cxx")
  if(PARAVIEW_ENABLE_PYTHON)
    list(APPEND _test_languages "Python")
  endif()
  if(VTK_WRAP_JAVA)
    list(APPEND _test_languages "Java")
  endif()
else()
  set(_test_languages "")
endif()

#----------------------------------------------------------------------
# Load the module DAG.

# Assess modules, and tests in the repository.
foreach (source_dir IN LISTS PARAVIEW_MODULE_ROOTS)
  file(GLOB_RECURSE files RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${source_dir}/module.cmake")
  foreach (module_cmake IN LISTS files)
    get_filename_component(base "${module_cmake}" PATH)
    if (PARAVIEW_USING_EXTERNAL_VTK)
      vtk_add_module(
        "${CMAKE_CURRENT_SOURCE_DIR}/${base}"
        module.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/${base}"
        ${_test_languages})
    else()
      # Simply add to module-search paths for VTK and let VTK deal with it.
      vtk_add_to_module_search_path(
        "${CMAKE_CURRENT_SOURCE_DIR}/${base}"
        "${CMAKE_CURRENT_BINARY_DIR}/${base}")
    endif()
  endforeach()
endforeach()

if (NOT PARAVIEW_USING_EXTERNAL_VTK)
  # include VTK
  set (old_build_examples ${BUILD_EXAMPLES})
  set (BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
  add_subdirectory(VTK)
  set (BUILD_EXAMPLES ${old_build_examples} CACHE BOOL "" FORCE)
  include(${ParaView_BINARY_DIR}/VTK/VTKConfig.cmake)
  return()
endif()

include(vtkGroups)

# Validate the module DAG.
macro(vtk_module_check vtk-module _needed_by stack)
  list(FIND VTK_MODULES_ENABLED ${vtk-module} _found)
  if (_found EQUAL -1)
    if(NOT ${vtk-module}_DECLARED)
      # Check if the module has been imported.
      #message(FATAL_ERROR "No such module \"${vtk-module}\" needed by \"${_needed_by}\"")
      set(check_finished_${vtk-module} 1)
    endif()
    if(check_started_${vtk-module} AND NOT check_finished_${vtk-module})
      # We reached a module while traversing its own dependencies recursively.
      set(msg "")
      foreach(entry ${stack})
        set(msg " ${entry} =>${msg}")
        if("${entry}" STREQUAL "${vtk-module}")
          break()
        endif()
      endforeach()
      message(FATAL_ERROR "Module dependency cycle detected:\n ${msg} ${vtk-module}")
    elseif(NOT check_started_${vtk-module})
      # Traverse dependencies of this module.  Mark the start and finish.
      set(check_started_${vtk-module} 1)
      foreach(dep IN LISTS ${vtk-module}_DEPENDS)
        vtk_module_check(${dep} ${vtk-module} "${vtk-module};${stack}")
      endforeach()
      set(check_finished_${vtk-module} 1)
    endif()
  endif()
endmacro()

foreach(vtk-module ${VTK_MODULES_ALL})
  vtk_module_check("${vtk-module}" "" "")
endforeach()

#----------------------------------------------------------------------
# Provide an option for each module.
foreach(vtk-module ${VTK_MODULES_ALL})
  if(NOT ${vtk-module}_IS_TEST)
    option(Module_${vtk-module} "Request building ${vtk-module}"
      ${${vtk-module}_DEFAULT})
    mark_as_advanced(Module_${vtk-module})
    if(${vtk-module}_EXCLUDE_FROM_ALL)
      set(${vtk-module}_IN_ALL 0)
    else()
      set(${vtk-module}_IN_ALL ${VTK_BUILD_ALL_MODULES})
    endif()
  endif()
endforeach()

# Follow dependencies.
macro(vtk_module_enable vtk-module _needed_by)
  if(NOT Module_${vtk-module})
    list(APPEND ${vtk-module}_NEEDED_BY ${_needed_by})
  endif()
  if(NOT ${vtk-module}_ENABLED)
    set(${vtk-module}_ENABLED 1)
    foreach(dep IN LISTS ${vtk-module}_DEPENDS)
      vtk_module_enable(${dep} ${vtk-module})
    endforeach()

    # If VTK_BUILD_ALL_MODULES_FOR_TESTS is true, then and then
    # alone do we include the test modules in building build the dependency
    # graph for enabled modules (BUG #13297).
    if (VTK_BUILD_ALL_MODULES_FOR_TESTS)
      foreach(test IN LISTS ${vtk-module}_TESTED_BY)
        vtk_module_enable(${test} "")
      endforeach()
    elseif (BUILD_TESTING)
      # identify vtkTesting<> dependencies on the test module and enable them.
      # this ensures that core testing modules such as vtkTestingCore,
      # vtkTestingRendering which many test modules depend on, are automatically
      # enabled.
      foreach(test IN LISTS ${vtk-module}_TESTED_BY)
        foreach(test-depends IN LISTS ${test}_DEPENDS)
          if (test-depends MATCHES "^vtkTesting.*")
            vtk_module_enable(${test-depends} "")
          endif()
        endforeach()
      endforeach()
    endif()
  endif()
endmacro()

foreach(vtk-module ${VTK_MODULES_ALL})
  if(Module_${vtk-module} OR ${vtk-module}_IN_ALL)
    vtk_module_enable("${vtk-module}" "")
  elseif(${vtk-module}_REQUEST_BY)
    vtk_module_enable("${vtk-module}" "${${vtk-module}_REQUEST_BY}")
  endif()
endforeach()

foreach(vtk-module ${VTK_MODULES_ALL})
  # Exclude modules that exist only to test this module
  # from the report of modules that need this one.  They
  # are enabled exactly because this module is enabled.
  if(${vtk-module}_NEEDED_BY AND ${vtk-module}_TESTED_BY)
    list(REMOVE_ITEM ${vtk-module}_NEEDED_BY "${${vtk-module}_TESTED_BY}")
  endif()
endforeach()

# Build final list of enabled modules.
set(PV_MODULES_ENABLED "")
set(PV_MODULES_DISABLED "")
foreach(vtk-module ${VTK_MODULES_ALL})
  if(${vtk-module}_ENABLED)
    list(APPEND PV_MODULES_ENABLED ${vtk-module})
  else()
    list(APPEND PV_MODULES_DISABLED ${vtk-module})
  endif()
endforeach()

if (NOT PV_MODULES_ENABLED)
  message(WARNING "No modules enabled!")
  return()
endif()

list(SORT PV_MODULES_ENABLED) # Deterministic order.
list(SORT PV_MODULES_DISABLED) # Deterministic order.


# Order list to satisfy dependencies.
include(TopologicalSort)
topological_sort(PV_MODULES_ENABLED "" _DEPENDS)

# topological_sort() ends up bringing in VTK modules that were imported, so we
# explicitly remove them before proceeding further.

if (VTK_MODULES_ENABLED)
  list(REMOVE_ITEM PV_MODULES_ENABLED ${VTK_MODULES_ENABLED})
  list(REMOVE_ITEM PV_MODULES_DISABLED ${VTK_MODULES_ENABLED})
endif()

# Report what will be built.
set(_modules_enabled_alpha "${PV_MODULES_ENABLED}")
list(SORT _modules_enabled_alpha)
list(REMOVE_ITEM _modules_enabled_alpha vtkWrappingJava vtkWrappingPython)
list(LENGTH _modules_enabled_alpha _length)
message(STATUS "Enabled ${_length} modules:")
foreach(vtk-module ${_modules_enabled_alpha})
  if(NOT ${vtk-module}_IS_TEST)
    if(Module_${vtk-module})
      set(_reason ", requested by Module_${vtk-module}.")
    elseif(${vtk-module}_IN_ALL)
      set(_reason ", requested by VTK_BUILD_ALL_MODULES.")
    else()
      set(_needed_by "${${vtk-module}_NEEDED_BY}")
      list(SORT _needed_by)
      list(LENGTH _needed_by _length)
      if(_length GREATER "1")
        set(_reason ", needed by ${_length} modules:")
        foreach(dep ${_needed_by})
          set(_reason "${_reason}\n        ${dep}")
        endforeach()
      else()
        set(_reason ", needed by ${_needed_by}.")
      endif()
    endif()
    message(STATUS " * ${vtk-module}${_reason}")
  endif()
endforeach()

# Hide options for modules that will build anyway.
foreach(vtk-module ${VTK_MODULES_ALL})
  if(NOT ${vtk-module}_IS_TEST)
    if(${vtk-module}_IN_ALL OR ${vtk-module}_NEEDED_BY)
      set_property(CACHE Module_${vtk-module} PROPERTY TYPE INTERNAL)
    else()
      set_property(CACHE Module_${vtk-module} PROPERTY TYPE BOOL)
    endif()
  endif()
endforeach()

if(NOT PV_MODULES_ENABLED)
  message(WARNING "No modules enabled!")
  return()
endif()

macro(verify_vtk_module_is_set)
  if("" STREQUAL "${vtk-module}")
    message(FATAL_ERROR "CMake variable vtk-module is not set")
  endif()
endmacro()

macro(init_module_vars)
  verify_vtk_module_is_set()
# set(${vtk-module}-targets VTKTargets)
# set(${vtk-module}-targets-install "${VTK_INSTALL_PACKAGE_DIR}/VTKTargets.cmake")
# set(${vtk-module}-targets-build "${VTK_BINARY_DIR}/VTKTargets.cmake")
endmacro()

# VTK_WRAP_PYTHON_MODULES can be used to explicitly control which modules
# get Python wrapped (no automatic dependency support is provided at this
# time). If it has been set mark the modules in the list as such.
# Note any wrap exclude entries in the module.cmake will take precedence
# If entry has not been set default to PV_MODULES_ENABLED.
if(VTK_WRAP_PYTHON)
  if(NOT VTK_WRAP_PYTHON_MODULES)
    set(VTK_WRAP_PYTHON_MODULES ${PV_MODULES_ENABLED})
  endif()
  foreach(_wrap_module ${VTK_WRAP_PYTHON_MODULES})
    if(NOT ${_wrap_module}_EXCLUDE_FROM_WRAPPING)
      set(${_wrap_module}_WRAP_PYTHON ON)
    endif()
  endforeach()
endif()

# Build all modules.
set (VTK_MODULES_ENABLED ${VTK_MODULES_ENABLED} ${PV_MODULES_ENABLED})
foreach(vtk-module ${PV_MODULES_ENABLED})

  set(_module ${vtk-module})

  if(NOT ${_module}_IS_TEST)
    init_module_vars()
  else()
    set(vtk-module ${${_module}_TESTS_FOR})
  endif()

  #message("vtk-module = ${vtk-module}")
  include("${${_module}_SOURCE_DIR}/vtk-module-init.cmake" OPTIONAL)
  add_subdirectory("${${_module}_SOURCE_DIR}" "${${_module}_BINARY_DIR}")
endforeach()

vtk_module_config(VTK ${VTK_MODULES_ENABLED})
