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
# This file is similar to vtkModuleMacros.cmake in role. It adds the pv_plugin
# macro used to add plugins (similar to modules for VTK) for ParaView. Plugins
# are expected to include a plugin.cmake file (analogous to module.cmake) file
# which includes the call to pv_plugin() macro.

# Now, to build a plugin to be incorporated into ParaView one puts a
# plugin.cmake file in that directory containing the CMakeLists.txt file to
# create the plugin. At compile time, ParaView searches for plugin.cmake files
# in the ParaView/Plugins root or under locations added explicitly using
# pv_add_plugin_search_path(). The plugin.cmake file should have at most a single call
# to pv_plugin() (the file can skip calling the pv_plugin() macro if decided so
# based on certain cmake variables). This call provides information about the
# plugin to paraview even before trying to process it.
# Then the CMakeLists.txt file has the pertinant code to build the plugin. The
# CMakeLists.txt file will be processed automaticall if the plugin is enabled.
# pv_plugin() macro provides options that can be used to control if the plugin
# is enabled by default, or autoloaded or is named differently (or comprises of multiple
# plugins -- which is rare).

get_filename_component(_ParaViewPluginsMacros_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include (ParaViewPlugins)
include (CMakeParseArguments)

#------------------------------------------------------------------------------
# USAGE:
#   pv_plugin(<NAME>
#             # provide a descrption for the plugin.
#             [DESCRIPTION <text>]
#
#             # if provided, the plugin will be autoloaded on startup.
#             [AUTOLOAD]
#
#             # if provided, the plugin will be auto-enabled for building.
#             # otherwise user has to turn it ON an CMake configure time.
#             [DEFAULT_ENABLED]
#
#             # <NAME> is generally used as the name of the plugin used for the
#             # single call to add_paraview_plugin(). If a different name is
#             # used, or more that 1 plugin is added in then all the plugin
#             # names must be specified using this argument.
#             [PLUGIN_NAMES <name1> <name2>]
# )
macro(pv_plugin _name)
  vtk_module_check_name(${_name})
  set(pv-plugin ${_name})
  set(${pv-plugin}_DESCRIPTION "(plugin)")
  set(${pv-plugin}_AUTOLOAD FALSE)
  set(${pv-plugin}_DEFAULT_ENABLED FALSE)
  set(${pv-plugin}_PLUGIN_NAMES "${_name}")

  cmake_parse_arguments("${pv-plugin}"
      "AUTOLOAD;DEFAULT_ENABLED" # options
      "DESCRIPTION" # one_value_keywords
      "PLUGIN_NAMES" # multi_value_keywords
      ${ARGN})

  # set default values.
  if (NOT ${pv-plugin}_PLUGIN_NAMES)
    set(${pv-plugin}_PLUGIN_NAMES "${_name}")
  endif()

  if (${pv-plugin}_UNPARSED_ARGUMENTS)
    message(AUTHOR_WARNING
      "Unrecognized arguments: ${${pv-plugin}_UNPARSED_ARGUMENTS}")
  endif()
endmacro()


#------------------------------------------------------------------------------
macro(pv_plugin_impl)
  include(plugin.cmake)
endmacro()

#------------------------------------------------------------------------------
macro(pv_add_plugin src f bld)
  unset(pv-plugin)
  include (${src}/${f} OPTIONAL)
  if (DEFINED pv-plugin)
    list (APPEND PARAVIEW_PLUGINS_ALL ${pv-plugin})
    get_filename_component(${pv-plugin}_BASE ${f} PATH)
    set (${pv-plugin}_SOURCE_DIR ${src}/${${pv-plugin}_BASE})
    set (${pv-plugin}_BINARY_DIR ${bld}/${${pv-plugin}_BASE})
  endif()
  unset(pv-plugin)
endmacro()

#------------------------------------------------------------------------------
# Use this macro to add paths to search for plugin.cmake in.
macro(pv_add_plugin_search_path src bld)
  list (APPEND pv_plugin_search_path "${src},${bld}")
endmacro()

#------------------------------------------------------------------------------
# Call this macro to load information about available plugins from all
# pv_plugin_search_path locations.
macro(pv_plugin_search_from_paths)
  foreach (pair IN LISTS pv_plugin_search_path)
    string(REGEX MATCH "^([^,]*),([^,]*)$" _tmp "${pair}")
    set (src "${CMAKE_MATCH_1}")
    set (bld "${CMAKE_MATCH_2}")
    pv_add_plugin("${src}" plugin.cmake "${bld}")
  endforeach()
endmacro()

macro(pv_plugin_search_under_root root_src root_build)
  file(GLOB_RECURSE files RELATIVE "${root_src}" "${root_src}/plugin.cmake")
  foreach(plugin_file IN LISTS files)
    get_filename_component(base "${plugin_file}" PATH)
    if(base)
      pv_add_plugin("${root_src}/${base}" plugin.cmake "${root_build}/${base}")
    else()
      # The root directory is in fact the plugin directory
      pv_add_plugin("${root_src}" plugin.cmake "${root_build}")
    endif()
  endforeach()
endmacro()

set(PARAVIEW_EXTRA_EXTERNAL_PLUGINS ""
  CACHE STRING "A list of plugins to autoload (but not built as part of ParaView itself)")
mark_as_advanced(PARAVIEW_EXTRA_EXTERNAL_PLUGINS)

#------------------------------------------------------------------------------
# pv_process_plugins() serves as the entry point for processing plugins. A
# top-level cmake file simply sets the plugin paths to search using
# pv_add_plugin_search_path() and then call this macro and this will handle
# searching for the plugins available, providing options for enabling them and
# then processing the cmake files for enabled plugins.
macro(pv_process_plugins root_src root_build)
  set(PARAVIEW_EXTERNAL_PLUGIN_DIRS "" CACHE STRING
      "Semi-colon seperated paths to extrenal plugin directories.")
  mark_as_advanced(PARAVIEW_EXTERNAL_PLUGIN_DIRS)

  set (PARAVIEW_PLUGINS_ALL)
  pv_plugin_search_under_root(${root_src} ${root_build})

  # search for available plugins and load information about them.
  pv_plugin_search_from_paths()

  # if PARAVIEW_EXTERNAL_PLUGIN_DIRS is specified, we scan those directories
  # too.
  if (PARAVIEW_EXTERNAL_PLUGIN_DIRS)
    foreach(extra_dir ${PARAVIEW_EXTERNAL_PLUGIN_DIRS})
      get_filename_component(dir_name "${extra_dir}" NAME)
      message(STATUS "Processing external plugins under '${extra_dir}'")
      set (build_dir "${ParaView_BINARY_DIR}/ExternalPlugins/${dir_name}")
      pv_plugin_search_under_root("${extra_dir}" "${build_dir}")
    endforeach()
  endif()

  # provide options that user can use to enable/disable plugins.
  foreach (pv-plugin IN LISTS PARAVIEW_PLUGINS_ALL)
    set(PARAVIEW_BUILD_PLUGIN_${pv-plugin} ${${pv-plugin}_DEFAULT_ENABLED}
      CACHE BOOL "Build ${pv-plugin} Plugin")
    mark_as_advanced(PARAVIEW_BUILD_PLUGIN_${pv-plugin})
    cmake_dependent_option(PARAVIEW_AUTOLOAD_PLUGIN_${pv-plugin}
      "Load ${pv-plugin} Plugin Automatically"
      ${${pv-plugin}_AUTOLOAD} "PARAVIEW_BUILD_PLUGIN_${pv-plugin}" OFF)
    mark_as_advanced(PARAVIEW_AUTOLOAD_PLUGIN_${pv-plugin})
  endforeach()

  # process all enabled plugins.
  foreach (pv-plugin IN LISTS PARAVIEW_PLUGINS_ALL)
    if (PARAVIEW_BUILD_PLUGIN_${pv-plugin})
      message(STATUS "Plugin: ${pv-plugin} - ${${pv-plugin}_DESCRIPTION} : Enabled")
      add_subdirectory("${${pv-plugin}_SOURCE_DIR}" "${${pv-plugin}_BINARY_DIR}")
      # The plugin's CMakeLists.txt might not add targets for all the
      # plugin-names that it said it will generate based on CMake settings e.g.
      # GUI is not being built. To handle that case, we prune the plugin-names
      # list to include only the valid ones.
      set (real_plugin_names)
      foreach (libname IN LISTS ${pv-plugin}_PLUGIN_NAMES)
        if(TARGET ${libname})
          list(APPEND real_plugin_names ${libname})
        else()
          message(STATUS "Plugin '${pv-plugin}' lists plugin library named "
            "'${libname}', which isn't being built. So skipping it.")
        endif()
      endforeach()
      set (${pv-plugin}_PLUGIN_NAMES ${real_plugin_names})
    else()
      message(STATUS "Plugin: ${pv-plugin} - ${${pv-plugin}_DESCRIPTION} : Disabled")
    endif()
  endforeach()

  foreach (pv-plugin IN LISTS PARAVIEW_EXTRA_EXTERNAL_PLUGINS)
    list(APPEND PARAVIEW_PLUGINS_ALL
      "${pv-plugin}")
    # Assume the plugin will exist.
    set(PARAVIEW_BUILD_PLUGIN_${pv-plugin} TRUE)
    set(${pv-plugin}_PLUGIN_NAMES "${pv-plugin}")
    # Assume the plugin is to be autoloaded.
    set(PARAVIEW_AUTOLOAD_PLUGIN_${pv-plugin} TRUE)
  endforeach ()
 
  # write the .plugins file.
  set (PARAVIEW_PLUGINLIST)
  set (plugin_ini "<?xml version=\"1.0\"?>\n<Plugins>\n")
  foreach (pv-plugin IN LISTS PARAVIEW_PLUGINS_ALL)
    if (PARAVIEW_BUILD_PLUGIN_${pv-plugin})
      # fill up the PARAVIEW_PLUGINLIST with the names of the libraries for all
      # the enabled plugins.
      list(APPEND PARAVIEW_PLUGINLIST ${${pv-plugin}_PLUGIN_NAMES})
      foreach (libname IN LISTS ${pv-plugin}_PLUGIN_NAMES)
        if (PARAVIEW_AUTOLOAD_PLUGIN_${pv-plugin})
          set(plugin_ini
            "${plugin_ini}  <Plugin name=\"${libname}\" auto_load=\"1\"/>\n")
        else ()
          set(plugin_ini
            "${plugin_ini}  <Plugin name=\"${libname}\" auto_load=\"0\"/>\n")
        endif()
      endforeach()
    endif()
  endforeach()
  set (plugin_ini "${plugin_ini}</Plugins>\n")
  file(WRITE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.plugins" "${plugin_ini}")

  # Install the .plugins configuration file.
  install(FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.plugins"
          DESTINATION "${PV_INSTALL_PLUGIN_DIR}"
          COMPONENT Runtime)

  if (NOT BUILD_SHARED_LIBS)
    # write the static plugins init file.
    _write_static_plugins_init_file(
      ${CMAKE_CURRENT_BINARY_DIR}/pvStaticPluginsInit.h
      ${CMAKE_CURRENT_BINARY_DIR}/pvStaticPluginsInit.cxx
      ${PARAVIEW_PLUGINLIST})

    vtk_module_dep_includes(vtkPVClientServerCoreCore)
    include_directories(${vtkPVClientServerCoreCore_INCLUDE_DIRS} ${vtkPVClientServerCoreCore_DEPENDS_INCLUDE_DIRS})
    add_library(vtkPVStaticPluginsInit
      ${CMAKE_CURRENT_BINARY_DIR}/pvStaticPluginsInit.cxx)
    target_link_libraries(vtkPVStaticPluginsInit
      LINK_PRIVATE ${PARAVIEW_PLUGINLIST})
  endif ()
endmacro()

#------------------------------------------------------------------------------
# Internal function used to generate a header file initializing all plugins that
# can be used by executables to link against the plugins when building
# statically.
function(_write_static_plugins_init_file header source)
  file(WRITE "${header}" "void paraview_static_plugins_init();\n")

  set(plugins_init_function "#include \"vtkPVPlugin.h\"\n")
  set(plugins_init_function "${plugins_init_function}#include \"vtkPVPluginLoader.h\"\n\n")
  set(plugins_init_function "${plugins_init_function}#include \"vtkPVPluginTracker.h\"\n\n")
  set(plugins_init_function "${plugins_init_function}#include <string>\n\n")

  # write PV_PLUGIN_IMPORT_INIT calls
  foreach(plugin_name ${ARGN})
    set(plugins_init_function "${plugins_init_function}PV_PLUGIN_IMPORT_INIT(${plugin_name});\n")
  endforeach()
  set(plugins_init_function "${plugins_init_function}\n")

  set(plugins_init_function "${plugins_init_function}static bool paraview_static_plugins_load(const char* name);\n\n")
  set(plugins_init_function "${plugins_init_function}static bool paraview_static_plugins_search(const char* name);\n\n")
  set(plugins_init_function "${plugins_init_function}void paraview_static_plugins_init()\n{\n")
  set(plugins_init_function "${plugins_init_function}  vtkPVPluginLoader::SetStaticPluginLoadFunction(paraview_static_plugins_load);\n")
  set(plugins_init_function "${plugins_init_function}  vtkPVPluginTracker::SetStaticPluginSearchFunction(paraview_static_plugins_search);\n")
  set(plugins_init_function "${plugins_init_function}}\n\n")

  # write callback functions
  set(plugins_init_function "${plugins_init_function}static bool paraview_static_plugins_func(const char* name, bool load);\n\n")
  set(plugins_init_function "${plugins_init_function}static bool paraview_static_plugins_load(const char* name)\n{\n")
  set(plugins_init_function "${plugins_init_function}  return paraview_static_plugins_func(name, true);\n")
  set(plugins_init_function "${plugins_init_function}}\n\n")
  set(plugins_init_function "${plugins_init_function}static bool paraview_static_plugins_search(const char* name)\n{\n")
  set(plugins_init_function "${plugins_init_function}  return paraview_static_plugins_func(name, false);\n")
  set(plugins_init_function "${plugins_init_function}}\n\n")

  # write PV_PLUGIN_IMPORT calls
  set(plugins_init_function "${plugins_init_function}static bool paraview_static_plugins_func(const char* name, bool load)\n{\n")
  set(plugins_init_function "${plugins_init_function}  std::string sname = name;\n\n")
  foreach(plugin_name ${ARGN})
    set(plugins_init_function "${plugins_init_function}  if (sname == \"${plugin_name}\")\n")
    set(plugins_init_function "${plugins_init_function}    {\n")
    set(plugins_init_function "${plugins_init_function}    if (load)\n")
    set(plugins_init_function "${plugins_init_function}      {\n")
    set(plugins_init_function "${plugins_init_function}      static bool loaded = false;\n")
    set(plugins_init_function "${plugins_init_function}      if (!loaded)\n")
    set(plugins_init_function "${plugins_init_function}        {\n")
    set(plugins_init_function "${plugins_init_function}        PV_PLUGIN_IMPORT(${plugin_name});\n")
    set(plugins_init_function "${plugins_init_function}        loaded = true;\n")
    set(plugins_init_function "${plugins_init_function}        }\n")
    set(plugins_init_function "${plugins_init_function}      }\n")
    set(plugins_init_function "${plugins_init_function}    return true;\n")
    set(plugins_init_function "${plugins_init_function}    }\n")
  endforeach()
  set(plugins_init_function "${plugins_init_function}  return false;\n")
  set(plugins_init_function "${plugins_init_function}}\n")

  file(WRITE "${source}" "${plugins_init_function}")
endfunction()
