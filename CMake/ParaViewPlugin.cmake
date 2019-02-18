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
  [REQUIRES_MODULES         <variable>])
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

    if (NOT ${_paraview_scan_plugin_name}_REQUIRES_MODULES)
      message(WARNING
        "The ${_paraview_scan_plugin_name} plugin claims that it does not "
        "require any modules. This is probably an oversight.")
    endif ()

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

#[==[.md
## Building plugins

Once all plugins have been scanned, they need to be built.

```
paraview_plugin_build(
  PLUGINS <plugin>...
  [TARGET <target>]
  [AUTOLOAD <plugin>...]

  [RUNTIME_DESTINATION <destination>]
  [LIBRARY_DESTINATION <destination>]
  [LIBRARY_SUBDIRECTORY <subdirectory>]

  [PLUGINS_FILE_NAME <filename>])
```

  * `PLUGINS`: (Required) The list of plugins to build. May be empty.
  * `TARGET`: (Recommended) The name of an interface target to generate. This
    provides. an initialization function `<TARGET>_initialize` which
    initializes static plugins. The function is provided, but is a no-op for
    shared plugin builds.
  * `AUTOLOAD`: A list of plugins to mark for autoloading.
  * `RUNTIME_DESTINATION`: (Defaults to `${CMAKE_INSTALL_BINDIR}`) Where to
    install runtime files.
  * `LIBRARY_DESTINATION`: (Defaults to `${CMAKE_INSTALL_LIBDIR}`) Where to
    install modules built by plugins.
  * `LIBRARY_SUBDIRECTORY`: (Defaults to `""`) Where to install the plugins
    themselves. Each plugin lives in a directory of its name in
    `<RUNTIME_DESTINATION>/<LIBRARY_SUBDIRECTORY>` (for Windows) or
    `<LIBRARY_DESTINATION>/<LIBRARY_SUBDIRECTORY>` for other platforms.
  * `PLUGINS_FILE_NAME`: The name of the XML plugin file to generate for the
    built plugins. This file will be placed under
    `<LIBRARY_DESTINATION>/<LIBRARY_SUBDIRECTORY>`. It will be installed with
    the `plugin` component.
#]==]
function (paraview_plugin_build)
  cmake_parse_arguments(_paraview_build
    ""
    "RUNTIME_DESTINATION;LIBRARY_DESTINATION;LIBRARY_SUBDIRECTORY;TARGET;PLUGINS_FILE_NAME"
    "PLUGINS;AUTOLOAD"
    ${ARGN})

  if (_paraview_build_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_plugin_build: "
      "${_paraview_build_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_build_RUNTIME_DESTINATION)
    set(_paraview_build_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif ()

  if (NOT DEFINED _paraview_build_LIBRARY_DESTINATION)
    set(_paraview_build_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  endif ()

  if (NOT DEFINED _paraview_build_LIBRARY_SUBDIRECTORY)
    set(_paraview_build_LIBRARY_SUBDIRECTORY "")
  endif ()

  if (WIN32)
    set(_paraview_build_plugin_destination "${_paraview_build_RUNTIME_DESTINATION}")
  else ()
    set(_paraview_build_plugin_destination "${_paraview_build_LIBRARY_DESTINATION}")
  endif ()
  string(APPEND _paraview_build_plugin_destination "/${_paraview_build_LIBRARY_SUBDIRECTORY}")

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
    add_library("${_paraview_build_TARGET}" INTERFACE)
    target_include_directories("${_paraview_build_TARGET}"
      INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_paraview_build_TARGET}>")
    set(_paraview_build_include_file
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_paraview_build_TARGET}/${_paraview_build_TARGET}.h")

    if (BUILD_SHARED_LIBS)
      set(_paraview_build_include_content
        "#ifndef ${_paraview_build_TARGET}_h
#define ${_paraview_build_TARGET}_h

void ${_paraview_build_TARGET}_initialize()
{
}

#endif\n")
    else ()
      target_link_libraries("${_paraview_build_TARGET}"
        INTERFACE
          ParaView::ClientServerCoreCore
          ${_paraview_build_PLUGINS})

      set(_paraview_build_declarations)
      set(_paraview_build_calls)
      foreach (_paraview_build_plugin IN LISTS _paraview_build_PLUGINS)
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
      endforeach ()

      set(_paraview_build_include_content
        "#ifndef ${_paraview_build_TARGET}_h
#define ${_paraview_build_TARGET}_h

#include \"vtkPVPlugin.h\"
#include \"vtkPVPluginLoader.h\"
#include \"vtkPVPluginTracker.h\"
#include <string>

${_paraview_build_declarations}
static bool ${_paraview_build_TARGET}_static_plugins_load(const char* name);
static bool ${_paraview_build_TARGET}_static_plugins_search(const char* name);

void ${_paraview_build_TARGET}_initialize()
{
  vtkPVPluginLoader::RegisterLoadPluginCallback(${_paraview_build_TARGET}_static_plugins_load);
  vtkPVPluginTracker::SetStaticPluginSearchFunction(${_paraview_build_TARGET}_static_plugins_search);
}

static bool ${_paraview_build_TARGET}_static_plugins_func(const char* name, bool load);

bool ${_paraview_build_TARGET}_static_plugins_load(const char* name)
{
  return ${_paraview_build_TARGET}_static_plugins_func(name, true);
}

bool ${_paraview_build_TARGET}_static_plugins_search(const char* name)
{
  return ${_paraview_build_TARGET}_static_plugins_func(name, false);
}

bool ${_paraview_build_TARGET}_static_plugins_func(const char* name, bool load)
{
  std::string sname = name;

  ${_paraview_build_calls}
  return false;
}

#endif\n")
    endif ()

    file(GENERATE
      OUTPUT  "${_paraview_build_include_file}"
      CONTENT "${_paraview_build_include_content}")
  endif ()

  if (DEFINED _paraview_build_PLUGINS_FILE_NAME)
    set(_paraview_build_xml_file
      "${CMAKE_BINARY_DIR}/${_paraview_build_plugin_destination}/${_paraview_build_PLUGINS_FILE_NAME}")
    set(_paraview_build_xml_content
      "<?xml version=\"1.0\"?>\n<Plugins>\n")
    foreach (_paraview_build_plugin IN LISTS _paraview_build_PLUGINS)
      set(_paraview_build_autoload 0)
      list(FIND _paraview_build_AUTOLOAD "${_paraview_build_plugin}" _paraview_build_idx)
      if (NOT _paraview_build_idx EQUAL -1)
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
      COMPONENT   "plugin")
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

  [MODULES <module>...]
  [SOURCES <source>...]
  [SERVER_MANAGER_XML <xml>...]

  [UI_INTERFACES <interface>...]
  [UI_RESOURCES <resource>...]
  [UI_FILES <file>...]

  [PYTHON_MODULES <module>...]

  [REQUIRED_PLUGINS <plugin>...]

  [EULA <eula>]
  [XML_DOCUMENTATION <ON|OFF>]
  [DOCUMENTATION_DIR <directory>]

  [EXPORT <export>])
```

  * `REQUIRED_ON_SERVER`: The plugin is required to be loaded on the server for
    proper functionality.
  * `REQUIRED_ON_CLIENT`: The plugin is required to be loaded on the client for
    proper functionality.
  * `VERSION`: (Required) The version number of the plugin.
  * `MODULES`: Modules to include in the plugin. These modules will be wrapped
    using client server and have their server manager XML files processed.
  * `SOURCES`: Source files for the plugin.
  * `SERVER_MANAGER_XML`: Server manager XML files for the plugin.
  * `UI_INTERFACES`: Interfaces to initialize. See the plugin interfaces
    section for more details.
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
  * `DOCUMENTATION_DIR`: (Defaults to `${CMAKE_CURRENT_SOURCE_DIR}`) Where to
    look for documentation files.
  * `EXPORT`: If provided, the plugin will be added to the given export set.
#]==]
function (paraview_add_plugin name)
  if (NOT name STREQUAL _paraview_build_plugin)
    message(FATAL_ERROR
      "The ${_paraview_build_plugin}'s CMakeLists.txt may not add the ${name} "
      "plugin.")
  endif ()

  cmake_parse_arguments(_paraview_add_plugin
    "REQUIRED_ON_SERVER;REQUIRED_ON_CLIENT"
    "VERSION;EULA;EXPORT;XML_DOCUMENTATION;DOCUMENTATION_DIR"
    "REQUIRED_PLUGINS;SERVER_MANAGER_XML;SOURCES;MODULES;UI_INTERFACES;UI_RESOURCES;UI_FILES;PYTHON_MODULES"
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

  if (NOT DEFINED _paraview_add_plugin_DOCUMENTATION_DIR)
    set(_paraview_add_plugin_DOCUMENTATION_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}")
  elseif (NOT _paraview_add_plugin_XML_DOCUMENTATION)
    message(FATAL_ERROR
      "Specifying `DOCUMENTATION_DIR` and turning off `XML_DOCUMENTATION` "
      "makes no sense.")
  endif ()

  # TODO: resource initialization for static builds
  # WITH_PYTHON

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

  set(_paraview_add_plugin_includes)
  set(_paraview_add_plugin_required_libraries)

  set(_paraview_add_plugin_module_xmls)
  set(_paraview_add_plugin_with_xml 0)
  if (_paraview_add_plugin_MODULES)
    set(_paraview_add_plugin_with_xml 1)

    list(APPEND _paraview_add_plugin_required_libraries
      ${_paraview_add_plugin_MODULES})

    set(_paraview_add_plugin_cs_args)
    if (_paraview_add_plugin_EXPORT)
      list(APPEND _paraview_add_plugin_cs_args
        INSTALL_EXPORT "${_paraview_add_plugin_EXPORT}")
    endif ()

    vtk_module_wrap_client_server(
      MODULES ${_paraview_add_plugin_MODULES}
      TARGET  "${_paraview_build_plugin}_client_server"
      ${_paraview_add_plugin_cs_args})
    paraview_server_manager_process(
      MODULES   ${_paraview_add_plugin_MODULES}
      TARGET    "${_paraview_build_plugin}_server_manager_modules"
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
      FILES     ${_paraview_add_plugin_xmls})
    list(APPEND _paraview_add_plugin_required_libraries
      "${_paraview_build_plugin}_server_manager")
  endif ()

  if ((_paraview_add_plugin_module_xmls OR _paraview_add_plugin_xmls) AND
      PARAVIEW_BUILD_QT_GUI AND _paraview_add_plugin_XML_DOCUMENTATION)
    paraview_client_documentation(
      TARGET  "${_paraview_build_plugin}_doc"
      XMLS    ${_paraview_add_plugin_module_xmls}
              ${_paraview_add_plugin_xmls})
    paraview_client_generate_help(
      NAME        "${_paraview_build_plugin}"
      TARGET      "${_paraview_build_plugin}_qch"
      SOURCE_DIR  "${_paraview_add_plugin_DOCUMENTATION_DIR}"
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
      COMMAND ParaView::ProcessXML
              -base64
              "${_paraview_add_plugin_qch_output}"
              \"\"
              "_qch"
              "_qch"
              "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_build_plugin}.qch"
      DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_build_plugin}.qch"
              "${_paraview_build_plugin}_qch"
              ParaView::ProcessXML
      COMMENT "Generating header for ${_paraview_build_plugin} documentation")
    set_property(SOURCE "${_paraview_add_plugin_qch_output}"
      PROPERTY
        SKIP_AUTOMOC 1)

    string(APPEND _paraview_add_plugin_includes
      "#include \"${_paraview_build_plugin}_qch.h\"\n")
    string(APPEND _paraview_add_plugin_binary_resources
      "  {
    const char *text = ${_paraview_build_plugin}_qch();
    resources.push_back(text);
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
        "  arg.push_back(new ${_paraview_add_plugin_ui_interface}(this)); \\\n")
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
    if (NOT BUILD_SHARED_LIBS)
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
        COMMAND ParaView::ProcessXML
                "${_paraview_add_plugin_python_header}"
                "module_${_paraview_add_plugin_python_module_mangled}_"
                "_string"
                "_source"
                "${_paraview_add_plugin_python_path}"
        DEPENDS "${_paraview_add_plugin_python_path}"
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
  if (DEFINED _paraview_build_LIBRARY_SUBDIRECTORY)
    string(APPEND CMAKE_LIBRARY_OUTPUT_DIRECTORY "/${_paraview_build_LIBRARY_SUBDIRECTORY}")
  endif ()
  string(APPEND CMAKE_LIBRARY_OUTPUT_DIRECTORY "/${_paraview_build_plugin}")

  set(_paraview_build_plugin_type MODULE)
  if (NOT BUILD_SHARED_LIBS)
    set(_paraview_build_plugin_type STATIC)
  endif ()

  add_library("${_paraview_build_plugin}" "${_paraview_build_plugin_type}"
    ${_paraview_add_plugin_header}
    ${_paraview_add_plugin_source}
    ${_paraview_add_plugin_eula_sources}
    ${_paraview_add_plugin_binary_headers}
    ${_paraview_add_plugin_ui_sources}
    ${_paraview_add_plugin_python_sources}
    ${_paraview_add_plugin_SOURCES})
  if (NOT BUILD_SHARED_LIBS)
    target_compile_definitions("${_paraview_build_plugin}"
      PRIVATE
        QT_STATICPLUGIN)
  endif ()
  target_link_libraries("${_paraview_build_plugin}"
    PRIVATE
      ParaView::ClientServerCoreCore
      ${_paraview_add_plugin_required_libraries})
  if (_paraview_add_plugin_UI_INTERFACES)
    target_include_directories("${_paraview_build_plugin}"
      PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}")
  endif ()
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
    COMPONENT "plugin"
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
    XML_GROUP                 <group>
    <XML_NAME|XML_NAME_REGEX> <name>]...)
```

  * `NAME`: The name of the proxy.
  * `INTERFACES`: The name of the generated interface.
  * `SOURCES`: The source files generated by the interface.

At least one `PROXY_TYPE` must be specified. Each proxy type must be given an
`XML_GROUP` and either an `XML_NAME` or `XML_NAME_REGEX`.
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
    if (NOT DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_name" OR
        NOT DEFINED "_paraview_proxy_type_${_paraview_proxy_type}_xml_name_regex")
      message(FATAL_ERROR
        "Missing `XML_NAME` or `XML_NAME_REGEX` for `${_paraview_proxy_type}`")
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

    string(APPEND _paraview_proxy_includes
      "#include \"${_paraview_proxy_type}.h\"\n")
    string(APPEND _paraview_proxy_body
      "  {
    static const QString group(\"${_paraview_proxy_group}\");
    static const ${_paraview_proxy_name_type} name(\"${_paraview_proxy_name}\");
    if (group == proxy->GetXMLGroup() && ${_paraview_proxy_cmp})
    {
      return new ${_paraview_proxy_type}(regGroup, regName, proxy, server, nullptr);
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
