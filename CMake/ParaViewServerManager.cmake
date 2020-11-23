#[==[.md
# Server Manager XMLs

ParaView uses XML files to describe filters available in its user interface.
Modules may add filters to the UI by providing XML files.

TODO: Document the ServerManager XML format.
#]==]

cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW)

#[==[.md
## Adding XMLs to modules

Modules may have associated XML files. They can be added to the module target
using this function.

```
paraview_add_server_manager_xmls(
  XMLS <xml>...)
```
#]==]
function (paraview_add_server_manager_xmls)
  cmake_parse_arguments(_paraview_add_sm
    ""
    "MODULE"
    "XMLS"
    ${ARGN})

  if (_paraview_add_sm_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_add_server_manager_xmls: "
      "${_paraview_add_sm_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _paraview_add_sm_XMLS)
    message(FATAL_ERROR
      "The `XMLS` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_add_sm_MODULE)
    set(_paraview_add_sm_MODULE "${_vtk_build_module}")
  endif ()

  foreach (_paraview_add_sm_xml IN LISTS _paraview_add_sm_XMLS)
    if (NOT IS_ABSOLUTE "${_paraview_add_sm_xml}")
      string(PREPEND _paraview_add_sm_xml "${CMAKE_CURRENT_SOURCE_DIR}/")
    endif ()

    _vtk_module_set_module_property("${_paraview_add_sm_MODULE}" APPEND
      PROPERTY  "paraview_server_manager_xml"
      VALUE     "${_paraview_add_sm_xml}")
  endforeach ()
endfunction ()

#[==[.md
## Building XML files

There are two functions offered to build server manager XML files. The first
uses modules:

```
paraview_server_manager_process(
  MODULES <module>...
  TARGET <target>
  [INSTALL_EXPORT <export>]
  [FILES <file>...]
  [XML_FILES  <variable>])
```

The `MODULES` argument contains the modules to include in the server manager
target. The function gathers the XML files declared using
`paraview_add_server_manager_xmls` and then calls
`paraview_server_manager_process_files`. If additional, non-module related XML
files are required, they may be passed via `FILES`.

If `XML_FILES` is given, the list of process XML files are set on the given
variable.

If `INSTALL_EXPORT` is given, the interface target will be added to the given
export set.
#]==]
function (paraview_server_manager_process)
  cmake_parse_arguments(_paraview_sm_process
    ""
    "TARGET;XML_FILES;INSTALL_EXPORT"
    "MODULES;FILES"
    ${ARGN})

  if (_paraview_sm_process_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_server_manager_process: "
      "${_paraview_sm_process_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _paraview_sm_process_MODULES)
    message(FATAL_ERROR
      "The `MODULES` argument is required.")
  endif ()

  if (NOT DEFINED _paraview_sm_process_TARGET)
    message(FATAL_ERROR
      "The `TARGET` argument is required.")
  endif ()

  # Topologically sort modules so that XML definitions that depend on each
  # other are loaded in the right order.
  set(_paraview_sm_process_sorted_modules ${_paraview_sm_process_MODULES})
  set(_paraview_sm_process_module_stack ${_paraview_sm_process_MODULES})
  set(_paraview_sm_process_module_seen)
  while (_paraview_sm_process_module_stack)
    list(GET _paraview_sm_process_module_stack 0 _paraview_sm_process_module)
    list(REMOVE_AT _paraview_sm_process_module_stack 0)
    if (_paraview_sm_process_module IN_LIST _paraview_sm_process_module_seen)
      continue ()
    endif ()
    list(APPEND _paraview_sm_process_module_seen
      "${_paraview_sm_process_module}")

    get_property(_paraview_sm_process_module_is_imported
      TARGET    "${_paraview_sm_process_module}"
      PROPERTY  IMPORTED)
    if (_paraview_sm_process_module_is_imported)
      _vtk_module_get_module_property("${_paraview_sm_process_module}"
        PROPERTY  "depends"
        VARIABLE  "_paraview_sm_process_depends")
      _vtk_module_get_module_property("${_paraview_sm_process_module}"
        PROPERTY  "private_depends"
        VARIABLE  "_paraview_sm_process_private_depends")
      _vtk_module_get_module_property("${_paraview_sm_process_module}"
        PROPERTY  "optional_depends"
        VARIABLE  "_paraview_sm_process_optional_depends")
    else ()
      get_property("_paraview_sm_process_depends" GLOBAL
        PROPERTY "_vtk_module_${_paraview_sm_process_module}_depends")
      get_property("_paraview_sm_process_private_depends" GLOBAL
        PROPERTY "_vtk_module_${_paraview_sm_process_module}_private_depends")
      get_property("_paraview_sm_process_optional_depends" GLOBAL
        PROPERTY "_vtk_module_${_paraview_sm_process_module}_optional_depends")
    endif ()

    # Prune optional dependencies that do not exist.
    set(_paraview_sm_process_optional_depends_exists)
    foreach (_paraview_sm_process_optional_depend IN LISTS _paraview_sm_process_optional_depends)
      if (TARGET "${_paraview_sm_process_optional_depend}")
        list(APPEND _paraview_sm_process_optional_depends_exists
          "${_paraview_sm_process_optional_depend}")
      endif ()
    endforeach ()

    # Put all of the dependencies into a single variable.
    set("_paraview_sm_process_${_paraview_sm_process_module}_all_depends"
      ${_paraview_sm_process_depends}
      ${_paraview_sm_process_private_depends}
      ${_paraview_sm_process_optional_depends_exists})
    list(APPEND _paraview_sm_process_module_stack
      ${_paraview_sm_process_depends}
      ${_paraview_sm_process_private_depends}
      ${_paraview_sm_process_optional_depends_exists})
  endwhile ()

  # Topologically sort according to dependencies.
  vtk_topological_sort(_paraview_sm_process_sorted_modules "_paraview_sm_process_" "_all_depends")

  # Limit the sorted modules to those that are actually in the pass module list.
  set(_paraview_sm_process_modules)
  foreach (_paraview_sm_process_sorted_module IN LISTS _paraview_sm_process_sorted_modules)
    if (_paraview_sm_process_sorted_module IN_LIST _paraview_sm_process_MODULES)
      list(APPEND _paraview_sm_process_modules
        "${_paraview_sm_process_sorted_module}")
    endif ()
  endforeach ()

  set(_paraview_sm_process_files)
  foreach (_paraview_sm_process_module IN LISTS _paraview_sm_process_modules)
    _vtk_module_get_module_property("${_paraview_sm_process_module}"
      PROPERTY  "paraview_server_manager_xml"
      VARIABLE  _paraview_sm_process_module_files)
    list(APPEND _paraview_sm_process_files
      ${_paraview_sm_process_module_files})
  endforeach ()

  list(APPEND _paraview_sm_process_files
    ${_paraview_sm_process_FILES})

  set(_paraview_sm_process_export_args)
  if (DEFINED _paraview_sm_process_INSTALL_EXPORT)
    list(APPEND _paraview_sm_process_export_args
      INSTALL_EXPORT "${_paraview_sm_process_INSTALL_EXPORT}")
  endif ()

  paraview_server_manager_process_files(
    TARGET  ${_paraview_sm_process_TARGET}
    FILES   ${_paraview_sm_process_files}
    ${_paraview_sm_process_export_args})

  if (DEFINED _paraview_sm_process_XML_FILES)
    set("${_paraview_sm_process_XML_FILES}"
      "${_paraview_sm_process_files}"
      PARENT_SCOPE)
  endif ()
endfunction ()

#[==[.md
The second way to process XML files directly.

```
paraview_server_manager_process_files(
  FILES <file>...
  TARGET <target>
  [INSTALL_EXPORT <export>])
```

The files passed to the `FILES` argument will be processed in to functions
which are then consumed by ParaView applications.

The name of the target is given to the `TARGET` argument. By default, the
filename is `<TARGET>.h` and it contains a function named
`<TARGET>_initialize`. They may be changed using the `FILE_NAME` and
`FUNCTION_NAME` arguments. The target has an interface usage requirement that
will allow the generated header to be included.

If `INSTALL_EXPORT` is given, the interface target will be added to the given
export set.
#]==]
function (paraview_server_manager_process_files)
  cmake_parse_arguments(_paraview_sm_process_files
    ""
    "TARGET;INSTALL_EXPORT"
    "FILES"
    ${ARGN})

  if (_paraview_sm_process_files_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for paraview_server_manager_process_files: "
      "${_paraview_sm_process_files_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _paraview_sm_process_files_TARGET)
    message(FATAL_ERROR
      "The `TARGET` argument is required.")
  endif ()

  set(_paraview_sm_process_files_output_dir
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_paraview_sm_process_files_TARGET}")
  set(_paraview_sm_process_files_output
    "${_paraview_sm_process_files_output_dir}/${_paraview_sm_process_files_TARGET}_data.h")
  set(_paraview_sm_process_files_response_file
    "${_paraview_sm_process_files_output_dir}/${_paraview_sm_process_files_TARGET}.args")

  string(REPLACE ";" "\n" _paraview_sm_process_files_input_file_content
    "${_paraview_sm_process_files_FILES}")
  file(GENERATE
    OUTPUT  "${_paraview_sm_process_files_response_file}"
    CONTENT "${_paraview_sm_process_files_input_file_content}")

  add_custom_command(
    OUTPUT  "${_paraview_sm_process_files_output}"
    DEPENDS ${_paraview_sm_process_files_FILES}
            "$<TARGET_FILE:ParaView::ProcessXML>"
            "${_paraview_sm_process_files_response_file}"
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
            $<TARGET_FILE:ParaView::ProcessXML>
            "${_paraview_sm_process_files_output}"
            "${_paraview_sm_process_files_TARGET}"
            "Interface"
            "GetInterfaces"
            "@${_paraview_sm_process_files_response_file}"
    COMMENT "Generating server manager headers for ${_paraview_sm_process_files_TARGET}.")
  add_custom_target("${_paraview_sm_process_files_TARGET}_xml_content"
    DEPENDS
      "${_paraview_sm_process_files_output}")

  set(_paraview_sm_process_files_init_content
    "#ifndef ${_paraview_sm_process_files_TARGET}_h
#define ${_paraview_sm_process_files_TARGET}_h

#include \"${_paraview_sm_process_files_TARGET}_data.h\"
#include <string>
#include <vector>

void ${_paraview_sm_process_files_TARGET}_initialize(std::vector<std::string>& xmls)
{\n  (void)xmls;\n")
  foreach (_paraview_sm_process_files_file IN LISTS _paraview_sm_process_files_FILES)
    get_filename_component(_paraview_sm_process_files_name "${_paraview_sm_process_files_file}" NAME_WE)
    string(APPEND _paraview_sm_process_files_init_content
      "  {
    char *init_string = ${_paraview_sm_process_files_TARGET}${_paraview_sm_process_files_name}GetInterfaces();
    xmls.push_back(init_string);
    delete [] init_string;
  }\n")
  endforeach ()
  string(APPEND _paraview_sm_process_files_init_content
    "}

#endif\n")

  file(GENERATE
    OUTPUT  "${_paraview_sm_process_files_output_dir}/${_paraview_sm_process_files_TARGET}.h"
    CONTENT "${_paraview_sm_process_files_init_content}")

  add_library("${_paraview_sm_process_files_TARGET}" INTERFACE)
  add_dependencies("${_paraview_sm_process_files_TARGET}"
    "${_paraview_sm_process_files_TARGET}_xml_content")
  target_include_directories("${_paraview_sm_process_files_TARGET}"
    INTERFACE
      "$<BUILD_INTERFACE:${_paraview_sm_process_files_output_dir}>")
  _vtk_module_apply_properties("${_paraview_sm_process_files_TARGET}")
  if (DEFINED _paraview_sm_process_files_INSTALL_EXPORT)
    set(_vtk_build_INSTALL_EXPORT
      "${_paraview_sm_process_files_INSTALL_EXPORT}")
  endif ()
  _vtk_module_install("${_paraview_sm_process_files_TARGET}")
endfunction ()

cmake_policy(POP)
