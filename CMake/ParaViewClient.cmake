set(_ParaViewClient_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")
set(_ParaViewClient_script_file "${CMAKE_CURRENT_LIST_FILE}")

#[==[.md
## Building a client

TODO: Document

```
paraview_client_add(
  NAME    <name>
  VERSION <version>
  SOURCES <source>...
  [APPLICATION_XMLS <xml>...]
  [QCH_FILES <file>...]

  [MAIN_WINDOW_CLASS    <class>]
  [MAIN_WINDOW_INCLUDE  <include>]

  [PLUGINS_TARGETS  <target>...]
  [REQUIRED_PLUGINS <plugin>...]
  [OPTIONAL_PLUGINS <plugin>...]

  [APPLICATION_NAME <name>]
  [ORGANIZATION     <organization>]
  [TITLE            <title>]

  [DEFAULT_STYLE  <style>]

  [APPLICATION_ICON <icon>]
  [BUNDLE_ICON      <icon>]
  [BUNDLE_PLIST     <plist>]
  [SPLASH_IMAGE     <image>]

  [NAMESPACE            <namespace>]
  [EXPORT               <export>]
  [FORCE_UNIX_LAYOUT    <ON|OFF>]
  [BUNDLE_DESTINATION   <directory>]
  [RUNTIME_DESTINATION  <directory>]
  [LIBRARY_DESTINATION  <directory>])
```

  * `NAME`: (Required) The name of the application. This is used as the target
    name as well.
  * `VERSION`: (Required) The version of the application.
  * `SOURCES`: (Required) Source files for the application.
  * `APPLICATION_XMLS`: Server manager XML files.
  * `QCH_FILES`: Any `.qch` files containing documentation.
  * `MAIN_WINDOW_CLASS`: (Defaults to `QMainWindow`) The name of the main
    window class.
  * `MAIN_WINDOW_INCLUDE`: (Defaults to `QMainWindow` or
    `<MAIN_WINDOW_CLASS>.h` if it is specified) The include file for the main
    window.
  * `PLUGINS_TARGETS`: The targets for plugins. The associated functions
    will be called upon startup.
  * `REQUIRED_PLUGINS`: Plugins to load upon startup.
  * `OPTIONAL_PLUGINS`: Plugins to load upon startup if available.
  * `APPLICATION_NAME`: (Defaults to `<NAME>`) The displayed name of the
    application.
  * `ORGANIZATION`: (Defaults to `Anonymous`) The organization for the
    application. This is used for the macOS GUI identifier.
  * `TITLE`: The window title for the application.
  * `DEFAULT_STYLE`: The default Qt style for the application.
  * `APPLICATION_ICON`: The path to the icon for the Windows application.
  * `BUNDLE_ICON`: The path to the icon for the macOS bundle.
  * `BUNDLE_PLIST`: The path to the `Info.plist.in` template.
  * `SPLASH_IMAGE`: The image to display upon startup.
  * `NAMESPACE`: If provided, an alias target `<NAMESPACE>::<NAME>` will be
    created.
  * `EXPORT`: If provided, the target will be exported.
  * `FORCE_UNIX_LAYOUT`: (Defaults to `OFF`) Forces a Unix-style layout even on
    platforms for which they are not the norm for GUI applications (e.g.,
    macOS).
  * `BUNDLE_DESTINATION`: (Defaults to `Applications`) Where to place the
    bundle executable.
  * `RUNTIME_DESTINATION`: (Defaults to `${CMAKE_INSTALL_BINDIR}`) Where to
    place the binary.
  * `LIBRARY_DESTINATION`: (Defaults to `${CMAKE_INSTALL_LIBDIR}`) Where
    libraries are placed. Sets up `RPATH` on ELF platforms (e.g., Linux and the
    BSD family).
#]==]
function (paraview_client_add)
  cmake_parse_arguments(_paraview_client
    ""
    "NAME;APPLICATION_NAME;ORGANIZATION;TITLE;SPLASH_IMAGE;BUNDLE_DESTINATION;BUNDLE_ICON;BUNDLE_PLIST;APPLICATION_ICON;MAIN_WINDOW_CLASS;MAIN_WINDOW_INCLUDE;VERSION;FORCE_UNIX_LAYOUT;PLUGINS_TARGET;DEFAULT_STYLE;RUNTIME_DESTINATION;LIBRARY_DESTINATION;NAMESPACE;EXPORT"
    "REQUIRED_PLUGINS;OPTIONAL_PLUGINS;APPLICATION_XMLS;SOURCES;QCH_FILES;QCH_FILE;PLUGINS_TARGETS"
    ${ARGN})

  if (_paraview_client_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_client_add: "
      "${_paraview_client_UNPARSED_ARGUMENTS}")
  endif ()

  # TODO: Installation.

  if (DEFINED _paraview_client_PLUGINS_TARGET)
    if (DEFINED _paraview_client_PLUGINS_TARGETS)
      message(FATAL_ERROR
        "The `paraview_client_add(PLUGINS_TARGET)` argument is incompatible "
        "with `PLUGINS_TARGETS`.")
    else ()
      message(DEPRECATION
        "The `paraview_client_add(PLUGINS_TARGET)` argument is deprecated in "
        "favor of `PLUGINS_TARGETS`.")
      set(_paraview_client_PLUGINS_TARGETS
        "${_paraview_client_PLUGINS_TARGET}")
    endif ()
  endif ()

  if (NOT DEFINED _paraview_client_NAME)
    message(FATAL_ERROR
      "The `NAME` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_VERSION)
    message(FATAL_ERROR
      "The `VERSION` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_SOURCES)
    message(FATAL_ERROR
      "The `SOURCES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_APPLICATION_NAME)
    set(_paraview_client_APPLICATION_NAME
      "${_paraview_client_NAME}")
  endif ()

  if (NOT DEFINED _paraview_client_ORGANIZATION)
    set(_paraview_client_ORGANIZATION
      "Anonymous")
  endif ()

  if (NOT DEFINED _paraview_client_FORCE_UNIX_LAYOUT)
    set(_paraview_client_FORCE_UNIX_LAYOUT
      OFF)
  endif ()

  if (NOT DEFINED _paraview_client_BUNDLE_DESTINATION)
    set(_paraview_client_BUNDLE_DESTINATION
      "Applications")
  endif ()

  if (NOT DEFINED _paraview_client_RUNTIME_DESTINATION)
    set(_paraview_client_RUNTIME_DESTINATION
      "${CMAKE_INSTALL_BINDIR}")
  endif ()

  if (NOT DEFINED _paraview_client_LIBRARY_DESTINATION)
    set(_paraview_client_LIBRARY_DESTINATION
      "${CMAKE_INSTALL_LIBDIR}")
  endif ()

  if (DEFINED _paraview_client_QCH_FILE)
    if (DEFINED _paraview_client_QCH_FILES)
      message(FATAL_ERROR
        "The `paraview_client_add(QCH_FILE)` argument is incompatible "
        "with `QCH_FILES`.")
    else ()
      message(DEPRECATION
        "The `paraview_client_add(QCH_FILE)` argument is deprecated in "
        "favor of `QCH_FILES`.")
      set(_paraview_client_QCH_FILES
        "${_paraview_client_QCH_FILE}")
    endif ()
  endif ()

  if (NOT DEFINED _paraview_client_MAIN_WINDOW_CLASS)
    if (DEFINED _paraview_client_MAIN_WINDOW_INCLUDE)
      message(FATAL_ERROR
        "The `MAIN_WINDOW_INCLUDE` argument cannot be specified without "
        "`MAIN_WINDOW_CLASS`.")
    endif ()

    set(_paraview_client_MAIN_WINDOW_CLASS
      "QMainWindow")
    set(_paraview_client_MAIN_WINDOW_INCLUDE
      "QMainWindow")
  endif ()

  if (NOT DEFINED _paraview_client_MAIN_WINDOW_INCLUDE)
    set(_paraview_client_MAIN_WINDOW_INCLUDE
      "${_paraview_client_MAIN_WINDOW_CLASS}.h")
  endif ()

  set(_paraview_client_extra_sources)
  set(_paraview_client_bundle_args)

  set(_paraview_client_executable_flags)
  if (WIN32)
    if (DEFINED _paraview_client_APPLICATION_ICON)
      set(_paraview_client_appicon_file
        "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_client_NAME}_appicon.rc")
      file(WRITE "${_paraview_client_appicon_file}.tmp"
        "// Icon with the lowest ID value placed first to ensure that the application
// icon remains consistent on all systems.
IDI_ICON1 ICON \"${_paraview_client_APPLICATION_ICON}\"\n")
      configure_file(
        "${_paraview_client_appicon_file}.tmp"
        "${_paraview_client_appicon_file}"
        COPYONLY)

      list(APPEND _paraview_client_extra_sources
        "${_paraview_client_appicon_file}")
    endif ()

    list(APPEND _paraview_client_executable_flags
      WIN32)
  elseif (APPLE)
    # TODO: nib files

    list(APPEND _paraview_client_bundle_args
      BUNDLE DESTINATION "${_paraview_client_BUNDLE_DESTINATION}")
    list(APPEND _paraview_client_executable_flags
      MACOSX_BUNDLE)
  endif ()

  set(_paraview_client_resource_files "")
  set(_paraview_client_resource_init "")

  if (DEFINED _paraview_client_SPLASH_IMAGE)
    set(_paraview_client_splash_base_name
      "${_paraview_client_NAME}_splash")
    set(_paraview_client_splash_image_name
      "${_paraview_client_splash_base_name}.img")
    set(_paraview_client_splash_resource
      ":/${_paraview_client_NAME}/${_paraview_client_splash_base_name}")

    set(_paraview_client_splash_resource_file
      "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_client_splash_base_name}.qrc")

    paraview_client_qt_resource(
      OUTPUT  "${_paraview_client_splash_resource_file}"
      PREFIX  "/${_paraview_client_NAME}"
      ALIAS   "${_paraview_client_splash_base_name}"
      FILE    "${_paraview_client_SPLASH_IMAGE}")

    list(APPEND _paraview_client_resource_files
      "${_paraview_client_splash_resource_file}")
    string(APPEND _paraview_client_resource_init
      "  Q_INIT_RESOURCE(${_paraview_client_splash_base_name});\n")
    set(CMAKE_AUTORCC 1)
  endif ()

  if (DEFINED _paraview_client_APPLICATION_XMLS)
    set(_paraview_client_application_base_name
      "${_paraview_client_NAME}_configuration")
    set(_paraview_client_application_resource_file
      "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_client_application_base_name}.qrc")

    paraview_client_qt_resources(
      OUTPUT  "${_paraview_client_application_resource_file}"
      PREFIX  "/${_paraview_client_NAME}/Configuration"
      FILES   "${_paraview_client_APPLICATION_XMLS}")

    list(APPEND _paraview_client_resource_files
      "${_paraview_client_application_resource_file}")
    string(APPEND _paraview_client_resource_init
      "  Q_INIT_RESOURCE(${_paraview_client_application_base_name});\n")
    set(CMAKE_AUTORCC 1)
  endif ()

  if (DEFINED _paraview_client_QCH_FILES)
    set(_paraview_client_documentation_base_name
      "${_paraview_client_NAME}_documentation")
    set(_paraview_client_documentation_resource_file
      "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_client_documentation_base_name}.qrc")

    paraview_client_qt_resources(
      OUTPUT  "${_paraview_client_documentation_resource_file}"
      # This prefix is part of the API.
      PREFIX  "/${_paraview_client_NAME}/Documentation"
      FILES   ${_paraview_client_QCH_FILES})
    set_property(SOURCE "${_paraview_client_documentation_resource_file}"
      PROPERTY
        OBJECT_DEPENDS "${_paraview_client_QCH_FILES}")

    list(APPEND _paraview_client_resource_files
      "${_paraview_client_documentation_resource_file}")
    string(APPEND _paraview_client_resource_init
      "  Q_INIT_RESOURCE(${_paraview_client_documentation_base_name});\n")
    set(CMAKE_AUTORCC 1)
  endif ()

  include("${_ParaViewClient_cmake_dir}/paraview-find-package-helpers.cmake" OPTIONAL)
  find_package(Qt5 REQUIRED QUIET COMPONENTS Core Widgets)

  # CMake 3.13 started using Qt5's version variables to detect what version
  # of Qt's tools to run for autorcc. However, they are looked up using the
  # target's directory scope, but these are here in a local scope and unset
  # when AutoGen gets around to asking about the variables at generate time.

  # Fix for 3.13.0–3.13.3. Does not work if `paraview_client_add` is called
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

  set(_paraview_client_built_shared 0)
  if (BUILD_SHARED_LIBS)
    set(_paraview_client_built_shared 1)
  endif ()

  set(_paraview_client_have_plugins 0)
  set(_paraview_client_plugins_includes)
  set(_paraview_client_plugins_calls)
  if (_paraview_client_PLUGINS_TARGETS)
    set(_paraview_client_have_plugins 1)
    foreach (_paraview_client_plugin_target IN LISTS _paraview_client_PLUGINS_TARGETS)
      string(REPLACE "::" "_" _paraview_client_plugin_target_safe "${_paraview_client_plugin_target}")
      string(APPEND _paraview_client_plugins_includes
        "#include \"${_paraview_client_plugin_target_safe}.h\"\n")
      string(APPEND _paraview_client_plugins_calls
        "  ${_paraview_client_plugin_target_safe}_initialize();\n")
    endforeach ()
  endif ()

  set(_paraview_client_source_files
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_client_NAME}_main.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/pq${_paraview_client_NAME}Initializer.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/pq${_paraview_client_NAME}Initializer.h")
  configure_file(
    "${_ParaViewClient_cmake_dir}/paraview_client_main.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_paraview_client_NAME}_main.cxx"
    @ONLY)
  configure_file(
    "${_ParaViewClient_cmake_dir}/paraview_client_initializer.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pq${_paraview_client_NAME}Initializer.cxx"
    @ONLY)
  configure_file(
    "${_ParaViewClient_cmake_dir}/paraview_client_initializer.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pq${_paraview_client_NAME}Initializer.h"
    @ONLY)

  # Set up rpaths
  set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
  if (UNIX AND NOT APPLE)
    file(RELATIVE_PATH _paraview_client_relpath
      "/prefix/${_paraview_client_RUNTIME_DESTINATION}"
      "/prefix/${_paraview_client_LIBRARY_DESTINATION}")
    set(_paraview_client_origin_rpath
      "$ORIGIN/${_paraview_client_relpath}")

    list(APPEND CMAKE_INSTALL_RPATH
      "${_paraview_client_origin_rpath}")
  endif ()

  if (_paraview_client_resource_files)
    source_group("resources"
      FILES
        ${_paraview_client_resource_files})
  endif ()
  add_executable("${_paraview_client_NAME}" ${_paraview_client_executable_flags}
    ${_paraview_client_SOURCES}
    ${_paraview_client_resource_files}
    ${_paraview_client_source_files}
    ${_paraview_client_extra_sources})
  if (DEFINED _paraview_client_NAMESPACE)
    add_executable("${_paraview_client_NAMESPACE}::${_paraview_client_NAME}" ALIAS "${_paraview_client_NAME}")
  endif ()
  target_include_directories("${_paraview_client_NAME}"
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${CMAKE_CURRENT_BINARY_DIR}"
      # https://gitlab.kitware.com/cmake/cmake/-/issues/18049
      "$<TARGET_PROPERTY:VTK::vtksys,INTERFACE_INCLUDE_DIRECTORIES>")
  target_link_libraries("${_paraview_client_NAME}"
    PRIVATE
      ParaView::pqApplicationComponents
      Qt5::Widgets
      VTK::vtksys)
  if (PARAVIEW_USE_QTWEBENGINE)
    find_package(Qt5 REQUIRED QUIET COMPONENTS WebEngineWidgets)
    target_link_libraries("${_paraview_client_NAME}"
      PRIVATE Qt5::WebEngineWidgets)
  endif ()

  set(_paraview_client_export)
  if (DEFINED _paraview_client_EXPORT)
    list(APPEND _paraview_client_export
      EXPORT "${_paraview_client_EXPORT}")
  endif ()

  install(
    TARGETS "${_paraview_client_NAME}"
    ${_paraview_client_export}
    COMPONENT "runtime"
    ${_paraview_client_bundle_args}
    RUNTIME DESTINATION "${_paraview_client_RUNTIME_DESTINATION}")

  if (DEFINED _paraview_client_PLUGINS_TARGETS)
    target_link_libraries("${_paraview_client_NAME}"
      PRIVATE
        ${_paraview_client_PLUGINS_TARGETS})

    set(_paraview_client_binary_destination
      "${_paraview_client_RUNTIME_DESTINATION}")
    set(_paraview_client_conf_destination
      "${_paraview_client_binary_destination}")
    if (APPLE)
      string(APPEND _paraview_client_binary_destination
        "/${_paraview_client_NAME}.app/Contents/Resources")
      set(_paraview_client_conf_destination
        "${_paraview_client_BUNDLE_DESTINATION}/${_paraview_client_NAME}.app/Contents/Resources")
    endif ()

    paraview_plugin_write_conf(
      NAME            "${_paraview_client_NAME}"
      PLUGINS_TARGETS ${_paraview_client_PLUGINS_TARGETS}
      BUILD_DESTINATION   "${_paraview_client_binary_destination}"
      INSTALL_DESTINATION "${_paraview_client_conf_destination}"
      COMPONENT "runtime")
  endif ()

  if (APPLE)
    if (DEFINED _paraview_client_BUNDLE_ICON)
      get_filename_component(_paraview_client_bundle_icon_file "${_paraview_client_BUNDLE_ICON}" NAME)
      set_property(TARGET "${_paraview_client_NAME}"
        PROPERTY
          MACOSX_BUNDLE_ICON_FILE "${_paraview_client_bundle_icon_file}")
      install(
        FILES       "${_paraview_client_BUNDLE_ICON}"
        DESTINATION "${_paraview_client_BUNDLE_DESTINATION}/${_paraview_client_APPLICATION_NAME}.app/Contents/Resources"
        COMPONENT   "runtime")
    endif ()
    if (DEFINED _paraview_client_BUNDLE_PLIST)
      set_property(TARGET "${_paraview_client_NAME}"
        PROPERTY
          MACOSX_BUNDLE_INFO_PLIST "${_paraview_client_BUNDLE_PLIST}")
    endif ()
    string(TOLOWER "${_paraview_client_ORGANIZATION}" _paraview_client_organization)
    set_target_properties("${_paraview_client_NAME}"
      PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME           "${_paraview_client_APPLICATION_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER        "org.${_paraview_client_organization}.${_paraview_client_APPLICATION_NAME}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING  "${_paraview_client_VERSION}")
  endif ()
endfunction ()

#[==[.md INTERNAL
## Quoting

Passing CMake lists down to the help generation and proxy documentation steps
requires escaping the `;` in them. These functions escape and unescape the
variable passed in. The new value is placed in the same variable in the calling
scope.
#]==]

function (_paraview_client_escape_cmake_list variable)
  string(REPLACE "_" "_u" _escape_tmp "${${variable}}")
  string(REPLACE ";" "_s" _escape_tmp "${_escape_tmp}")
  set("${variable}"
    "${_escape_tmp}"
    PARENT_SCOPE)
endfunction ()

function (_paraview_client_unescape_cmake_list variable)
  string(REPLACE "_s" ";" _escape_tmp "${${variable}}")
  string(REPLACE "_u" "_" _escape_tmp "${_escape_tmp}")
  set("${variable}"
    "${_escape_tmp}"
    PARENT_SCOPE)
endfunction ()

#[==[.md
## Documentation from XML files

Documentation can be generated from server manager XML files. The
`paraview_client_documentation` generates Qt help, HTML, and Wiki documentation
from them.

```
paraview_client_documentation(
  TARGET  <target>
  XMLS    <xml>...
  [OUTPUT_DIR <directory>])
```

  * `TARGET`: (Required) The name of the target to generate.
  * `XMLS`: (Required) The list of XML files to process.
  * `OUTPUT_DIR`: (Defaults to `${CMAKE_CURRENT_BINARY_DIR}`) Where to place
    generated documentation.
#]==]
function (paraview_client_documentation)
  cmake_parse_arguments(_paraview_client_doc
    ""
    "TARGET;OUTPUT_DIR"
    "XMLS"
    ${ARGN})

  if (_paraview_client_doc_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_client_documentation: "
      "${_paraview_client_doc_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_client_doc_OUTPUT_DIR)
    set(_paraview_client_doc_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  endif ()

  if (NOT DEFINED _paraview_client_doc_TARGET)
    message(FATAL_ERROR
      "The `TARGET` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_doc_XMLS)
    message(FATAL_ERROR
      "The `XMLS` argument is required.")
  endif ()

  include("${_ParaViewClient_cmake_dir}/paraview-find-package-helpers.cmake" OPTIONAL)
  find_program(qt_xmlpatterns_executable
    NAMES xmlpatterns-qt5 xmlpatterns
    HINTS "${Qt5_DIR}/../../../bin"
          "${Qt5_DIR}/../../../libexec/qt5/bin"
    DOC   "Path to xmlpatterns")
  mark_as_advanced(qt_xmlpatterns_executable)

  if (NOT qt_xmlpatterns_executable)
    message(FATAL_ERROR
      "Cannot find the xmlpatterns executable.")
  endif ()

  set(_paraview_client_doc_xmls)
  foreach (_paraview_client_doc_xml IN LISTS _paraview_client_doc_XMLS)
    get_filename_component(_paraview_client_doc_xml "${_paraview_client_doc_xml}" ABSOLUTE)
    list(APPEND _paraview_client_doc_xmls
      "${_paraview_client_doc_xml}")
  endforeach ()

  # Save xmls to a temporary file.
  set (_paraview_client_doc_xmls_file
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_paraview_client_doc_TARGET}-xmls.txt")
  file(GENERATE
    OUTPUT "${_paraview_client_doc_xmls_file}"
    CONTENT "${_paraview_client_doc_xmls}")

  add_custom_command(
    OUTPUT  "${_paraview_client_doc_OUTPUT_DIR}/${_paraview_client_doc_TARGET}.xslt"
            ${_paraview_client_doc_outputs}
    COMMAND "${CMAKE_COMMAND}"
            "-Dxmlpatterns=${qt_xmlpatterns_executable}"
            "-Doutput_dir=${_paraview_client_doc_OUTPUT_DIR}"
            "-Doutput_file=${_paraview_client_doc_OUTPUT_DIR}/${_paraview_client_doc_TARGET}.xslt"
            "-Dxmls_file=${_paraview_client_doc_xmls_file}"
            -D_paraview_generate_proxy_documentation_run=ON
            -P "${_ParaViewClient_script_file}"
    DEPENDS ${_paraview_client_doc_xmls_list}
            "${_paraview_client_doc_xmls_file}"
            "${_ParaViewClient_script_file}"
            "${_ParaViewClient_cmake_dir}/paraview_servermanager_convert_xml.xsl"
            "${_ParaViewClient_cmake_dir}/paraview_servermanager_convert_categoryindex.xsl"
            "${_ParaViewClient_cmake_dir}/paraview_servermanager_convert_html.xsl"
            "${_ParaViewClient_cmake_dir}/paraview_servermanager_convert_wiki.xsl.in"
    WORKING_DIRECTORY "${_paraview_client_doc_OUTPUT_DIR}"
    COMMENT "Generating documentation for ${_paraview_client_doc_TARGET}")
  add_custom_target("${_paraview_client_doc_TARGET}"
    DEPENDS
      "${_paraview_client_doc_OUTPUT_DIR}/${_paraview_client_doc_TARGET}.xslt"
      ${_paraview_client_doc_outputs})
endfunction ()

# Generate proxy documentation.
if (_paraview_generate_proxy_documentation_run AND CMAKE_SCRIPT_MODE_FILE)

  file(READ "${xmls_file}" xmls)

  set(_paraview_gpd_to_xml "${CMAKE_CURRENT_LIST_DIR}/paraview_servermanager_convert_xml.xsl")
  set(_paraview_gpd_to_catindex "${CMAKE_CURRENT_LIST_DIR}/paraview_servermanager_convert_categoryindex.xsl")
  set(_paraview_gpd_to_html "${CMAKE_CURRENT_LIST_DIR}/paraview_servermanager_convert_html.xsl")
  set(_paraview_gpd_to_wiki "${CMAKE_CURRENT_LIST_DIR}/paraview_servermanager_convert_wiki.xsl.in")

  set(_paraview_gpd_xslt "<xml>\n")
  file(MAKE_DIRECTORY "${output_dir}")
  foreach (_paraview_gpd_xml IN LISTS xmls)
    execute_process(
      COMMAND "${xmlpatterns}"
              "${_paraview_gpd_to_xml}"
              "${_paraview_gpd_xml}"
      OUTPUT_VARIABLE _paraview_gpd_output
      ERROR_VARIABLE  _paraview_gpd_error
      RESULT_VARIABLE _paraview_gpd_result)
    if (_paraview_gpd_result)
      message(FATAL_ERROR
        "Failed to convert servermanager XML: ${_paraview_gpd_error}")
    endif ()

    string(APPEND _paraview_gpd_xslt
      "${_paraview_gpd_output}")
  endforeach ()
  string(APPEND _paraview_gpd_xslt
    "</xml>\n")

  file(WRITE "${output_file}.xslt"
    "${_paraview_gpd_xslt}")
  execute_process(
    COMMAND "${xmlpatterns}"
            -output "${output_file}"
            "${_paraview_gpd_to_catindex}"
            "${output_file}.xslt"
    RESULT_VARIABLE _paraview_gpd_result)
  if (_paraview_gpd_result)
    message(FATAL_ERROR
      "Failed to generate category index")
  endif ()

  # Generate HTML files.
  execute_process(
    COMMAND "${xmlpatterns}"
            "${_paraview_gpd_to_html}"
            "${output_file}"
    OUTPUT_VARIABLE _paraview_gpd_output
    RESULT_VARIABLE _paraview_gpd_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (_paraview_gpd_result)
    message(FATAL_ERROR
      "Failed to generate HTML output")
  endif ()

  # Escape semicolons.
  _paraview_client_escape_cmake_list(_paraview_gpd_output)
  # Convert into a list of HTML documents.
  string(REPLACE "</html>\n<html>" "</html>\n;<html>"  _paraview_gpd_output "${_paraview_gpd_output}")

  foreach (_paraview_gpd_html_doc IN LISTS _paraview_gpd_output)
    string(REGEX MATCH "<meta name=\"filename\" contents=\"([^\"]*)\"" _ "${_paraview_gpd_html_doc}")
    set(_paraview_gpd_filename "${CMAKE_MATCH_1}")
    if (NOT _paraview_gpd_filename)
      message(FATAL_ERROR
        "No filename for an HTML output?")
    endif ()

    _paraview_client_unescape_cmake_list(_paraview_gpd_html_doc)

    # Replace reStructured Text markup.
    string(REGEX REPLACE "\\*\\*([^*]+)\\*\\*" "<b>\\1</b>" _paraview_gpd_html_doc "${_paraview_gpd_html_doc}")
    string(REGEX REPLACE "\\*([^*]+)\\*" "<em>\\1</em>" _paraview_gpd_html_doc "${_paraview_gpd_html_doc}")
    string(REGEX REPLACE "\n\n- " "\n<ul><li>" _paraview_gpd_html_doc "${_paraview_gpd_html_doc}")
    string(REGEX REPLACE "\n-" "\n<li>" _paraview_gpd_html_doc "${_paraview_gpd_html_doc}")
    string(REGEX REPLACE "<li>(.*)\n\n([^-])" "<li>\\1</ul>\n\\2" _paraview_gpd_html_doc "${_paraview_gpd_html_doc}")
    string(REGEX REPLACE "\n\n" "\n<p>\n" _paraview_gpd_html_doc "${_paraview_gpd_html_doc}")
    file(WRITE "${output_dir}/${_paraview_gpd_filename}"
      "${_paraview_gpd_html_doc}\n")
  endforeach ()

  # Generate Wiki files.
  string(REGEX MATCHALL "proxy_group=\"[^\"]*\"" _paraview_gpd_groups "${_paraview_gpd_xslt}")
  string(REGEX REPLACE "proxy_group=\"([^\"]*)\"" "\\1" _paraview_gpd_groups "${_paraview_gpd_groups}")
  list(APPEND _paraview_gpd_groups readers)
  if (_paraview_gpd_groups)
    list(REMOVE_DUPLICATES _paraview_gpd_groups)
  endif ()

  foreach (_paraview_gpd_group IN LISTS _paraview_gpd_groups)
    if (_paraview_gpd_group STREQUAL "readers")
      set(_paraview_gpd_query "contains(lower-case($proxy_name),'reader')")
      set(_paraview_gpd_group_real "sources")
    else ()
      set(_paraview_gpd_query "not(contains(lower-case($proxy_name),'reader'))")
      set(_paraview_gpd_group_real "${_paraview_gpd_group}")
    endif ()

    set(_paraview_gpd_wiki_xsl
      "${output_dir}/${_paraview_gpd_group}.xsl")
    configure_file(
      "${_paraview_gpd_to_wiki}"
      "${_paraview_gpd_wiki_xsl}"
      @ONLY)
    execute_process(
      COMMAND "${xmlpatterns}"
              "${_paraview_gpd_wiki_xsl}"
              "${output_file}"
      OUTPUT_VARIABLE _paraview_gpd_output
      RESULT_VARIABLE _paraview_gpd_result)
    if (_paraview_gpd_result)
      message(FATAL_ERROR
        "Failed to generate Wiki output for ${_paraview_gpd_group}")
    endif ()
    string(REGEX REPLACE " +" " " _paraview_gpd_output "${_paraview_gpd_output}")
    string(REPLACE "\n " "\n" _paraview_gpd_output "${_paraview_gpd_output}")
    file(WRITE "${output_dir}/${_paraview_gpd_group}.wiki"
      "${_paraview_gpd_output}")
  endforeach ()
endif ()

#[==[.md
## Generating help documentation

TODO: Document

```
paraview_client_generate_help(
  NAME    <name>
  [TARGET <target>]

  OUTPUT_PATH <var>

  [OUTPUT_DIR <directory>]
  [SOURCE_DIR <directory>]
  [PATTERNS   <pattern>...]
  [DEPENDS    <depend>...]

  [NAMESPACE  <namespace>]
  [FOLDER     <folder>]

  [TABLE_OF_CONTENTS      <toc>]
  [TABLE_OF_CONTENTS_FILE <tocfile>]

  [RESOURCE_FILE    <qrcfile>]
  [RESOURCE_PREFIX  <prefix>]
```

  * `NAME`: (Required) The basename of the generated `.qch` file.
  * `TARGET`: (Defaults to `<NAME>`) The name of the generated target.
  * `OUTPUT_PATH`: (Required) This variable is set to the output path of the
    generated `.qch` file.
  * `OUTPUT_DIR`: (Defaults to `${CMAKE_CURRENT_BINARY_DIR}`) Where to place
    generated files.
  * `SOURCE_DIR`: Where to copy input files from.
  * `PATTERNS`: (Defaults to `*.*`) If `SOURCE_DIR` is specified, files
    matching these globs will be copied to `OUTPUT_DIR`.
  * `DEPENDS`: A list of dependencies which are required before the help can be
    generated. Note that file paths which are generated via
    `add_custom_command` must be in the same directory as the
    `paraview_client_generate_help` on non-Ninja generators.
  * `NAMESPACE`: (Defaults to `<NAME>.org`) The namespace for the generated
    help.
  * `FOLDER`: (Defaults to `<NAME>`) The folder for the generated help.
  * `TABLE_OF_CONTENTS` and `TABLE_OF_CONTENTS_FILE`: At most one may be
    provided. This is used as the `<toc>` element in the generated help. If not
    provided at all, a table of contents will be generated.
  * `RESOURCE_FILE`: If provided, a Qt resource file providing the contents of
    the generated help will be generated at this path. It will be available as
    `<RESOURCE_PREFIX>/<NAME>`.
  * `RESOURCE_PREFIX`: The prefix to use for the generated help's Qt resource.
#]==]
function (paraview_client_generate_help)
  cmake_parse_arguments(_paraview_client_help
    ""
    "NAME;TARGET;OUTPUT_DIR;SOURCE_DIR;NAMESPACE;FOLDER;TABLE_OF_CONTENTS;TABLE_OF_CONTENTS_FILE;RESOURCE_FILE;RESOURCE_PREFIX;OUTPUT_PATH"
    "PATTERNS;DEPENDS"
    ${ARGN})

  if (_paraview_client_help_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_client_generate_help: "
      "${_paraview_client_help_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_client_help_NAME)
    message(FATAL_ERROR
      "The `NAME` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_help_OUTPUT_PATH)
    message(FATAL_ERROR
      "The `OUTPUT_PATH` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_help_TARGET)
    set(_paraview_client_help_TARGET
      "${_paraview_client_help_NAME}")
  endif ()

  if (NOT DEFINED _paraview_client_help_OUTPUT_DIR)
    set(_paraview_client_help_OUTPUT_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/paraview_help")
  endif ()

  if (NOT DEFINED _paraview_client_help_NAMESPACE)
    set(_paraview_client_help_NAMESPACE
      "${_paraview_client_help_NAME}.org")
  endif ()

  if (NOT DEFINED _paraview_client_help_FOLDER)
    set(_paraview_client_help_FOLDER
      "${_paraview_client_help_NAME}")
  endif ()

  if (DEFINED _paraview_client_help_TABLE_OF_CONTENTS_FILE)
    file(READ "${_paraview_client_help_TABLE_OF_CONTENTS_FILE}"
      _paraview_client_help_toc)
  elseif (DEFINED _paraview_client_help_TABLE_OF_CONTENTS)
    set(_paraview_client_help_toc
      "${_paraview_client_help_TABLE_OF_CONTENTS}")
  else ()
    set(_paraview_client_help_toc)
  endif ()
  string(REPLACE "\n" " " _paraview_client_help_toc "${_paraview_client_help_toc}")

  if (NOT DEFINED _paraview_client_help_PATTERNS)
    set(_paraview_client_help_PATTERNS
      "*.*")
  endif ()

  include("${_ParaViewClient_cmake_dir}/paraview-find-package-helpers.cmake" OPTIONAL)
  find_package(Qt5 QUIET REQUIRED COMPONENTS Help)

  set(_paraview_client_help_copy_sources)
  set(_paraview_client_help_copied_sources)
  if (DEFINED _paraview_client_help_SOURCE_DIR)
    list(APPEND _paraview_client_help_copy_sources
      COMMAND "${CMAKE_COMMAND}" -E copy_directory
              "${_paraview_client_help_SOURCE_DIR}"
              "${_paraview_client_help_OUTPUT_DIR}")

    file(GLOB _paraview_client_help_copied_sources
      ${_paraview_client_help_PATTERNS})
  endif ()

  file(MAKE_DIRECTORY "${_paraview_client_help_OUTPUT_DIR}")

  set(_paraview_client_help_patterns "${_paraview_client_help_PATTERNS}")
  _paraview_client_escape_cmake_list(_paraview_client_help_patterns)

  set(_paraview_client_help_qhp
    "${_paraview_client_help_OUTPUT_DIR}/${_paraview_client_help_NAME}.qhp")
  set(_paraview_client_help_output
    "${_paraview_client_help_OUTPUT_DIR}/${_paraview_client_help_NAME}.qch")
  add_custom_command(
    OUTPUT  "${_paraview_client_help_output}"
    DEPENDS "${_ParaViewClient_script_file}"
            ${_paraview_client_help_copied_sources}
            ${_paraview_client_help_DEPENDS}
    ${_paraview_client_help_copy_sources}
    COMMAND "${CMAKE_COMMAND}"
            "-Doutput_dir=${_paraview_client_help_OUTPUT_DIR}"
            "-Doutput_file=${_paraview_client_help_qhp}"
            "-Dnamespace=${_paraview_client_help_NAMESPACE}"
            "-Dfolder=${_paraview_client_help_FOLDER}"
            "-Dname=${_paraview_client_help_NAME}"
            "-Dtoc=${_paraview_client_help_toc}"
            "-Dpatterns=${_paraview_client_help_patterns}"
            -D_paraview_generate_help_run=ON
            -P "${_ParaViewClient_script_file}"
    VERBATIM
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
            $<TARGET_FILE:Qt5::qhelpgenerator>
            "${_paraview_client_help_qhp}"
            -s
            -o "${_paraview_client_help_output}"
    COMMENT "Compiling Qt help for ${_paraview_client_help_NAME}"
    WORKING_DIRECTORY "${_paraview_client_help_OUTPUT_DIR}")
  add_custom_target("${_paraview_client_help_TARGET}"
    DEPENDS
      "${_paraview_client_help_output}")

  if (DEFINED _paraview_client_help_RESOURCE_FILE)
    if (NOT DEFINED _paraview_client_help_RESOURCE_PREFIX)
      message(FATAL_ERROR
        "The `RESOURCE_PREFIX` argument is required if `RESOURCE_FILE` is given.")
    endif ()

    paraview_client_qt_resource(
      OUTPUT  "${_paraview_client_help_RESOURCE_FILE}"
      PREFIX  "${_paraview_client_help_RESOURCE_PREFIX}"
      FILE    "${_paraview_client_help_output}")
    set_property(SOURCE "${_paraview_client_help_RESOURCE_FILE}"
      PROPERTY
        OBJECT_DEPENDS "${_paraview_client_help_output}")
  endif ()

  set("${_paraview_client_help_OUTPUT_PATH}"
    "${_paraview_client_help_output}"
    PARENT_SCOPE)
endfunction ()

# Handle the generation of the help file.
if (_paraview_generate_help_run AND CMAKE_SCRIPT_MODE_FILE)
  _paraview_client_unescape_cmake_list(patterns)

  set(_paraview_help_patterns)
  foreach (_paraview_help_pattern IN LISTS patterns)
    if (IS_ABSOLUTE "${_paraview_help_pattern}")
      list(APPEND _paraview_help_patterns
        "${_paraview_help_pattern}")
    else ()
      list(APPEND _paraview_help_patterns
        "${output_dir}/${_paraview_help_pattern}")
    endif ()
  endforeach ()

  file(GLOB _paraview_help_files
    RELATIVE "${output_dir}"
    ${_paraview_help_patterns})

  if (NOT toc)
    if (NOT _paraview_help_files)
      message(FATAL_ERROR
        "No matching files given without a table of contents")
    endif ()
    set(_paraview_help_subsections "")
    list(GET _paraview_help_files 0
      _paraview_help_index)
    set(_paraview_help_subsections "")
    foreach (_paraview_help_file IN LISTS _paraview_help_files)
      if (NOT _paraview_help_file MATCHES "\\.html$")
        continue ()
      endif ()
      get_filename_component(_paraview_help_name "${_paraview_help_file}" NAME_WE)
      set(_paraview_help_title "${_paraview_help_name}")
      file(READ "${_paraview_help_file}" _paraview_help_contents)
      string(REGEX MATCH "<title>([^<]*)</title>" _ "${_paraview_help_contents}")
      if (CMAKE_MATCH_1)
        set(_paraview_help_title "${CMAKE_MATCH_1}")
      endif ()
      string(APPEND _paraview_help_subsections
        "    <section title=\"${_paraview_help_title}\" ref=\"${_paraview_help_file}\" />\n")

      string(TOLOWER "${_paraview_help_name}" _paraview_help_name_lower)
      if (_paraview_help_name_lower STREQUAL "index")
        set(_paraview_help_index
          "${_paraview_help_file}")
      endif ()
    endforeach ()
    set(toc
      "<toc>\n  <section title=\"${name}\" ref=\"${_paraview_help_index}\">\n${_paraview_help_subsections}  </section>\n</toc>")
  endif ()

  set(_paraview_help_file_entries "")
  foreach (_paraview_help_file IN LISTS _paraview_help_files)
    string(APPEND _paraview_help_file_entries
      "      <file>${_paraview_help_file}</file>\n")
  endforeach ()

  file(WRITE "${output_file}"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<QtHelpProject version=\"1.0\">
  <namespace>${namespace}</namespace>
  <virtualFolder>${folder}</virtualFolder>
  <filterSection>
    ${toc}
    <keywords>
      <!-- TODO: how to handle keywords? -->
    </keywords>
    <files>
${_paraview_help_file_entries}
    </files>
  </filterSection>
</QtHelpProject>\n")
endif ()

#[==[.md
## Qt resources

Compiling Qt resources into a client can be a little tedious. To help with
this, some functions are provided to make it easier to embed content into the
client.
#]==]

#[==[.md
### Single file

```
paraview_client_qt_resource(
  OUTPUT  <file>
  PREFIX  <prefix>
  FILE    <file>
  [ALIAS  <alias>])
```

Outputs a Qt resource to the file given to the `OUTPUT` argument. Its resource
name is `<PREFIX>/<ALIAS>`. The contents are copied from the contents of the
file specified by the `FILE` argument. If not given the name of the file is
used as the `ALIAS`.
#]==]
function (paraview_client_qt_resource)
  cmake_parse_arguments(_paraview_client_resource
    ""
    "OUTPUT;PREFIX;ALIAS;FILE"
    ""
    ${ARGN})

  if (_paraview_client_resource_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_client_qt_resource: "
      "${_paraview_client_resource_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_client_resource_OUTPUT)
    message(FATAL_ERROR
      "The `OUTPUT` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_resource_PREFIX)
    message(FATAL_ERROR
      "The `PREFIX` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_resource_FILE)
    message(FATAL_ERROR
      "The `FILE` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_resource_ALIAS)
    get_filename_component(_paraview_client_resource_ALIAS
      "${_paraview_client_resource_FILE}"
      NAME)
  endif ()

  get_filename_component(_paraview_client_resource_file_path
    "${_paraview_client_resource_FILE}"
    ABSOLUTE)
  get_filename_component(_paraview_client_resource_file_path
    "${_paraview_client_resource_file_path}"
    REALPATH)
  if (WIN32)
    file(TO_NATIVE_PATH
      "${_paraview_client_resource_file_path}"
      _paraview_client_resource_file_path)
  endif ()

  # We cannot use file(GENERATE) because automoc doesn't like when generated
  # sources are in the source list.
  file(WRITE "${_paraview_client_resource_OUTPUT}.tmp"
    "<RCC>
  <qresource prefix=\"/${_paraview_client_resource_PREFIX}\">
    <file alias=\"${_paraview_client_resource_ALIAS}\">${_paraview_client_resource_file_path}</file>
  </qresource>
</RCC>\n")
  configure_file(
    "${_paraview_client_resource_OUTPUT}.tmp"
    "${_paraview_client_resource_OUTPUT}"
    COPYONLY)
endfunction ()

#[==[.md
### Many files

```
paraview_client_qt_resources(
  OUTPUT  <file>
  PREFIX  <prefix>
  FILES   <file>...)
```

Outputs a Qt resource to the file given to the `OUTPUT` argument. Its resource
name is `<PREFIX>/<filename>` for each of the files in the given list. If
aliases other than the filenames are required, the
`paraview_client_qt_resource` function should be used instead.
#]==]
function (paraview_client_qt_resources)
  cmake_parse_arguments(_paraview_client_resources
    ""
    "OUTPUT;PREFIX"
    "FILES"
    ${ARGN})

  if (_paraview_client_resources_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_client_qt_resources: "
      "${_paraview_client_resources_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_client_resources_OUTPUT)
    message(FATAL_ERROR
      "The `OUTPUT` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_resources_PREFIX)
    message(FATAL_ERROR
      "The `PREFIX` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_client_resources_FILES)
    message(FATAL_ERROR
      "The `FILES` argument is required.")
  endif ()

  set(_paraview_client_resources_contents)

  string(APPEND _paraview_client_resources_contents
    "<RCC>\n  <qresource prefix=\"${_paraview_client_resources_PREFIX}\">\n")
  foreach (_paraview_client_resources_file IN LISTS _paraview_client_resources_FILES)
    get_filename_component(_paraview_client_resources_alias
      "${_paraview_client_resources_file}"
      NAME)
    get_filename_component(_paraview_client_resources_file_path
      "${_paraview_client_resources_file}"
      ABSOLUTE)
    get_filename_component(_paraview_client_resources_file_path
      "${_paraview_client_resources_file_path}"
      REALPATH)
    if (WIN32)
      file(TO_NATIVE_PATH
        "${_paraview_client_resources_file_path}"
        _paraview_client_resources_file_path)
    endif ()
    string(APPEND _paraview_client_resources_contents
      "    <file alias=\"${_paraview_client_resources_alias}\">${_paraview_client_resources_file_path}</file>\n")
  endforeach ()
  string(APPEND _paraview_client_resources_contents
    "  </qresource>\n</RCC>\n")

  # We cannot use file(GENERATE) because automoc doesn't like when generated
  # sources are in the source list.
  file(WRITE "${_paraview_client_resources_OUTPUT}.tmp"
    "${_paraview_client_resources_contents}")
  configure_file(
    "${_paraview_client_resources_OUTPUT}.tmp"
    "${_paraview_client_resources_OUTPUT}"
    COPYONLY)
endfunction ()
