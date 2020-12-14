set(_ParaViewPlugin_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

#[==[.md
# ParaView Plugin CMake API

# TODO

#]==]

#[==[.md
## Finding plugins

Similar to VTK modules, plugins first have to be discovered. The
`paraview_plugin_find_plugins` function does this. The output variable is the
list of discovered `paraview.plugin` files.

```
paraview_plugin_find_plugins(<output> [<directory>...])
```
#]==]
function (paraview_plugin_find_plugins output)
  set(_paraview_find_plugins_all)
  foreach (_paraview_find_plugins_directory IN LISTS ARGN)
    file(GLOB_RECURSE _paraview_find_plugins_plugins
      "${_paraview_find_plugins_directory}/paraview.plugin")
    list(APPEND _paraview_find_plugins_all
      ${_paraview_find_plugins_plugins})
  endforeach ()
  set("${output}" ${_paraview_find_plugins_all} PARENT_SCOPE)
endfunction ()

#[==[.md
## Plugin files

The `paraview.plugin` file is parsed and used as arguments to a CMake function.

Example:

```
NAME
  AdiosReaderPixie
CONDITION
  PARAVIEW_USE_MPI
DESCRIPTION
  Pixie file reader using ADIOS
REQUIRES_MODULES
  VTK:CommonCore
```

The supported arguments are:

  * `NAME`: (Required) The name of the plugin.
  * `DESCRIPTION`: (Recommended) Short text describing what the plugin does.
  * `CONDITION`: Arguments to CMake's `if` command which may be used to hide
    the plugin for certain platforms or other reasons. If the expression is
    false, the module is completely ignored.
  * `REQUIRES_MODULES`: If the plugin is enabled, these modules will be listed
    as those required to build the enabled plugins.
#]==]
macro (_paraview_plugin_parse_args name_output)
  cmake_parse_arguments("_name"
    ""
    "NAME"
    ""
    ${ARGN})

  if (NOT _name_NAME)
    message(FATAL_ERROR
      "A ParaView plugin requires a name (from ${_paraview_scan_plugin_file}).")
  endif ()
  set("${name_output}" "${_name_NAME}")

  cmake_parse_arguments("${_name_NAME}"
    ""
    "NAME"
    "DESCRIPTION;REQUIRES_MODULES;CONDITION"
    ${ARGN})

  if (${_name_NAME}_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for ${_name_NAME}: "
      "${${_name_NAME}_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT ${_name_NAME}_DESCRIPTION)
    message(WARNING "The ${_name_NAME} module should have a description")
  endif ()
  string(REPLACE ";" " " "${_name_NAME}_DESCRIPTION" "${${_name_NAME}_DESCRIPTION}")
endmacro ()

#[==[.md
## Scanning plugins

Once the `paraview.plugin` files have been found, they need to be scanned to
determine which should be built. Generally, plugins should be scanned first in
order to use the `REQUIRES_MODULES` list to enable them during the scan for
their required modules.

```
paraview_plugin_scan(
  PLUGIN_FILES              <file>...
  PROVIDES_PLUGINS          <variable>
  [ENABLE_BY_DEFAULT        <ON|OFF>]
  [HIDE_PLUGINS_FROM_CACHE  <ON|OFF>]
  [REQUIRES_MODULES         <module>...])
```

  * `PLUGIN_FILES`: (Required) The list of plugin files to scan.
  * `PROVIDES_PLUGINS`: (Required) This variable contains a list of the plugins
    to be built.
  * `ENABLE_BY_DEFAULT`: (Defaults to `OFF`) Whether to enable plugins by
    default or not.
  * `HIDE_PLUGINS_FROM_CACHE`: (Defaults to `OFF`) Whether to display options
    to enable and disable plugins in the cache or not.
  * `REQUIRES_MODULES`: The list of modules required by the enabled plugins.
#]==]

function (paraview_plugin_scan)
  cmake_parse_arguments(_paraview_scan
    ""
    "ENABLE_BY_DEFAULT;HIDE_PLUGINS_FROM_CACHE;REQUIRES_MODULES;PROVIDES_PLUGINS"
    "PLUGIN_FILES"
    ${ARGN})

  if (_paraview_scan_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_scan: "
      "${_paraview_scan_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_scan_ENABLE_BY_DEFAULT)
    set(_paraview_scan_ENABLE_BY_DEFAULT OFF)
  endif ()

  if (NOT DEFINED _paraview_scan_HIDE_PLUGINS_FROM_CACHE)
    set(_paraview_scan_HIDE_PLUGINS_FROM_CACHE OFF)
  endif ()

  if (NOT DEFINED _paraview_scan_PROVIDES_PLUGINS)
    message(FATAL_ERROR
      "The `PROVIDES_PLUGINS` argument is required.")
  endif ()

  if (NOT _paraview_scan_PLUGIN_FILES)
    message(FATAL_ERROR
      "No plugin files given to scan.")
  endif ()

  set(_paraview_scan_option_default_type BOOL)
  if (_paraview_scan_HIDE_PLUGINS_FROM_CACHE)
    set(_paraview_scan_option_default_type INTERNAL)
  endif ()

  set(_paraview_scan_provided_plugins)
  set(_paraview_scan_required_modules)

  foreach (_paraview_scan_plugin_file IN LISTS _paraview_scan_PLUGIN_FILES)
    if (NOT IS_ABSOLUTE "${_paraview_scan_plugin_file}")
      set(_paraview_scan_plugin_file
        "${CMAKE_CURRENT_SOURCE_DIR}/${_paraview_scan_plugin_file}")
    endif ()

    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND
      PROPERTY
        CMAKE_CONFIGURE_DEPENDS "${_paraview_scan_plugin_file}")

    file(READ "${_paraview_scan_plugin_file}" _paraview_scan_plugin_args)
    # Replace comments.
    string(REGEX REPLACE "#[^\n]*\n" "\n" _paraview_scan_plugin_args "${_paraview_scan_plugin_args}")
    # Use argument splitting.
    string(REGEX REPLACE "( |\n)+" ";" _paraview_scan_plugin_args "${_paraview_scan_plugin_args}")
    _paraview_plugin_parse_args(_paraview_scan_plugin_name ${_paraview_scan_plugin_args})

    list(APPEND _paraview_scan_all_plugins
      "${_paraview_scan_plugin_name}")

    set(_paraview_scan_plugin_default "${_paraview_scan_ENABLE_BY_DEFAULT}")
    if (DEFINED "_paraview_plugin_default_${_paraview_scan_plugin_name}")
      set(_paraview_scan_plugin_default "${_paraview_plugin_default_${_paraview_scan_plugin_name}}")
    endif ()
    option("PARAVIEW_PLUGIN_ENABLE_${_paraview_scan_plugin_name}"
      "Enable the ${_paraview_scan_plugin_name} plugin. ${${_paraview_scan_plugin_name}_DESCRIPTION}"
      "${_paraview_scan_plugin_default}")
    set("_paraview_scan_enable_${_paraview_scan_plugin_name}"
      "${PARAVIEW_PLUGIN_ENABLE_${_paraview_scan_plugin_name}}")

    set_property(CACHE "PARAVIEW_PLUGIN_ENABLE_${_paraview_scan_plugin_name}"
      PROPERTY
        TYPE "${_paraview_scan_option_default_type}")

    if (DEFINED ${_paraview_scan_plugin_name}_CONDITION)
      if (NOT (${${_paraview_scan_plugin_name}_CONDITION}))
        if (DEFINED "PARAVIEW_PLUGIN_ENABLE_${_paraview_scan_plugin_name}")
          set_property(CACHE "PARAVIEW_PLUGIN_ENABLE_${_paraview_scan_plugin_name}"
            PROPERTY
              TYPE INTERNAL)
        endif ()
        continue ()
      endif ()
    endif ()

    if (_paraview_scan_enable_${_paraview_scan_plugin_name})
      list(APPEND _paraview_scan_provided_plugins
        "${_paraview_scan_plugin_name}")
      list(APPEND _paraview_scan_required_modules
        ${${_paraview_scan_plugin_name}_REQUIRES_MODULES})
    endif ()

    set_property(GLOBAL
      PROPERTY
        "_paraview_plugin_${_paraview_scan_plugin_name}_file" "${_paraview_scan_plugin_file}")
    set_property(GLOBAL
      PROPERTY
        "_paraview_plugin_${_paraview_scan_plugin_name}_description" "${${_paraview_scan_plugin_name}_DESCRIPTION}")
    set_property(GLOBAL
      PROPERTY
      "_paraview_plugin_${_paraview_scan_plugin_name}_required_modules" "${${_paraview_scan_plugin_name}_REQUIRES_MODULES}")
  endforeach ()

  if (DEFINED _paraview_scan_REQUIRES_MODULES)
    set("${_paraview_scan_REQUIRES_MODULES}"
      ${_paraview_scan_required_modules}
      PARENT_SCOPE)
  endif ()

  set("${_paraview_scan_PROVIDES_PLUGINS}"
    ${_paraview_scan_provided_plugins}
    PARENT_SCOPE)
endfunction ()

function (_paraview_plugin_check_destdir variable)
  if (NOT DEFINED "${variable}")
    message(FATAL_ERROR
      "It appears as though ${variable} is not defined, but is needed to "
      "default a destination directory for build artifacts. Usually this is "
      "resolved by `include(GNUInstallDirs)` at the top of the project.")
  endif ()
endfunction ()

#[==[.md
## Building plugins

Once all plugins have been scanned, they need to be built.

```
paraview_plugin_build(
  PLUGINS <plugin>...
  [AUTOLOAD <plugin>...]
  [PLUGINS_COMPONENT <component>]

  [TARGET <target>]
  [INSTALL_EXPORT <export>]
  [CMAKE_DESTINATION <destination>]
  [TARGET_COMPONENT <component>]
  [INSTALL_HEADERS <ON|OFF>]

  [HEADERS_DESTINATION <destination>]
  [RUNTIME_DESTINATION <destination>]
  [LIBRARY_DESTINATION <destination>]
  [LIBRARY_SUBDIRECTORY <subdirectory>]
  [ADD_INSTALL_RPATHS <ON|OFF>]

  [PLUGINS_FILE_NAME <filename>])
```

  * `PLUGINS`: (Required) The list of plugins to build. May be empty.
  * `AUTOLOAD`: A list of plugins to mark for autoloading.
  * `PLUGINS_COMPONENT`: (Defaults to `paraview_plugins`) The installation
    component to use for installed plugins.
  * `TARGET`: (Recommended) The name of an interface target to generate. This
    provides. an initialization function `<TARGET>_initialize` which
    initializes static plugins. The function is provided, but is a no-op for
    shared plugin builds.
  * `INSTALL_EXPORT`: If provided, the generated target will be added to the
    named export set.
  * `CMAKE_DESTINATION`: If provided, the plugin target's properties will be
    written to a file named `<TARGET>-paraview-plugin-properties.cmake` in the
    specified destination.
  * `TARGET_COMPONENT`: (Defaults to `development`) The component to use for
    `<TARGET>`.
  * `INSTALL_HEADERS`: (Defaults to `ON`) Whether to install headers or not.
  * `HEADERS_DESTINATION`: (Defaults to `${CMAKE_INSTALL_INCLUDEDIR}`) Where to
    install include files.
  * `RUNTIME_DESTINATION`: (Defaults to `${CMAKE_INSTALL_BINDIR}`) Where to
    install runtime files.
  * `LIBRARY_DESTINATION`: (Defaults to `${CMAKE_INSTALL_LIBDIR}`) Where to
    install modules built by plugins.
  * `LIBRARY_SUBDIRECTORY`: (Defaults to `""`) Where to install the plugins
    themselves. Each plugin lives in a directory of its name in
    `<RUNTIME_DESTINATION>/<LIBRARY_SUBDIRECTORY>` (for Windows) or
    `<LIBRARY_DESTINATION>/<LIBRARY_SUBDIRECTORY>` for other platforms.
  * `ADD_INSTALL_RPATHS`: (Defaults to `ON`) If specified, an RPATH to load
    dependent libraries from the `LIBRARY_DESTINATION` from the plugins will be
    added.
  * `PLUGINS_FILE_NAME`: The name of the XML plugin file to generate for the
    built plugins. This file will be placed under
    `<LIBRARY_DESTINATION>/<LIBRARY_SUBDIRECTORY>`. It will be installed with
    the `plugin` component.
#]==]
function (paraview_plugin_build)
  cmake_parse_arguments(_paraview_build
    ""
    "HEADERS_DESTINATION;RUNTIME_DESTINATION;LIBRARY_DESTINATION;LIBRARY_SUBDIRECTORY;TARGET;PLUGINS_FILE_NAME;INSTALL_EXPORT;CMAKE_DESTINATION;PLUGINS_COMPONENT;TARGET_COMPONENT;ADD_INSTALL_RPATHS;INSTALL_HEADERS"
    "PLUGINS;AUTOLOAD"
    ${ARGN})

  if (_paraview_build_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_build: "
      "${_paraview_build_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_build_HEADERS_DESTINATION)
    _paraview_plugin_check_destdir(CMAKE_INSTALL_INCLUDEDIR)
    set(_paraview_build_HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  endif ()

  if (NOT DEFINED _paraview_build_RUNTIME_DESTINATION)
    _paraview_plugin_check_destdir(CMAKE_INSTALL_BINDIR)
    set(_paraview_build_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif ()

  if (NOT DEFINED _paraview_build_LIBRARY_DESTINATION)
    _paraview_plugin_check_destdir(CMAKE_INSTALL_LIBDIR)
    set(_paraview_build_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  endif ()

  if (NOT DEFINED _paraview_build_LIBRARY_SUBDIRECTORY)
    set(_paraview_build_LIBRARY_SUBDIRECTORY "")
  endif ()

  if (NOT DEFINED _paraview_build_INSTALL_HEADERS)
    set(_paraview_build_INSTALL_HEADERS ON)
  endif ()

  if (NOT DEFINED _paraview_build_ADD_INSTALL_RPATHS)
    set(_paraview_build_ADD_INSTALL_RPATHS ON)
  endif ()
  if (_paraview_build_ADD_INSTALL_RPATHS)
    if (NOT _paraview_build_LIBRARY_SUBDIRECTORY STREQUAL "")
      file(RELATIVE_PATH _paraview_build_relpath
        "/prefix/${_paraview_build_LIBRARY_DESTINATION}/${_paraview_build_LIBRARY_SUBDIRECTORY}/plugin"
        "/prefix/${_paraview_build_LIBRARY_DESTINATION}")
    else ()
      file(RELATIVE_PATH _paraview_build_relpath
        "/prefix/${_paraview_build_LIBRARY_DESTINATION}/plugin"
        "/prefix/${_paraview_build_LIBRARY_DESTINATION}")
    endif ()
    if (APPLE)
      list(APPEND CMAKE_INSTALL_RPATH
        "@loader_path/${_paraview_build_relpath}")
    elseif (UNIX)
      list(APPEND CMAKE_INSTALL_RPATH
        "$ORIGIN/${_paraview_build_relpath}")
    endif ()
  endif ()

  if (DEFINED _paraview_build_INSTALL_EXPORT
      AND NOT DEFINED _paraview_build_TARGET)
    message(FATAL_ERROR
      "The `INSTALL_EXPORT` argument requires the `TARGET` argument.")
  endif ()

  if (DEFINED _paraview_build_INSTALL_EXPORT
      AND NOT DEFINED _paraview_build_CMAKE_DESTINATION)
    message(FATAL_ERROR
      "The `INSTALL_EXPORT` argument requires the `CMAKE_DESTINATION` argument.")
  endif ()

  set(_paraview_build_extra_destinations)
  if (DEFINED _paraview_build_CMAKE_DESTINATION)
    list(APPEND _paraview_build_extra_destinations
      CMAKE_DESTINATION)
    if (NOT DEFINED _paraview_build_TARGET)
      message(FATAL_ERROR
        "The `CMAKE_DESTINATION` argument requires the `TARGET` argument.")
    endif ()
  endif ()

  if (DEFINED _paraview_build_TARGET)
    _vtk_module_split_module_name("${_paraview_build_TARGET}" _paraview_build)
    string(REPLACE "::" "_" _paraview_build_target_safe "${_paraview_build_TARGET}")
  endif ()

  _vtk_module_check_destinations(_paraview_build_
    HEADERS_DESTINATION
    RUNTIME_DESTINATION
    LIBRARY_DESTINATION
    LIBRARY_SUBDIRECTORY
    ${_paraview_build_extra_destinations})

  if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_paraview_build_RUNTIME_DESTINATION}")
  endif ()
  if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_paraview_build_LIBRARY_DESTINATION}")
  endif ()
  if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_paraview_build_LIBRARY_DESTINATION}")
  endif ()

  if (WIN32)
    set(_paraview_build_plugin_destination "${_paraview_build_RUNTIME_DESTINATION}")
  else ()
    set(_paraview_build_plugin_destination "${_paraview_build_LIBRARY_DESTINATION}")
  endif ()
  if (NOT _paraview_build_LIBRARY_SUBDIRECTORY STREQUAL "")
    string(APPEND _paraview_build_plugin_destination "/${_paraview_build_LIBRARY_SUBDIRECTORY}")
  endif ()

  foreach (_paraview_build_plugin IN LISTS _paraview_build_PLUGINS)
    get_property(_paraview_build_plugin_file GLOBAL
      PROPERTY  "_paraview_plugin_${_paraview_build_plugin}_file")
    if (NOT _paraview_build_plugin_file)
      message(FATAL_ERROR
        "The requested ${_paraview_build_plugin} plugin is not a ParaView plugin.")
    endif ()

    # TODO: Support external plugins?
    get_filename_component(_paraview_build_plugin_dir "${_paraview_build_plugin_file}" DIRECTORY)
    file(RELATIVE_PATH _paraview_build_plugin_subdir "${CMAKE_SOURCE_DIR}" "${_paraview_build_plugin_dir}")
    add_subdirectory(
      "${CMAKE_SOURCE_DIR}/${_paraview_build_plugin_subdir}"
      "${CMAKE_BINARY_DIR}/${_paraview_build_plugin_subdir}")
  endforeach ()

  if (DEFINED _paraview_build_TARGET)
    add_library("${_paraview_build_TARGET_NAME}" INTERFACE)
    if (_paraview_build_NAMESPACE)
      add_library("${_paraview_build_TARGET}" ALIAS "${_paraview_build_TARGET_NAME}")
    endif ()
    target_include_directories("${_paraview_build_TARGET_NAME}"
      INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_paraview_build_target_safe}>"
        "$<INSTALL_INTERFACE:${_paraview_build_HEADERS_DESTINATION}>")
    set(_paraview_build_include_file
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_paraview_build_target_safe}/${_paraview_build_target_safe}.h")

    set(_paraview_static_plugins)
    foreach (_paraview_build_plugin IN LISTS _paraview_build_PLUGINS)
      get_property(_paraview_build_plugin_type
        TARGET    "${_paraview_build_plugin}"
        PROPERTY  TYPE)
      if (_paraview_build_plugin_type STREQUAL "STATIC_LIBRARY")
        list(APPEND _paraview_static_plugins
          "${_paraview_build_plugin}")
      endif ()
    endforeach ()

    if (_paraview_static_plugins)
      target_link_libraries("${_paraview_build_TARGET_NAME}"
        INTERFACE
          ParaView::RemotingCore
          ${_paraview_static_plugins})

      set(_paraview_build_declarations)
      set(_paraview_build_calls)
      set(_paraview_build_names)
      foreach (_paraview_build_plugin IN LISTS _paraview_static_plugins)
        string(APPEND _paraview_build_declarations
          "PV_PLUGIN_IMPORT_INIT(${_paraview_build_plugin});\n")
        string(APPEND _paraview_build_calls
          "  if (sname == \"${_paraview_build_plugin}\")
  {
    if (load)
    {
      static bool loaded = false;
      if (!loaded)
      {
        loaded = PV_PLUGIN_IMPORT(${_paraview_build_plugin});
      }
    }
    return true;
  }\n\n")
        string(APPEND _paraview_build_names
          "  names.push_back(\"${_paraview_build_plugin}\");\n")
      endforeach ()

      set(_paraview_build_include_content
        "#ifndef ${_paraview_build_target_safe}_h
#define ${_paraview_build_target_safe}_h

#define PARAVIEW_BUILDING_PLUGIN
#define PARAVIEW_PLUGIN_BUILT_SHARED 0
#include \"vtkPVPlugin.h\"
#include \"vtkPVPluginLoader.h\"
#include \"vtkPVPluginTracker.h\"
#include <string>

${_paraview_build_declarations}
static bool ${_paraview_build_target_safe}_static_plugins_load(const char* name);
static bool ${_paraview_build_target_safe}_static_plugins_search(const char* name);
static void ${_paraview_build_target_safe}_static_plugins_list(const char* appname, std::vector<std::string>& names);

static void ${_paraview_build_target_safe}_initialize()
{
  vtkPVPluginLoader::RegisterLoadPluginCallback(${_paraview_build_target_safe}_static_plugins_load);
  vtkPVPluginTracker::RegisterStaticPluginSearchFunction(${_paraview_build_target_safe}_static_plugins_search);
  vtkPVPluginTracker::RegisterStaticPluginListFunction(${_paraview_build_target_safe}_static_plugins_list);
}

static bool ${_paraview_build_target_safe}_static_plugins_func(const char* name, bool load);

static bool ${_paraview_build_target_safe}_static_plugins_load(const char* name)
{
  return ${_paraview_build_target_safe}_static_plugins_func(name, true);
}

static bool ${_paraview_build_target_safe}_static_plugins_search(const char* name)
{
  return ${_paraview_build_target_safe}_static_plugins_func(name, false);
}

static void ${_paraview_build_target_safe}_static_plugins_list(const char* appname, std::vector<std::string>& names)
{
${_paraview_build_names}
  (void) appname;
  (void) names;
}

static bool ${_paraview_build_target_safe}_static_plugins_func(const char* name, bool load)
{
  std::string const sname = name;

${_paraview_build_calls}
  return false;
}

#endif\n")
    else ()
      set(_paraview_build_include_content
        "#ifndef ${_paraview_build_target_safe}_h
#define ${_paraview_build_target_safe}_h

void ${_paraview_build_target_safe}_initialize()
{
}

#endif\n")
    endif ()

    file(GENERATE
      OUTPUT  "${_paraview_build_include_file}"
      CONTENT "${_paraview_build_include_content}")
    if (_paraview_build_INSTALL_HEADERS)
      install(
        FILES       "${_paraview_build_include_file}"
        DESTINATION "${_paraview_build_HEADERS_DESTINATION}"
        COMPONENT   "${_paraview_build_TARGET_COMPONENT}")
    endif ()

    if (DEFINED _paraview_build_INSTALL_EXPORT)
      install(
        TARGETS   "${_paraview_build_TARGET_NAME}"
        EXPORT    "${_paraview_build_INSTALL_EXPORT}"
        COMPONENT "${_paraview_build_TARGET_COMPONENT}")

      set(_paraview_build_required_exports_include_file_name "${_paraview_build_INSTALL_EXPORT}-${_paraview_build_TARGET_NAME}-targets-depends.cmake")
      set(_paraview_build_required_exports_include_build_file
        "${CMAKE_BINARY_DIR}/${_paraview_build_CMAKE_DESTINATION}/${_paraview_build_required_exports_include_file_name}")
      set(_paraview_build_required_exports_include_contents "")
      get_property(_paraview_build_required_exports GLOBAL
        PROPERTY "paraview_plugin_${_paraview_build_TARGET}_required_exports")
      if (_paraview_build_required_exports)
        foreach (_paraview_build_required_export IN LISTS _paraview_build_required_exports)
          string(APPEND _paraview_build_required_exports_include_contents
            "include(\"\${CMAKE_CURRENT_LIST_DIR}/${_paraview_build_required_export}-targets.cmake\")\n"
            "include(\"\${CMAKE_CURRENT_LIST_DIR}/${_paraview_build_required_export}-vtk-module-properties.cmake\")\n"
            "\n")

          get_property(_paraview_build_modules GLOBAL
            PROPERTY "paraview_plugin_${_paraview_build_required_export}_modules")
          if (_paraview_build_modules)
            vtk_module_export_find_packages(
              CMAKE_DESTINATION "${_paraview_build_CMAKE_DESTINATION}"
              FILE_NAME         "${_paraview_build_required_export}-vtk-module-find-packages.cmake"
              MODULES           ${_paraview_build_modules})

            # TODO: The list of modules should be checked for their `_FOUND`
            # variables being false and propagate it up through the parent
            # project's `_FOUND` variable.
            string(APPEND _paraview_build_required_exports_include_contents
              "set(CMAKE_FIND_PACKAGE_NAME_save \"\${CMAKE_FIND_PACKAGE_NAME}\")\n"
              "set(${_paraview_build_required_export}_FIND_QUIETLY \"\${\${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY}\")\n"
              "set(${_paraview_build_required_export}_FIND_COMPONENTS)\n"
              "set(CMAKE_FIND_PACKAGE_NAME \"${_paraview_build_required_export}\")\n"
              "include(\"\${CMAKE_CURRENT_LIST_DIR}/${_paraview_build_required_export}-vtk-module-find-packages.cmake\")\n"
              "set(CMAKE_FIND_PACKAGE_NAME \"\${CMAKE_FIND_PACKAGE_NAME_save}\")\n"
              "unset(${_paraview_build_required_export}_FIND_QUIETLY)\n"
              "unset(${_paraview_build_required_export}_FIND_COMPONENTS)\n"
              "unset(CMAKE_FIND_PACKAGE_NAME_save)\n"
              "\n"
              "\n")
          endif ()
        endforeach ()
      endif ()
      file(GENERATE
        OUTPUT  "${_paraview_build_required_exports_include_build_file}"
        CONTENT "${_paraview_build_required_exports_include_contents}")
      if (_paraview_build_INSTALL_HEADERS)
        install(
          FILES       "${_paraview_build_required_exports_include_build_file}"
          DESTINATION "${_paraview_build_CMAKE_DESTINATION}"
          COMPONENT   "${_paraview_build_TARGET_COMPONENT}")
      endif ()

      set(_paraview_build_namespace_args)
      if (_paraview_build_NAMESPACE)
        list(APPEND _paraview_build_namespace_args
          NAMESPACE "${_paraview_build_NAMESPACE}::")
      endif ()

      if (_paraview_build_INSTALL_HEADERS)
        export(
          EXPORT    "${_paraview_build_INSTALL_EXPORT}"
          ${_paraview_build_namespace_args}
          FILE      "${CMAKE_BINARY_DIR}/${_paraview_build_CMAKE_DESTINATION}/${_paraview_build_INSTALL_EXPORT}-targets.cmake")
        install(
          EXPORT      "${_paraview_build_INSTALL_EXPORT}"
          DESTINATION "${_paraview_build_CMAKE_DESTINATION}"
          ${_paraview_build_namespace_args}
          FILE        "${_paraview_build_INSTALL_EXPORT}-targets.cmake"
          COMPONENT   "${_paraview_build_TARGET_COMPONENT}")
      endif ()
    endif ()
  endif ()

  if (DEFINED _paraview_build_PLUGINS_FILE_NAME)
    set(_paraview_build_xml_file
      "${CMAKE_BINARY_DIR}/${_paraview_build_plugin_destination}/${_paraview_build_PLUGINS_FILE_NAME}")
    set(_paraview_build_xml_content
      "<?xml version=\"1.0\"?>\n<Plugins>\n")
    foreach (_paraview_build_plugin IN LISTS _paraview_build_PLUGINS)
      set(_paraview_build_autoload 0)
      if (_paraview_build_plugin IN_LIST _paraview_build_AUTOLOAD)
        set(_paraview_build_autoload 1)
      endif ()
      string(APPEND _paraview_build_xml_content
        "  <Plugin name=\"${_paraview_build_plugin}\" auto_load=\"${_paraview_build_autoload}\"/>\n")
    endforeach ()
    string(APPEND _paraview_build_xml_content
      "</Plugins>\n")

    file(GENERATE
      OUTPUT  "${_paraview_build_xml_file}"
      CONTENT "${_paraview_build_xml_content}")
    install(
      FILES       "${_paraview_build_xml_file}"
      DESTINATION "${_paraview_build_plugin_destination}"
      COMPONENT   "${_paraview_build_TARGET_COMPONENT}")

    if (DEFINED _paraview_build_INSTALL_EXPORT)
      set_property(TARGET "${_paraview_build_TARGET_NAME}"
        PROPERTY
          "INTERFACE_paraview_plugin_plugins_file" "${_paraview_build_xml_file}")

     if (DEFINED _paraview_build_RUNTIME_DESTINATION)
        set_property(TARGET "${_paraview_build_TARGET_NAME}"
          PROPERTY
            "INTERFACE_paraview_plugin_plugins_file_install" "${_paraview_build_plugin_destination}/${_paraview_build_PLUGINS_FILE_NAME}")
      endif ()

      if (DEFINED _paraview_build_CMAKE_DESTINATION)
        set(_paraview_build_properties_filename "${_paraview_build_INSTALL_EXPORT}-paraview-plugin-properties.cmake")
        set(_paraview_build_properties_build_file
          "${CMAKE_BINARY_DIR}/${_paraview_build_CMAKE_DESTINATION}/${_paraview_build_properties_filename}")
        set(_paraview_build_properties_install_file
          "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_paraview_build_properties_filename}.install")

        file(WRITE "${_paraview_build_properties_build_file}")
        file(WRITE "${_paraview_build_properties_install_file}")

        _vtk_module_write_import_prefix(
          "${_paraview_build_properties_install_file}"
          "${_paraview_build_CMAKE_DESTINATION}")

        file(APPEND "${_paraview_build_properties_build_file}"
          "set_property(TARGET \"${_paraview_build_TARGET}\"
  PROPERTY
    INTERFACE_paraview_plugin_plugins_file \"${_paraview_build_xml_file}\")\n")
        file(APPEND "${_paraview_build_properties_install_file}"
          "set_property(TARGET \"${_paraview_build_TARGET}\"
  PROPERTY
    INTERFACE_paraview_plugin_plugins_file \"\${_vtk_module_import_prefix}/${_paraview_build_plugin_destination}/${_paraview_build_PLUGINS_FILE_NAME}\")
unset(_vtk_module_import_prefix)\n")

        if (_paraview_build_INSTALL_HEADERS)
          install(
            FILES       "${_paraview_build_properties_install_file}"
            DESTINATION "${_paraview_build_CMAKE_DESTINATION}"
            RENAME      "${_paraview_build_properties_filename}"
            COMPONENT   "${_paraview_build_TARGET_COMPONENT}")
        endif ()
      endif ()
    endif ()
  endif ()
endfunction ()

#[==[.md
## Plugin configuration files

Applications will want to consume plugin targets by discovering their locations
at runtime. In order to facilitate this, ParaView supports loading a `conf`
file which contains the locations of plugin targets' XML files. The plugins
specified in that file is then

```
paraview_plugin_write_conf(
  NAME <name>
  PLUGINS_TARGETS <target>...
  BUILD_DESTINATION <destination>

  [INSTALL_DESTINATION <destination>]
  [COMPONENT <component>])
```

  * `NAME`: (Required) The base name of the configuration file.
  * `PLUGINS_TARGETS`: (Required) The list of plugin targets to add to the
    configuration file.
  * `BUILD_DESTINATION`: (Required) Where to place the configuration file in
    the build tree.
  * `INSTALL_DESTINATION`: Where to install the configuration file in the
    install tree. If not provided, the configuration file will not be
    installed.
  * `COMPONENT`: (Defaults to `runtime`) The component to use when installing
    the configuration file.
#]==]
function (paraview_plugin_write_conf)
  cmake_parse_arguments(_paraview_plugin_conf
    ""
    "NAME;BUILD_DESTINATION;INSTALL_DESTINATION;COMPONENT"
    "PLUGINS_TARGETS"
    ${ARGN})

  if (_paraview_plugin_conf_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_write_conf: "
      "${_paraview_plugin_conf_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _paraview_plugin_conf_NAME)
    message(FATAL_ERROR
      "The `NAME` must not be empty.")
  endif ()

  if (NOT DEFINED _paraview_plugin_conf_BUILD_DESTINATION)
    message(FATAL_ERROR
      "The `BUILD_DESTINATION` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_plugin_conf_PLUGINS_TARGETS)
    message(FATAL_ERROR
      "The `PLUGINS_TARGETS` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_plugin_conf_COMPONENT)
    set(_paraview_plugin_conf_COMPONENT "runtime")
  endif ()

  set(_paraview_plugin_conf_file_name
    "${_paraview_plugin_conf_NAME}.conf")
  set(_paraview_plugin_conf_build_file
    "${CMAKE_BINARY_DIR}/${_paraview_plugin_conf_BUILD_DESTINATION}/${_paraview_plugin_conf_file_name}")
  set(_paraview_plugin_conf_install_file
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_paraview_plugin_conf_file_name}.install")
  set(_paraview_plugin_conf_build_contents)
  set(_paraview_plugin_conf_install_contents)
  foreach (_paraview_plugin_conf_target IN LISTS _paraview_plugin_conf_PLUGINS_TARGETS)
    get_property(_paraview_plugin_conf_plugins_target_is_imported
      TARGET    "${_paraview_plugin_conf_target}"
      PROPERTY  IMPORTED)
    if (_paraview_plugin_conf_plugins_target_is_imported)
      get_property(_paraview_plugin_conf_plugins_target_xml_build
        TARGET    "${_paraview_plugin_conf_target}"
        PROPERTY  "INTERFACE_paraview_plugin_plugins_file")
      set(_paraview_plugin_conf_plugins_target_xml_install
        "${_paraview_plugin_conf_plugins_target_xml_build}")

      file(RELATIVE_PATH _paraview_plugin_conf_rel_path
        "/prefix/${CMAKE_INSTALL_PREFIX}"
        "/prefix/${_paraview_plugin_conf_plugins_target_xml_install}")
      # If the external plugins XML file is under our installation destination,
      # use a relative path to it, otherwise keep the absolute path.
      if (NOT _paraview_plugin_conf_rel_path MATCHES "^\.\./")
        file(RELATIVE_PATH _paraview_plugin_conf_plugins_target_xml_install
          "/prefix/${CMAKE_INSTALL_PREFIX}/${_paraview_plugin_conf_INSTALL_DESTINATION}"
          "/prefix/${_paraview_plugin_conf_plugins_target_xml_install}")
      endif ()
    else ()
      get_property(_paraview_plugin_conf_plugins_target_is_alias
        TARGET    "${_paraview_plugin_conf_target}"
        PROPERTY  ALIASED_TARGET
        SET)
      if (_paraview_plugin_conf_plugins_target_is_alias)
        get_property(_paraview_plugin_conf_target
          TARGET    "${_paraview_plugin_conf_target}"
          PROPERTY  ALIASED_TARGET)
      endif ()
      get_property(_paraview_plugin_conf_plugins_target_xml_build
        TARGET    "${_paraview_plugin_conf_target}"
        PROPERTY  "INTERFACE_paraview_plugin_plugins_file")
      get_property(_paraview_plugin_conf_plugins_target_xml_install
        TARGET    "${_paraview_plugin_conf_target}"
        PROPERTY  "INTERFACE_paraview_plugin_plugins_file_install")

      if (_paraview_plugin_conf_plugins_target_xml_install)
        # Compute the relative path within the install tree.
        file(RELATIVE_PATH _paraview_plugin_conf_plugins_target_xml_install
          "/prefix/${_paraview_plugin_conf_INSTALL_DESTINATION}"
          "/prefix/${_paraview_plugin_conf_plugins_target_xml_install}")
      endif ()
    endif ()

    # TODO: Write out in JSON instead.
    if (_paraview_plugin_conf_plugins_target_xml_build)
      string(APPEND _paraview_plugin_conf_build_contents
        "${_paraview_plugin_conf_plugins_target_xml_build}\n")
    endif ()
    if (_paraview_plugin_conf_plugins_target_xml_install)
      string(APPEND _paraview_plugin_conf_install_contents
        "${_paraview_plugin_conf_plugins_target_xml_install}\n")
    endif ()
  endforeach ()

  file(GENERATE
    OUTPUT  "${_paraview_plugin_conf_build_file}"
    CONTENT "${_paraview_plugin_conf_build_contents}")

  if (_paraview_plugin_conf_INSTALL_DESTINATION)
    file(GENERATE
      OUTPUT  "${_paraview_plugin_conf_install_file}"
      CONTENT "${_paraview_plugin_conf_install_contents}")
    install(
      FILES       "${_paraview_plugin_conf_install_file}"
      DESTINATION "${_paraview_plugin_conf_INSTALL_DESTINATION}"
      RENAME      "${_paraview_plugin_conf_file_name}"
      COMPONENT   "${_paraview_plugin_conf_COMPONENT}")
  endif ()
endfunction ()

set(_paraview_plugin_source_dir "${CMAKE_CURRENT_LIST_DIR}")

#[==[.md
## Adding a plugin

TODO: Describe.

```
paraview_add_plugin(<name>
  [REQUIRED_ON_SERVER] [REQUIRED_ON_CLIENT]
  VERSION <version>

  [MODULE_FILES <vtk.module>...]
  [MODULE_ARGS <arg>...]
  [MODULES <module>...]
  [SOURCES <source>...]
  [SERVER_MANAGER_XML <xml>...]
  [MODULE_INSTALL_EXPORT <export>]

  [UI_INTERFACES <interface>...]
  [UI_RESOURCES <resource>...]
  [UI_FILES <file>...]

  [PYTHON_MODULES <module>...]

  [REQUIRED_PLUGINS <plugin>...]

  [EULA <eula>]
  [XML_DOCUMENTATION <ON|OFF>]
  [DOCUMENTATION_DIR <directory>]

  [FORCE_STATIC <ON|OFF>])
```

  * `REQUIRED_ON_SERVER`: The plugin is required to be loaded on the server for
    proper functionality.
  * `REQUIRED_ON_CLIENT`: The plugin is required to be loaded on the client for
    proper functionality.
  * `VERSION`: (Required) The version number of the plugin.
  * `MODULE_FILES`: Paths to `vtk.module` files describing modules to include
    in the plugin.
  * `MODULE_ARGS`: Arguments to pass to `vtk_module_build` for included modules.
  * `MODULES`: Modules to include in the plugin. These modules will be wrapped
    using client server and have their server manager XML files processed.
  * `SOURCES`: Source files for the plugin.
  * `SERVER_MANAGER_XML`: Server manager XML files for the plugin.
  * `UI_INTERFACES`: Interfaces to initialize, in the given order. See the
    plugin interfaces section for more details.
  * `MODULE_INSTALL_EXPORT`: (Defaults to `<name>`) If provided, any modules
    will be added to the given export set.
  * `UI_RESOURCES`: Qt resource files to include with the plugin.
  * `UI_FILES`: Qt `.ui` files to include with the plugin.
  * `PYTHON_MODULES`: Python modules to embed into the plugin.
  * `REQUIRED_PLUGINS`: Plugins which must be loaded for this plugin to
    function. These plugins do not need to be available at build time and are
    therefore their existence is not checked here.
  * `EULA`: A file with content to display as an end-user license agreement
    before the plugin is initialized at runtime.
  * `XML_DOCUMENTATION`: (Defaults to `ON`) If set, documentation will be
    generated for the associated XML files.
  * `DOCUMENTATION_DIR`: If specified, `*.html`, `*.css`, `*.png`, and `*.jpg`
    files in this directory will be copied and made available to the
    documentation.
  * `EXPORT`: (Deprecated) Use `paraview_plugin_build(INSTALL_EXPORT)` instead.
  * `FORCE_STATIC`: (Defaults to `OFF`) If set, the plugin will be built
    statically so that it can be embedded into an application.
#]==]
function (paraview_add_plugin name)
  if (NOT name STREQUAL _paraview_build_plugin)
    message(FATAL_ERROR
      "The ${_paraview_build_plugin}'s CMakeLists.txt may not add the ${name} "
      "plugin.")
  endif ()

  cmake_parse_arguments(_paraview_add_plugin
    "REQUIRED_ON_SERVER;REQUIRED_ON_CLIENT"
    "VERSION;EULA;EXPORT;MODULE_INSTALL_EXPORT;XML_DOCUMENTATION;DOCUMENTATION_DIR;FORCE_STATIC"
    "REQUIRED_PLUGINS;SERVER_MANAGER_XML;SOURCES;MODULES;UI_INTERFACES;UI_RESOURCES;UI_FILES;PYTHON_MODULES;MODULE_FILES;MODULE_ARGS"
    ${ARGN})

  if (_paraview_add_plugin_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_add_plugin: "
      "${_paraview_add_plugin_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_add_plugin_VERSION)
    message(FATAL_ERROR
      "The `VERSION` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_add_plugin_XML_DOCUMENTATION)
    set(_paraview_add_plugin_XML_DOCUMENTATION ON)
  endif ()

  if (NOT DEFINED _paraview_add_plugin_FORCE_STATIC)
    set(_paraview_add_plugin_FORCE_STATIC OFF)
  endif ()

  if (DEFINED _paraview_add_plugin_DOCUMENTATION_DIR AND
      NOT _paraview_add_plugin_XML_DOCUMENTATION)
    message(FATAL_ERROR
      "Specifying `DOCUMENTATION_DIR` and turning off `XML_DOCUMENTATION` "
      "makes no sense.")
  endif ()

  if (DEFINED _paraview_add_plugin_EXPORT)
    message(FATAL_ERROR
      "The `paraview_add_plugin(EXPORT)` argument is ignored in favor of "
      "`paraview_plugin_build(INSTALL_EXPORT)`.")
  endif ()

  if (_paraview_add_plugin_MODULE_ARGS)
    if (NOT _paraview_add_plugin_MODULE_FILES OR
        NOT _paraview_add_plugin_MODULES)
      message(FATAL_ERROR
        "The `MODULE_ARGS` argument requires `MODULE_FILES` and `MODULES` to be provided.")
    endif ()
  endif ()

  if (DEFINED _paraview_build_INSTALL_EXPORT AND
      NOT DEFINED _paraview_add_plugin_MODULE_INSTALL_EXPORT)
    set(_paraview_add_plugin_MODULE_INSTALL_EXPORT
      "${name}")
  endif ()

  if (_paraview_add_plugin_MODULE_FILES)
    if (NOT _paraview_add_plugin_MODULES)
      message(FATAL_ERROR
        "The `MODULE_FILES` argument requires `MODULES` to be provided.")
    endif ()

    if (_paraview_build_ADD_INSTALL_RPATHS)
      if (APPLE)
        list(INSERT CMAKE_INSTALL_RPATH 0
          "@loader_path")
      elseif (UNIX)
        list(INSERT CMAKE_INSTALL_RPATH 0
          "$ORIGIN")
      endif ()
    endif ()

    set(_paraview_add_plugin_module_install_export_args)
    if (DEFINED _paraview_add_plugin_MODULE_INSTALL_EXPORT)
      list(APPEND _paraview_add_plugin_module_install_export_args
        INSTALL_EXPORT "${_paraview_add_plugin_MODULE_INSTALL_EXPORT}")
      if (DEFINED _paraview_build_TARGET)
        set_property(GLOBAL APPEND
          PROPERTY
            "paraview_plugin_${_paraview_build_TARGET}_required_exports" "${_paraview_add_plugin_MODULE_INSTALL_EXPORT}")
      endif ()
    endif ()

    vtk_module_scan(
      MODULE_FILES      ${_paraview_add_plugin_MODULE_FILES}
      REQUEST_MODULES   ${_paraview_add_plugin_MODULES}
      PROVIDES_MODULES  plugin_modules
      REQUIRES_MODULES  required_modules
      HIDE_MODULES_FROM_CACHE ON)

    if (required_modules)
      foreach (required_module IN LISTS required_modules)
        if (NOT TARGET "${required_module}")
          message(FATAL_ERROR
            "Failed to find the required module ${required_module}.")
        endif ()
      endforeach ()
    endif ()

    if (WIN32)
      set(_paraview_plugin_subdir "${_paraview_build_RUNTIME_DESTINATION}")
    else ()
      set(_paraview_plugin_subdir "${_paraview_build_LIBRARY_DESTINATION}")
    endif ()
    if (NOT _paraview_build_LIBRARY_SUBDIRECTORY STREQUAL "")
      string(APPEND _paraview_plugin_subdir "/${_paraview_build_LIBRARY_SUBDIRECTORY}")
    endif ()
    string(APPEND _paraview_plugin_subdir "/${_paraview_build_plugin}")
    set(_paraview_plugin_CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(_paraview_plugin_CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    set(_paraview_plugin_CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    set(_paraview_plugin_CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_NAME_DIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_paraview_plugin_subdir}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_paraview_plugin_subdir}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_paraview_plugin_subdir}")
    set(CMAKE_INSTALL_NAME_DIR "@loader_path")

    vtk_module_build(
      MODULES             ${plugin_modules}
      PACKAGE             "${_paraview_build_plugin}"
      ${_paraview_add_plugin_module_install_export_args}
      INSTALL_HEADERS     "${_paraview_build_INSTALL_HEADERS}"
      TARGETS_COMPONENT   "${_paraview_build_PLUGINS_COMPONENT}"
      HEADERS_DESTINATION "${_paraview_build_HEADERS_DESTINATION}/${_paraview_build_target_safe}"
      ARCHIVE_DESTINATION "${_paraview_plugin_subdir}"
      LIBRARY_DESTINATION "${_paraview_plugin_subdir}"
      RUNTIME_DESTINATION "${_paraview_plugin_subdir}"
      CMAKE_DESTINATION   "${_paraview_build_CMAKE_DESTINATION}"
      ${_paraview_add_plugin_MODULE_ARGS})

    set_property(GLOBAL APPEND
      PROPERTY
        "paraview_plugin_${_paraview_add_plugin_MODULE_INSTALL_EXPORT}_modules" "${plugin_modules}")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${_paraview_plugin_CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${_paraview_plugin_CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${_paraview_plugin_CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    set(CMAKE_INSTALL_NAME_DIR "${_paraview_plugin_CMAKE_INSTALL_NAME_DIR}")
    unset(_paraview_plugin_CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    unset(_paraview_plugin_CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    unset(_paraview_plugin_CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    unset(_paraview_plugin_CMAKE_INSTALL_NAME_DIR)
  endif ()

  # TODO: resource initialization for static builds

  if (_paraview_add_plugin_REQUIRED_ON_SERVER)
    set(_paraview_add_plugin_required_on_server "true")
  else ()
    set(_paraview_add_plugin_required_on_server "false")
  endif ()

  if (_paraview_add_plugin_REQUIRED_ON_CLIENT)
    set(_paraview_add_plugin_required_on_client "true")
  else ()
    set(_paraview_add_plugin_required_on_client "false")
  endif ()

  set(_paraview_add_plugin_export_args)
  set(_paraview_add_plugin_install_export_args)
  if (DEFINED _paraview_build_INSTALL_EXPORT)
    list(APPEND _paraview_add_plugin_export_args
      EXPORT "${_paraview_build_INSTALL_EXPORT}")
    list(APPEND _paraview_add_plugin_install_export_args
      INSTALL_EXPORT "${_paraview_build_INSTALL_EXPORT}")
  endif ()

  set(_paraview_add_plugin_includes)
  set(_paraview_add_plugin_required_libraries)

  set(_paraview_add_plugin_module_xmls)
  set(_paraview_add_plugin_with_xml 0)
  if (_paraview_add_plugin_MODULES)
    set(_paraview_add_plugin_with_xml 1)

    list(APPEND _paraview_add_plugin_required_libraries
      ${_paraview_add_plugin_MODULES})

    vtk_module_wrap_client_server(
      MODULES ${_paraview_add_plugin_MODULES}
      TARGET  "${_paraview_build_plugin}_client_server"
      ${_paraview_add_plugin_install_export_args})
    paraview_server_manager_process(
      MODULES   ${_paraview_add_plugin_MODULES}
      TARGET    "${_paraview_build_plugin}_server_manager_modules"
      ${_paraview_add_plugin_install_export_args}
      XML_FILES _paraview_add_plugin_module_xmls)

    list(APPEND _paraview_add_plugin_required_libraries
      "${_paraview_build_plugin}_client_server"
      "${_paraview_build_plugin}_server_manager_modules")
  endif ()

  set(_paraview_add_plugin_binary_resources "")
  set(_paraview_add_plugin_binary_headers)
  if (_paraview_add_plugin_SERVER_MANAGER_XML)
    set(_paraview_add_plugin_with_xml 1)

    set(_paraview_add_plugin_xmls)
    foreach (_paraview_add_plugin_xml IN LISTS _paraview_add_plugin_SERVER_MANAGER_XML)
      if (NOT IS_ABSOLUTE "${_paraview_add_plugin_xml}")
        set(_paraview_add_plugin_xml "${CMAKE_CURRENT_SOURCE_DIR}/${_paraview_add_plugin_xml}")
      endif ()

      list(APPEND _paraview_add_plugin_xmls
        "${_paraview_add_plugin_xml}")
    endforeach ()

    paraview_server_manager_process_files(
      TARGET    "${_paraview_build_plugin}_server_manager"
      ${_paraview_add_plugin_install_export_args}
      FILES     ${_paraview_add_plugin_xmls})
    list(APPEND _paraview_add_plugin_required_libraries
      "${_paraview_build_plugin}_server_manager")
  endif ()

  if ((_paraview_add_plugin_module_xmls OR _paraview_add_plugin_xmls) AND
      PARAVIEW_USE_QT AND _paraview_add_plugin_XML_DOCUMENTATION)
    set(_paraview_build_plugin_docdir
      "${CMAKE_CURRENT_BINARY_DIR}/paraview_help")

    paraview_client_documentation(
      TARGET      "${_paraview_build_plugin}_doc"
      OUTPUT_DIR  "${_paraview_build_plugin_docdir}"
      XMLS        ${_paraview_add_plugin_module_xmls}
                  ${_paraview_add_plugin_xmls})

    set(_paraview_build_plugin_doc_source_args)
    if (DEFINED _paraview_add_plugin_DOCUMENTATION_DIR)
      list(APPEND _paraview_build_plugin_doc_source_args
        SOURCE_DIR "${_paraview_add_plugin_DOCUMENTATION_DIR}")
    endif ()

    paraview_client_generate_help(
      NAME        "${_paraview_build_plugin}"
      OUTPUT_PATH _paraview_build_plugin_qch_path
      OUTPUT_DIR  "${_paraview_build_plugin_docdir}"
      TARGET      "${_paraview_build_plugin}_qch"
      ${_paraview_build_plugin_doc_source_args}
      DEPENDS     "${_paraview_build_plugin}_doc"
      PATTERNS    "*.html" "*.css" "*.png" "*.jpg")

    list(APPEND _paraview_add_plugin_extra_include_dirs
      "${CMAKE_CURRENT_BINARY_DIR}")
    set(_paraview_add_plugin_qch_output
      "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_build_plugin}_qch.h")
    list(APPEND _paraview_add_plugin_binary_headers
      "${_paraview_add_plugin_qch_output}")
    add_custom_command(
      OUTPUT "${_paraview_add_plugin_qch_output}"
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
              $<TARGET_FILE:ParaView::ProcessXML>
              -base64
              "${_paraview_add_plugin_qch_output}"
              \"\"
              "_qch"
              "_qch"
              "${_paraview_build_plugin_qch_path}"
      DEPENDS "${_paraview_build_plugin_qch_path}"
              "${_paraview_build_plugin}_qch"
              "$<TARGET_FILE:ParaView::ProcessXML>"
      COMMENT "Generating header for ${_paraview_build_plugin} documentation")
    set_property(SOURCE "${_paraview_add_plugin_qch_output}"
      PROPERTY
        SKIP_AUTOMOC 1)

    string(APPEND _paraview_add_plugin_includes
      "#include \"${_paraview_build_plugin}_qch.h\"\n")
    string(APPEND _paraview_add_plugin_binary_resources
      "  {
    const char *text = ${_paraview_build_plugin}_qch();
    resources.emplace_back(text);
    delete [] text;
  }\n")
  endif ()

  set(_paraview_add_plugin_eula_sources)
  if (_paraview_add_plugin_EULA)
    vtk_encode_string(
      INPUT "${_paraview_add_plugin_EULA}"
      NAME  "${_paraview_build_plugin}_EULA"
      HEADER_OUTPUT _paraview_add_plugin_eula_header
      SOURCE_OUTPUT _paraview_add_plugin_eula_source)
    list(APPEND _paraview_add_plugin_eula_sources
      "${_paraview_add_plugin_eula_header}"
      "${_paraview_add_plugin_eula_source}")
  endif ()

  set(_paraview_add_plugin_with_ui 0)
  set(_paraview_add_plugin_ui_sources)
  if (_paraview_add_plugin_UI_INTERFACES)
    set(_paraview_add_plugin_with_ui 1)
    set(CMAKE_AUTOMOC 1)
    set(_paraview_add_plugin_push_back_interfaces
      "#define PARAVIEW_ADD_INTERFACES(arg) \\\n")
    set(_paraview_add_plugin_include_interfaces "")

    foreach (_paraview_add_plugin_ui_interface IN LISTS _paraview_add_plugin_UI_INTERFACES)
      string(APPEND _paraview_add_plugin_push_back_interfaces
        "  (arg).push_back(new ${_paraview_add_plugin_ui_interface}(this)); \\\n")
      string(APPEND _paraview_add_plugin_include_interfaces
        "#include \"${_paraview_add_plugin_ui_interface}.h\"\n")
    endforeach ()
    list(APPEND _paraview_add_plugin_required_libraries
      ParaView::pqComponents)
  endif ()

  set(_paraview_add_plugin_with_resources 0)
  set(_paraview_add_plugin_resources_init)
  if (_paraview_add_plugin_UI_RESOURCES)
    set(_paraview_add_plugin_with_resources 1)
    set(CMAKE_AUTORCC 1)
    if (NOT BUILD_SHARED_LIBS OR _paraview_add_plugin_FORCE_STATIC)
      foreach (_paraview_add_plugin_ui_resource IN LISTS _paraview_add_plugin_UI_RESOURCES)
        get_filename_component(_paraview_add_plugin_ui_resource_base "${_paraview_add_plugin_ui_resource}" NAME_WE)
        string(APPEND _paraview_add_plugin_resources_init
          "  Q_INIT_RESOURCE(${_paraview_add_plugin_ui_resource_base});\n")
      endforeach ()
    endif ()
    list(APPEND _paraview_add_plugin_ui_sources
      ${_paraview_add_plugin_UI_RESOURCES})
  endif ()

  set(_paraview_add_plugin_qt_extra_components)
  if (_paraview_add_plugin_UI_FILES)
    set(_paraview_add_plugin_with_ui 1)
    set(CMAKE_AUTOUIC 1)
    list(APPEND _paraview_add_plugin_qt_extra_components
      Widgets)
    list(APPEND _paraview_add_plugin_required_libraries
      Qt5::Widgets)
    list(APPEND _paraview_add_plugin_ui_sources
      ${_paraview_add_plugin_UI_FILES})
  endif ()

  if (_paraview_add_plugin_with_ui OR _paraview_add_plugin_with_resources)
    include("${_ParaViewPlugin_cmake_dir}/paraview-find-package-helpers.cmake" OPTIONAL)
    find_package(Qt5 QUIET REQUIRED COMPONENTS Core ${_paraview_add_plugin_qt_extra_components})
    list(APPEND _paraview_add_plugin_required_libraries
      Qt5::Core)
    if (_paraview_add_plugin_with_ui)
      list(APPEND _paraview_add_plugin_required_libraries
        ParaView::pqCore)
    endif ()

    # CMake 3.13 started using Qt5's version variables to detect what version
    # of Qt's tools to run for automoc, autouic, and autorcc. However, they are
    # looked up using the target's directory scope, but these are here in a
    # local scope and unset when AutoGen gets around to asking about the
    # variables at generate time.

    # Fix for 3.13.0–3.13.3. Does not work if `paraview_add_plugin` is called
    # from another function.
    set(Qt5Core_VERSION_MAJOR "${Qt5Core_VERSION_MAJOR}" PARENT_SCOPE)
    set(Qt5Core_VERSION_MINOR "${Qt5Core_VERSION_MINOR}" PARENT_SCOPE)
    # Fix for 3.13.4+.
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      PROPERTY
        Qt5Core_VERSION_MAJOR "${Qt5Core_VERSION_MAJOR}")
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      PROPERTY
        Qt5Core_VERSION_MINOR "${Qt5Core_VERSION_MAJOR}")
  endif ()

  set(_paraview_add_plugin_with_python 0)
  set(_paraview_add_plugin_python_sources)
  set(_paraview_add_plugin_python_includes)
  set(_paraview_add_plugin_python_modules)
  set(_paraview_add_plugin_python_module_sources)
  set(_paraview_add_plugin_python_package_flags)
  if (_paraview_add_plugin_PYTHON_MODULES)
    set(_paraview_add_plugin_with_python 1)
    foreach (_paraview_add_plugin_python_module IN LISTS _paraview_add_plugin_PYTHON_MODULES)
      set(_paraview_add_plugin_python_path
        "${CMAKE_CURRENT_SOURCE_DIR}/${_paraview_add_plugin_python_module}")
      get_filename_component(_paraview_add_plugin_python_package "${_paraview_add_plugin_python_module}" PATH)
      get_filename_component(_paraview_add_plugin_python_name "${_paraview_add_plugin_python_module}" NAME_WE)
      if (_paraview_add_plugin_python_package)
        set(_paraview_add_plugin_python_full_name
          "${_paraview_add_plugin_python_package}.${_paraview_add_plugin_python_name}")
      else ()
        set(_paraview_add_plugin_python_full_name
          "${_paraview_add_plugin_python_name}")
      endif ()
      string(REPLACE "." "_" _paraview_add_plugin_python_module_mangled "${_paraview_add_plugin_python_full_name}")
      set(_paraview_add_plugin_python_is_package 0)
      set(_paraview_add_plugin_python_import
        "${_paraview_add_plugin_python_full_name}")
      if (_paraview_add_plugin_python_name STREQUAL "__init__")
        set(_paraview_add_plugin_python_is_package 1)
        set(_paraview_add_plugin_python_import
          "${_paraview_add_plugin_python_package}")
      endif ()
      set(_paraview_add_plugin_python_header_name
        "WrappedPython_${_paraview_build_plugin}_${_paraview_add_plugin_python_module_mangled}.h")
      set(_paraview_add_plugin_python_header
        "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_add_plugin_python_header_name}")
      add_custom_command(
        OUTPUT  "${_paraview_add_plugin_python_header}"
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
                $<TARGET_FILE:ParaView::ProcessXML>
                "${_paraview_add_plugin_python_header}"
                "module_${_paraview_add_plugin_python_module_mangled}_"
                "_string"
                "_source"
                "${_paraview_add_plugin_python_path}"
        DEPENDS "${_paraview_add_plugin_python_path}"
                "$<TARGET_FILE:ParaView::ProcessXML>"
        COMMENT "Convert Python module ${_paraview_add_plugin_python_module_name} for ${_paraview_build_plugin}")

      list(APPEND _paraview_add_plugin_python_sources
        "${_paraview_add_plugin_python_header}")
      string(APPEND _paraview_add_plugin_python_includes
        "#include \"${_paraview_add_plugin_python_header_name}\"\n")
      string(APPEND _paraview_add_plugin_python_modules
        "    \"${_paraview_add_plugin_python_import}\",\n")
      string(APPEND _paraview_add_plugin_python_module_sources
        "    module_${_paraview_add_plugin_python_module_mangled}_${_paraview_add_plugin_python_name}_source(),\n")
      string(APPEND _paraview_add_plugin_python_package_flags
        "    ${_paraview_add_plugin_python_is_package},\n")
    endforeach ()

    # Add terminators to the list.
    string(APPEND _paraview_add_plugin_python_modules
      "    nullptr")
    string(APPEND _paraview_add_plugin_python_module_sources
      "    nullptr")
    string(APPEND _paraview_add_plugin_python_package_flags
      "    -1")
  endif ()

  set(_paraview_add_plugin_header
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_build_plugin}Plugin.h")
  set(_paraview_add_plugin_source
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_build_plugin}Plugin.cxx")

  get_property(_paraview_add_plugin_description GLOBAL
    PROPERTY "_paraview_plugin_${_paraview_build_plugin}_description")

  set(_paraview_build_plugin_type MODULE)
  set(_paraview_add_plugin_built_shared 1)
  if (NOT BUILD_SHARED_LIBS OR _paraview_add_plugin_FORCE_STATIC)
    set(_paraview_build_plugin_type STATIC)
    set(_paraview_add_plugin_built_shared 0)
  endif ()

  configure_file(
    "${_paraview_plugin_source_dir}/paraview_plugin.h.in"
    "${_paraview_add_plugin_header}")
  configure_file(
    "${_paraview_plugin_source_dir}/paraview_plugin.cxx.in"
    "${_paraview_add_plugin_source}")

  if (WIN32)
    # On Windows, we want `MODULE` libraries to go to the runtime directory,
    # but CMake always uses `CMAKE_LIBRARY_OUTPUT_DIRECTORY`.
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  endif ()
  if (NOT _paraview_build_LIBRARY_SUBDIRECTORY STREQUAL "")
    string(APPEND CMAKE_LIBRARY_OUTPUT_DIRECTORY "/${_paraview_build_LIBRARY_SUBDIRECTORY}")
  endif ()
  string(APPEND CMAKE_LIBRARY_OUTPUT_DIRECTORY "/${_paraview_build_plugin}")

  # Place static plugins in the same place they would be if they were shared.
  if (NOT _paraview_build_LIBRARY_SUBDIRECTORY STREQUAL "")
    string(APPEND CMAKE_ARCHIVE_OUTPUT_DIRECTORY "/${_paraview_build_LIBRARY_SUBDIRECTORY}")
  endif ()
  string(APPEND CMAKE_ARCHIVE_OUTPUT_DIRECTORY "/${_paraview_build_plugin}")

  add_library("${_paraview_build_plugin}" "${_paraview_build_plugin_type}"
    ${_paraview_add_plugin_header}
    ${_paraview_add_plugin_source}
    ${_paraview_add_plugin_eula_sources}
    ${_paraview_add_plugin_binary_headers}
    ${_paraview_add_plugin_ui_sources}
    ${_paraview_add_plugin_python_sources}
    ${_paraview_add_plugin_SOURCES})
  if (NOT BUILD_SHARED_LIBS OR _paraview_add_plugin_FORCE_STATIC)
    target_compile_definitions("${_paraview_build_plugin}"
      PRIVATE
        QT_STATICPLUGIN)
  endif ()
  target_link_libraries("${_paraview_build_plugin}"
    PRIVATE
      ParaView::RemotingCore
      ${_paraview_add_plugin_required_libraries})
  target_include_directories("${_paraview_build_plugin}"
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}"
      ${_paraview_add_plugin_extra_include_dirs})
  set_property(TARGET "${_paraview_build_plugin}"
    PROPERTY
      PREFIX "")

  set(_paraview_add_plugin_destination
    "${_paraview_build_plugin_destination}/${_paraview_build_plugin}")
  install(
    TARGETS "${_paraview_build_plugin}"
    ${_paraview_add_plugin_export_args}
    COMPONENT "${_paraview_build_PLUGINS_COMPONENT}"
    ARCHIVE DESTINATION "${_paraview_add_plugin_destination}"
    LIBRARY DESTINATION "${_paraview_add_plugin_destination}")
endfunction ()

#[==[.md
## Plugin interfaces

ParaView plugins may satisfy a number of interfaces. These functions all take a
`INTERFACES` argument which takes the name of a variable to set with the name
of the interface generated. This variable's should be passed to
`paraview_add_plugin`'s `UI_INTERFACES` argument.
#]==]

#[==[.md
### Property widget

TODO: What is a property widget?

```
paraview_plugin_add_property_widget(
  KIND  <WIDGET|GROUP_WIDGET|WIDGET_DECORATOR>
  TYPE <type>
  CLASS_NAME <name>
  INTERFACES <variable>
  SOURCES <variable>)
```

  * `KIND`: The kind of widget represented.
  * `TYPE`: The name of the property type.
  * `CLASS_NAME`: The name of the property widget class.
  * `INTERFACES`: The name of the generated interface.
  * `SOURCES`: The source files generated by the interface.
#]==]
function (paraview_plugin_add_property_widget)
  cmake_parse_arguments(_paraview_property_widget
    ""
    "KIND;TYPE;CLASS_NAME;INTERFACES;SOURCES"
    ""
    ${ARGN})

  if (_paraview_property_widget_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_add_property_widget: "
      "${_paraview_property_widget_UNPARSED_ARGUMENTS}")
  endif ()

  set(_paraview_property_widget_kind_widget 0)
  set(_paraview_property_widget_kind_group_widget 0)
  set(_paraview_property_widget_kind_widget_decorator 0)
  if (_paraview_property_widget_KIND STREQUAL "WIDGET")
    set(_paraview_property_widget_kind_widget 1)
  elseif (_paraview_property_widget_KIND STREQUAL "GROUP_WIDGET")
    set(_paraview_property_widget_kind_group_widget 1)
  elseif (_paraview_property_widget_KIND STREQUAL "WIDGET_DECORATOR")
    set(_paraview_property_widget_kind_widget_decorator 1)
  else ()
    message(FATAL_ERROR
      "The `KIND` argument must be one of `WIDGET`, `GROUP_WIDGET`, or "
      "`WIDGET_DECORATOR`.")
  endif ()

  if (NOT DEFINED _paraview_property_widget_TYPE)
    message(FATAL_ERROR
      "The `TYPE` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_property_widget_CLASS_NAME)
    message(FATAL_ERROR
      "The `CLASS_NAME` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_property_widget_INTERFACES)
    message(FATAL_ERROR
      "The `INTERFACES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_property_widget_SOURCES)
    message(FATAL_ERROR
      "The `SOURCES` argument is required.")
  endif ()

  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqPropertyWidgetInterface.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_property_widget_CLASS_NAME}PWIImplementation.h"
    @ONLY)
  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqPropertyWidgetInterface.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_property_widget_CLASS_NAME}PWIImplementation.cxx"
    @ONLY)

  set("${_paraview_property_widget_INTERFACES}"
    "${_paraview_property_widget_CLASS_NAME}PWIImplementation"
    PARENT_SCOPE)

  set("${_paraview_property_widget_SOURCES}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_property_widget_CLASS_NAME}PWIImplementation.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_property_widget_CLASS_NAME}PWIImplementation.h"
    PARENT_SCOPE)
endfunction ()

#[==[.md
### Dock window

TODO: What is a dock window?

```
paraview_plugin_add_dock_window(
  CLASS_NAME <name>
  [DOCK_AREA <Right|Left|Top|Bottom>]
  INTERFACES <variable>
  SOURCES <variable>)
```

  * `CLASS_NAME`: The name of the dock window class.
  * `DOCK_AREA`: (Default `Left`) Where to dock the window within the
    application.
  * `INTERFACES`: The name of the generated interface.
  * `SOURCES`: The source files generated by the interface.
#]==]
function (paraview_plugin_add_dock_window)
  cmake_parse_arguments(_paraview_dock_window
    ""
    "DOCK_AREA;CLASS_NAME;INTERFACES;SOURCES"
    ""
    ${ARGN})

  if (_paraview_dock_window_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_add_dock_window: "
      "${_paraview_dock_window_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_dock_window_CLASS_NAME)
    message(FATAL_ERROR
      "The `CLASS_NAME` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_dock_window_INTERFACES)
    message(FATAL_ERROR
      "The `INTERFACES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_dock_window_SOURCES)
    message(FATAL_ERROR
      "The `SOURCES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_dock_window_DOCK_AREA)
    set(_paraview_dock_window_DOCK_AREA "Left")
  endif ()

  if (NOT _paraview_dock_window_DOCK_AREA STREQUAL "Left" AND
      NOT _paraview_dock_window_DOCK_AREA STREQUAL "Right" AND
      NOT _paraview_dock_window_DOCK_AREA STREQUAL "Top" AND
      NOT _paraview_dock_window_DOCK_AREA STREQUAL "Bottom")
    message(FATAL_ERROR
      "`DOCK_AREA` must be one of `Left`, `Right`, `Top`, or `Bottom`. Got "
      "`${_paraview_dock_window_DOCK_AREA}`.")
  endif ()

  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqDockWindowImplementation.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_dock_window_CLASS_NAME}Implementation.h"
    @ONLY)
  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqDockWindowImplementation.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_dock_window_CLASS_NAME}Implementation.cxx"
    @ONLY)

  set("${_paraview_dock_window_INTERFACES}"
    "${_paraview_dock_window_CLASS_NAME}Implementation"
    PARENT_SCOPE)

  set("${_paraview_dock_window_SOURCES}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_dock_window_CLASS_NAME}Implementation.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_dock_window_CLASS_NAME}Implementation.h"
    PARENT_SCOPE)
endfunction ()

#[==[.md
### Action group

TODO: What is an action group?

```
paraview_plugin_add_action_group(
  CLASS_NAME <name>
  GROUP_NAME <name>
  INTERFACES <variable>
  SOURCES <variable>)
```

  * `CLASS_NAME`: The name of the action group class.
  * `GROUP_NAME`: The name of the action group.
  * `INTERFACES`: The name of the generated interface.
  * `SOURCES`: The source files generated by the interface.
#]==]
function (paraview_plugin_add_action_group)
  cmake_parse_arguments(_paraview_action_group
    ""
    "CLASS_NAME;GROUP_NAME;INTERFACES;SOURCES"
    ""
    ${ARGN})

  if (_paraview_action_group_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_add_action_group: "
      "${_paraview_action_group_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_action_group_CLASS_NAME)
    message(FATAL_ERROR
      "The `CLASS_NAME` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_action_group_GROUP_NAME)
    message(FATAL_ERROR
      "The `GROUP_NAME` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_action_group_INTERFACES)
    message(FATAL_ERROR
      "The `INTERFACES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_action_group_SOURCES)
    message(FATAL_ERROR
      "The `SOURCES` argument is required.")
  endif ()

  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqActionGroupImplementation.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_action_group_CLASS_NAME}Implementation.h"
    @ONLY)
  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqActionGroupImplementation.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_action_group_CLASS_NAME}Implementation.cxx"
    @ONLY)

  set("${_paraview_action_group_INTERFACES}"
    "${_paraview_action_group_CLASS_NAME}Implementation"
    PARENT_SCOPE)

  set("${_paraview_action_group_SOURCES}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_action_group_CLASS_NAME}Implementation.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_action_group_CLASS_NAME}Implementation.h"
    PARENT_SCOPE)
endfunction ()

#[==[.md
### Toolbar

TODO: What is a toolbar?

```
paraview_plugin_add_toolbar(
  CLASS_NAME <name>
  INTERFACES <variable>
  SOURCES <variable>)
```

  * `CLASS_NAME`: The name of the toolbar class.
  * `INTERFACES`: The name of the generated interface.
  * `SOURCES`: The source files generated by the interface.
#]==]
function (paraview_plugin_add_toolbar)
  cmake_parse_arguments(_paraview_toolbar
    ""
    "CLASS_NAME;INTERFACES;SOURCES"
    ""
    ${ARGN})

  if (_paraview_toolbar_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_add_toolbar: "
      "${_paraview_toolbar_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_toolbar_CLASS_NAME)
    message(FATAL_ERROR
      "The `CLASS_NAME` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_toolbar_INTERFACES)
    message(FATAL_ERROR
      "The `INTERFACES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_toolbar_SOURCES)
    message(FATAL_ERROR
      "The `SOURCES` argument is required.")
  endif ()

  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqToolBarImplementation.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_toolbar_CLASS_NAME}Implementation.h"
    @ONLY)
  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqToolBarImplementation.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_toolbar_CLASS_NAME}Implementation.cxx"
    @ONLY)

  set("${_paraview_toolbar_INTERFACES}"
    "${_paraview_toolbar_CLASS_NAME}Implementation"
    PARENT_SCOPE)

  set("${_paraview_toolbar_SOURCES}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_toolbar_CLASS_NAME}Implementation.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_toolbar_CLASS_NAME}Implementation.h"
    PARENT_SCOPE)
endfunction ()

#[==[.md
### View frame action group

TODO: What is a view frame action group?

```
paraview_plugin_add_view_frame_action_group(
  CLASS_NAME <name>
  INTERFACES <variable>
  SOURCES <variable>)
```

  * `CLASS_NAME`: The name of the view frame action group class.
  * `INTERFACES`: The name of the generated interface.
  * `SOURCES`: The source files generated by the interface.
#]==]
function (paraview_plugin_add_view_frame_action_group)
  cmake_parse_arguments(_paraview_view_frame_action_group
    ""
    "CLASS_NAME;INTERFACES;SOURCES"
    ""
    ${ARGN})

  if (_paraview_view_frame_action_group_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_add_view_frame_action_group: "
      "${_paraview_view_frame_action_group_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_view_frame_action_group_CLASS_NAME)
    message(FATAL_ERROR
      "The `CLASS_NAME` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_view_frame_action_group_INTERFACES)
    message(FATAL_ERROR
      "The `INTERFACES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_view_frame_action_group_SOURCES)
    message(FATAL_ERROR
      "The `SOURCES` argument is required.")
  endif ()

  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqViewFrameActionGroupImplementation.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_view_frame_action_group_CLASS_NAME}Implementation.h"
    @ONLY)
  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqViewFrameActionGroupImplementation.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_view_frame_action_group_CLASS_NAME}Implementation.cxx"
    @ONLY)

  set("${_paraview_view_frame_action_group_INTERFACES}"
    "${_paraview_view_frame_action_group_CLASS_NAME}Implementation"
    PARENT_SCOPE)

  set("${_paraview_view_frame_action_group_SOURCES}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_view_frame_action_group_CLASS_NAME}Implementation.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_view_frame_action_group_CLASS_NAME}Implementation.h"
    PARENT_SCOPE)
endfunction ()

#[==[.md
### Auto start

TODO: What is an auto start?

```
paraview_plugin_add_auto_start(
  CLASS_NAME <name>
  [STARTUP <function>]
  [SHUTDOWN <function>]
  INTERFACES <variable>
  SOURCES <variable>)
```

  * `CLASS_NAME`: The name of the auto start class.
  * `STARTUP`: (Defaults to `startup`) The name of the method to call on
    startup.
  * `SHUTDOWN`: (Defaults to `shutdown`) The name of the method to call on
    shutdown.
  * `INTERFACES`: The name of the generated interface.
  * `SOURCES`: The source files generated by the interface.
#]==]
function (paraview_plugin_add_auto_start)
  cmake_parse_arguments(_paraview_auto_start
    ""
    "CLASS_NAME;INTERFACES;SOURCES;STARTUP;SHUTDOWN"
    ""
    ${ARGN})

  if (_paraview_auto_start_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_add_auto_start: "
      "${_paraview_auto_start_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_auto_start_CLASS_NAME)
    message(FATAL_ERROR
      "The `CLASS_NAME` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_auto_start_INTERFACES)
    message(FATAL_ERROR
      "The `INTERFACES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_auto_start_SOURCES)
    message(FATAL_ERROR
      "The `SOURCES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_auto_start_STARTUP)
    set(_paraview_auto_start_STARTUP "startup")
  endif ()

  if (NOT DEFINED _paraview_auto_start_SHUTDOWN)
    set(_paraview_auto_start_SHUTDOWN "shutdown")
  endif ()

  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqAutoStartImplementation.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_auto_start_CLASS_NAME}Implementation.h"
    @ONLY)
  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqAutoStartImplementation.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_auto_start_CLASS_NAME}Implementation.cxx"
    @ONLY)

  set("${_paraview_auto_start_INTERFACES}"
    "${_paraview_auto_start_CLASS_NAME}Implementation"
    PARENT_SCOPE)

  set("${_paraview_auto_start_SOURCES}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_auto_start_CLASS_NAME}Implementation.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_auto_start_CLASS_NAME}Implementation.h"
    PARENT_SCOPE)
endfunction ()

#[==[.md
### Tree layout strategy

TODO: What is a tree layout strategy?

```
paraview_plugin_add_tree_layout_strategy(
  STRATEGY_TYPE <type>
  STRATEGY_LABEL <label>
  INTERFACES <variable>
  SOURCES <variable>)
```

  * `STRATEGY_TYPE`: The name of the tree layout strategy class.
  * `STRATEGY_LABEL`: The label to use for the strategy.
  * `INTERFACES`: The name of the generated interface.
  * `SOURCES`: The source files generated by the interface.
#]==]
function (paraview_plugin_add_tree_layout_strategy)
  cmake_parse_arguments(_paraview_tree_layout_strategy
    ""
    "INTERFACES;SOURCES;STRATEGY_TYPE;STRATEGY_LABEL"
    ""
    ${ARGN})

  if (_paraview_tree_layout_strategy_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_add_tree_layout_strategy: "
      "${_paraview_tree_layout_strategy_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_tree_layout_strategy_STRATEGY_TYPE)
    message(FATAL_ERROR
      "The `STRATEGY_TYPE` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_tree_layout_strategy_STRATEGY_LABEL)
    message(FATAL_ERROR
      "The `STRATEGY_LABEL` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_tree_layout_strategy_INTERFACES)
    message(FATAL_ERROR
      "The `INTERFACES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_tree_layout_strategy_SOURCES)
    message(FATAL_ERROR
      "The `SOURCES` argument is required.")
  endif ()

  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqTreeLayoutStrategyImplementation.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_tree_layout_strategy_STRATEGY_TYPE}Implementation.h"
    @ONLY)
  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqTreeLayoutStrategyImplementation.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_tree_layout_strategy_STRATEGY_TYPE}Implementation.cxx"
    @ONLY)

  set("${_paraview_tree_layout_strategy_INTERFACES}"
    "${_paraview_tree_layout_strategy_STRATEGY_TYPE}Implementation"
    PARENT_SCOPE)

  set("${_paraview_tree_layout_strategy_SOURCES}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_tree_layout_strategy_STRATEGY_TYPE}Implementation.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_tree_layout_strategy_STRATEGY_TYPE}Implementation.h"
    PARENT_SCOPE)
endfunction ()

#[==[.md
### Proxy

TODO: What is a proxy?

```
paraview_plugin_add_proxy(
  NAME <name>
  INTERFACES <variable>
  SOURCES <variable>
  [PROXY_TYPE <type>
    [CLASS_NAME               <class>]
    XML_GROUP                 <group>
    <XML_NAME|XML_NAME_REGEX> <name>]...)
```

  * `NAME`: The name of the proxy.
  * `INTERFACES`: The name of the generated interface.
  * `SOURCES`: The source files generated by the interface.

At least one `PROXY_TYPE` must be specified. Each proxy type must be given an
`XML_GROUP` and either an `XML_NAME` or `XML_NAME_REGEX`. If `CLASS_NAME` is
not given, the `PROXY_TYPE` name is used instead.
#]==]
function (paraview_plugin_add_proxy)
  cmake_parse_arguments(_paraview_proxy
    ""
    "INTERFACES;SOURCES;NAME"
    ""
    ${ARGN})

  if (NOT DEFINED _paraview_proxy_INTERFACES)
    message(FATAL_ERROR
      "The `INTERFACES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_proxy_SOURCES)
    message(FATAL_ERROR
      "The `SOURCES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_proxy_NAME)
    message(FATAL_ERROR
      "The `NAME` argument is required.")
  endif ()

  set(_paraview_proxy_parse "")
  set(_paraview_proxy_type)
  set(_paraview_proxy_types)
  foreach (_paraview_proxy_arg IN LISTS _paraview_proxy_UNPARSED_ARGUMENTS)
    if (_paraview_proxy_parse STREQUAL "")
      set(_paraview_proxy_parse "${_paraview_proxy_arg}")
    elseif (_paraview_proxy_parse STREQUAL "PROXY_TYPE")
      set(_paraview_proxy_type "${_paraview_proxy_arg}")
      list(APPEND _paraview_proxy_types "${_paraview_proxy_type}")
      set(_paraview_proxy_parse "")
    elseif (_paraview_proxy_parse STREQUAL "CLASS_NAME")
      if (NOT _paraview_proxy_type)
        message(FATAL_ERROR
          "Missing `PROXY_TYPE` for `CLASS_NAME`")
      endif ()
      if (DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_class_name")
        message(FATAL_ERROR
          "Duplicate `CLASS_NAME` for `${_paraview_proxy_type}`")
      endif ()
      set("_paraview_proxy_type_${_paraview_proxy_type}_class_name"
        "${_paraview_proxy_arg}")
      set(_paraview_proxy_parse "")
    elseif (_paraview_proxy_parse STREQUAL "XML_GROUP")
      if (NOT _paraview_proxy_type)
        message(FATAL_ERROR
          "Missing `PROXY_TYPE` for `XML_GROUP`")
      endif ()
      if (DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_group")
        message(FATAL_ERROR
          "Duplicate `XML_GROUP` for `${_paraview_proxy_type}`")
      endif ()
      set("_paraview_proxy_type_${_paraview_proxy_type}_xml_group"
        "${_paraview_proxy_arg}")
      set(_paraview_proxy_parse "")
    elseif (_paraview_proxy_parse STREQUAL "XML_NAME")
      if (NOT _paraview_proxy_type)
        message(FATAL_ERROR
          "Missing `PROXY_TYPE` for `XML_NAME`")
      endif ()
      if (DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_name" OR
          DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_name_regex")
        message(FATAL_ERROR
          "Duplicate `XML_NAME` or `XML_NAME_REGEX` for `${_paraview_proxy_type}`")
      endif ()
      set("_paraview_proxy_type_${_paraview_proxy_type}_xml_name"
        "${_paraview_proxy_arg}")
      set(_paraview_proxy_parse "")
    elseif (_paraview_proxy_parse STREQUAL "XML_NAME_REGEX")
      if (NOT _paraview_proxy_type)
        message(FATAL_ERROR
          "Missing `PROXY_TYPE` for `XML_NAME_REGEX`")
      endif ()
      if (DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_name" OR
          DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_name_regex")
        message(FATAL_ERROR
          "Duplicate `XML_NAME` or `XML_NAME_REGEX` for `${_paraview_proxy_type}`")
      endif ()
      set("_paraview_proxy_type_${_paraview_proxy_type}_xml_name_regex"
        "${_paraview_proxy_arg}")
      set(_paraview_proxy_parse "")
    else ()
      message(FATAL_ERROR
        "Unknown argument `${_paraview_proxy_parse}`")
    endif ()
  endforeach ()

  if (_paraview_proxy_parse)
    message(FATAL_ERROR
      "Missing argument for `${_paraview_proxy_parse}`")
  endif ()

  if (NOT _paraview_proxy_types)
    message(FATAL_ERROR
      "No `PROXY_TYPE` arguments given")
  endif ()

  set(_paraview_proxy_includes)
  set(_paraview_proxy_body)
  foreach (_paraview_proxy_type IN LISTS _paraview_proxy_types)
    if (NOT DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_group")
      message(FATAL_ERROR
        "Missing `XML_GROUP` for `${_paraview_proxy_type}`")
    endif ()
    if (NOT DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_name" AND
        NOT DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_name_regex")
      message(FATAL_ERROR
        "Missing `XML_NAME` or `XML_NAME_REGEX` for `${_paraview_proxy_type}`")
    endif ()
    if (DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_class_name")
      set(_paraview_proxy_class
        "${_paraview_proxy_type_${_paraview_proxy_type}_class_name}")
    else ()
      set(_paraview_proxy_class
        "${_paraview_proxy_type}")
    endif ()

    set(_paraview_proxy_group "${_paraview_proxy_type_${_paraview_proxy_type}_xml_group}")
    if (DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_name")
      set(_paraview_proxy_name "${_paraview_proxy_type_${_paraview_proxy_type}_xml_name}")
      set(_paraview_proxy_name_type "QString")
      set(_paraview_proxy_cmp "name == proxy->GetXMLName()")
    else ()
      set(_paraview_proxy_name "${_paraview_proxy_type_${_paraview_proxy_type}_xml_name_regex}")
      set(_paraview_proxy_name_type "QRegularExpression")
      set(_paraview_proxy_cmp "QString(proxy->GetXMLName()).contains(name)")
    endif ()

    if (NOT DEFINED "_paraview_proxy_included_${_paraview_proxy_class}")
      string(APPEND _paraview_proxy_includes
        "#include \"${_paraview_proxy_class}.h\"\n")
      set("_paraview_proxy_included_${_paraview_proxy_class}" 1)
    endif ()
    string(APPEND _paraview_proxy_body
      "  {
    static const QString group(\"${_paraview_proxy_group}\");
    static const ${_paraview_proxy_name_type} name(\"${_paraview_proxy_name}\");
    if (group == proxy->GetXMLGroup() && ${_paraview_proxy_cmp})
    {
      return new ${_paraview_proxy_class}(regGroup, regName, proxy, server, nullptr);
    }
  }\n")
  endforeach ()

  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqServerManagerModelImplementation.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_proxy_NAME}ServerManagerModelImplementation.h"
    @ONLY)
  configure_file(
    "${_ParaViewPlugin_cmake_dir}/pqServerManagerModelImplementation.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_proxy_NAME}ServerManagerModelImplementation.cxx"
    @ONLY)

  set("${_paraview_proxy_INTERFACES}"
    "${_paraview_proxy_NAME}ServerManagerModelImplementation"
    PARENT_SCOPE)

  set("${_paraview_proxy_SOURCES}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_proxy_NAME}ServerManagerModelImplementation.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_proxy_NAME}ServerManagerModelImplementation.h"
    PARENT_SCOPE)
endfunction ()
