#[==[.md
# `vtkModuleWrapClientServer`

This module includes logic necessary in order to wrap VTK modules using
ParaView's ClientServer "language". This allows for classes in the module to be
used as proxies between ParaView client and server programs.
#]==]

#[==[.md INTERNAL
## Wrapping a single module

This function generates the wrapped sources for a module. It places the list of
generated source files and classes in variables named in the second and third
arguments, respectively.

```
_vtk_module_wrap_client_server_sources(<module> <sources> <classes>)
```
#]==]
function (_vtk_module_wrap_client_server_sources module sources classes)
  _vtk_module_get_module_property("${module}"
    PROPERTY  "exclude_wrap"
    VARIABLE  _vtk_client_server_exclude_wrap)
  if (_vtk_client_server_exclude_wrap)
    return ()
  endif ()
  _vtk_module_get_module_property("${module}"
    PROPERTY  "client_server_exclude"
    VARIABLE  _vtk_client_server_exclude)
  if (_vtk_client_server_exclude)
    return ()
  endif ()

  set(_vtk_client_server_args_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_client_server_library_name}-client-server.$<CONFIGURATION>.args")

  set(_vtk_client_server_genex_compile_definitions
    "$<TARGET_PROPERTY:${_vtk_client_server_target_name},COMPILE_DEFINITIONS>")
  set(_vtk_client_server_genex_include_directories
    "$<TARGET_PROPERTY:${_vtk_client_server_target_name},INCLUDE_DIRECTORIES>")
  file(GENERATE
    OUTPUT  "${_vtk_client_server_args_file}"
    CONTENT "$<$<BOOL:${_vtk_client_server_genex_compile_definitions}>:\n-D\'$<JOIN:${_vtk_client_server_genex_compile_definitions},\'\n-D\'>\'>\n
$<$<BOOL:${_vtk_client_server_genex_include_directories}>:\n-I\'$<JOIN:${_vtk_client_server_genex_include_directories},\'\n-I\'>\'>\n")

  _vtk_module_get_module_property("${module}"
    PROPERTY  "hierarchy"
    VARIABLE  _vtk_client_server_hierarchy_file)

  get_property(_vtk_client_server_is_imported
    TARGET    "${module}"
    PROPERTY  "IMPORTED")
  if (_vtk_client_server_is_imported OR CMAKE_GENERATOR MATCHES "Ninja")
    set(_vtk_client_server_command_depend "${_vtk_client_server_hierarchy_file}")
  else ()
    if (TARGET "${_vtk_client_server_target_name}-hierarchy")
      set(_vtk_client_server_command_depend "${_vtk_client_server_target_name}-hierarchy")
    else ()
      message(FATAL_ERROR
        "The ${module} hierarchy file is attached to a non-imported target "
        "and a hierarchy target is missing.")
    endif ()
  endif ()

  set(_vtk_client_server_sources)

  _vtk_module_get_module_property("${module}"
    PROPERTY  "headers"
    VARIABLE  _vtk_client_server_headers)
  set(_vtk_client_server_classes)
  foreach (_vtk_client_server_header IN LISTS _vtk_client_server_headers)
    get_filename_component(_vtk_client_server_basename "${_vtk_client_server_header}" NAME_WE)
    list(APPEND _vtk_client_server_classes
      "${_vtk_client_server_basename}")

    set(_vtk_client_server_source_output
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_client_server_basename}ClientServer.cxx")
    list(APPEND _vtk_client_server_sources
      "${_vtk_client_server_source_output}")

    add_custom_command(
      OUTPUT  "${_vtk_client_server_source_output}"
      COMMAND ParaView::WrapClientServer
              "@${_vtk_client_server_args_file}"
              -o "${_vtk_client_server_source_output}"
              "${_vtk_client_server_header}"
              --types "${_vtk_client_server_hierarchy_file}"
      IMPLICIT_DEPENDS
              CXX "${_vtk_client_server_header}"
      COMMENT "Generating client_server wrapper sources for ${_vtk_client_server_basename}"
      DEPENDS
        ParaView::WrapClientServer
        "${_vtk_client_server_header}"
        "${_vtk_client_server_args_file}"
        "${_vtk_client_server_command_depend}")
  endforeach ()

  set("${sources}"
    "${_vtk_client_server_sources}"
    PARENT_SCOPE)
  set("${classes}"
    "${_vtk_client_server_classes}"
    PARENT_SCOPE)
endfunction ()

#[==[.md INTERNAL
## Generating a client server library

A client server library may consist of the wrappings of multiple VTK modules.
This is useful for kit-based builds where the modules part of the same kit
belong to the same client server library as well.

```
_vtk_module_wrap_client_server_library(<name> <module>...)
```

The first argument is the name of the client server library. The remaining
arguments are VTK modules to include in the library.

The remaining information it uses is assumed to be provided by the
`vtk_module_wrap_client_server` function.
#]==]
function (_vtk_module_wrap_client_server_library name)
  set(_vtk_client_server_library_sources)
  set(_vtk_client_server_library_classes)
  foreach (_vtk_client_server_module IN LISTS ARGN)
    _vtk_module_get_module_property("${_vtk_client_server_module}"
      PROPERTY  "exclude_wrap"
      VARIABLE  _vtk_client_server_exclude_wrap)
    if (_vtk_client_server_exclude_wrap)
      continue ()
    endif ()
    _vtk_module_get_module_property("${_vtk_client_server_module}"
      PROPERTY  "client_server_exclude"
      VARIABLE  _vtk_client_server_exclude)
    if (_vtk_client_server_exclude)
      return ()
    endif ()

    _vtk_module_wrap_client_server_sources("${_vtk_client_server_module}" _vtk_client_server_sources _vtk_client_server_classes)
    list(APPEND _vtk_client_server_library_sources
      ${_vtk_client_server_sources})
    list(APPEND _vtk_client_server_library_classes
      ${_vtk_client_server_classes})
  endforeach ()

  if (NOT _vtk_client_server_library_sources)
    return ()
  endif ()

  # TODO: Support unified bindings?

  set(_vtk_client_server_declarations)
  set(_vtk_client_server_calls)
  foreach (_vtk_client_server_class IN LISTS _vtk_client_server_library_classes)
    string(APPEND _vtk_client_server_declarations
      "extern void ${_vtk_client_server_class}_Init(vtkClientServerInterpreter*);\n")
    string(APPEND _vtk_client_server_calls
      "  ${_vtk_client_server_class}_Init(csi);\n")
  endforeach ()
  set(_vtk_client_server_init_content
    "#include \"vtkABI.h\"
#include \"vtkClientServerInterpreter.h\"

${_vtk_client_server_declarations}
extern \"C\" void VTK_ABI_EXPORT ${name}_Initialize(vtkClientServerInterpreter* csi)
{
  (void)csi;
${_vtk_client_server_calls}}\n")

  set(_vtk_client_server_init_file
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${name}Init.cxx")
  file(GENERATE
    OUTPUT  "${_vtk_client_server_init_file}"
    CONTENT "${_vtk_client_server_init_content}")
  # XXX(cmake): Why is this necessary? One would expect that `file(GENERATE)`
  # would do this automatically.
  set_property(SOURCE "${_vtk_client_server_init_file}"
    PROPERTY
      GENERATED 1)

  add_library("${name}" STATIC
    ${_vtk_client_server_library_sources}
    "${_vtk_client_server_init_file}")
  if (BUILD_SHARED_LIBS)
    set_property(TARGET "${name}"
      PROPERTY
        POSITION_INDEPENDENT_CODE 1)
  endif ()
  set(_vtk_build_LIBRARY_NAME_SUFFIX "${_vtk_client_server_LIBRARY_NAME_SUFFIX}")
  set(_vtk_build_ARCHIVE_DESTINATION "${_vtk_client_server_DESTINATION}")
  _vtk_module_apply_properties("${name}")
  _vtk_module_install("${name}")

  vtk_module_autoinit(
    MODULES ${ARGN}
    TARGETS "${name}")

  target_link_libraries("${name}"
    PRIVATE
      ${ARGN}
      ParaView::ClientServer
      VTK::CommonCore)

  set(_vtk_client_server_export)
  if (_vtk_client_server_INSTALL_EXPORT)
    set(_vtk_client_server_export
      EXPORT "${_vtk_client_server_INSTALL_EXPORT}")
  endif ()

  install(
    TARGETS             "${name}"
    ${_vtk_client_server_export}
    COMPONENT           "${_vtk_client_server_COMPONENT}"
    ARCHIVE DESTINATION "${_vtk_client_server_DESTINATION}")
endfunction ()

#[==[.md
## Wrapping a set of VTK modules for ClientServer

```
vtk_module_wrap_client_server(
  MODULES <module>...
  TARGET <target>
  [WRAPPED_MODULES <varname>]

  [FUNCTION_NAME <function>]
  [DESTINATION <destination>]

  [INSTALL_EXPORT <export>]
  [COMPONENT <component>])
```

  * `MODULES`: (Required) The list of modules to wrap.
  * `TARGET`: (Required) The target to create which represents all wrapped
    ClientServer modules. This is used to provide the function used to
    initialize the bindings.
  * `WRAPPED_MODULES`: (Recommended) Not all modules are wrappable. This
    variable will be set to contain the list of modules which were wrapped.
  * `FUNCTION_NAME`: (Recommended) (Defaults to `<TARGET>_initialize`) The
    function name to generate in order to initialize the client server
    bindings.A header with the name `<TARGET>.h` should be included in order to
    access the initialization function.
  * `DESTINATION`: (Defaults to `${CMAKE_INSTALL_LIBDIR}`) Where to install the
    generated libraries.
  * `INSTALL_EXPORT`: If provided, installs will add the installed
    libraries to the provided export set.
  * `COMPONENT`: (Defaults to `development`) All install rules created by this
    function will use this installation component.
#]==]
function (vtk_module_wrap_client_server)
  cmake_parse_arguments(_vtk_client_server
    ""
    "DESTINATION;INSTALL_EXPORT;TARGET;COMPONENT;FUNCTION_NAME;WRAPPED_MODULES"
    "MODULES"
    ${ARGN})

  if (_vtk_client_server_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_wrap_client_server: "
      "${_vtk_client_server_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_client_server_MODULES)
    message(WARNING
      "No modules were requested for client server wrapping.")
    return ()
  endif ()

  if (NOT _vtk_client_server_TARGET)
    message(FATAL_ERROR
      "The `TARGET` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_client_server_DESTINATION)
    set(_vtk_client_server_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  endif ()

  if (NOT DEFINED _vtk_client_server_COMPONENT)
    set(_vtk_client_server_COMPONENT "development")
  endif ()

  if (NOT DEFINED _vtk_client_server_FUNCTION_NAME)
    set(_vtk_client_server_FUNCTION_NAME "${_vtk_client_server_TARGET}_initialize")
  endif ()

  # TODO: Install cmake properties?

  set(_vtk_client_server_all_modules)
  set(_vtk_client_server_all_wrapped_modules)
  foreach (_vtk_client_server_module IN LISTS _vtk_client_server_MODULES)
    _vtk_module_get_module_property("${_vtk_client_server_module}"
      PROPERTY  "exclude_wrap"
      VARIABLE  _vtk_client_server_exclude_wrap)
    if (_vtk_client_server_exclude_wrap)
      continue ()
    endif ()
    _vtk_module_get_module_property("${_vtk_client_server_module}"
      PROPERTY  "client_server_exclude"
      VARIABLE  _vtk_client_server_exclude)
    if (_vtk_client_server_exclude)
      continue ()
    endif ()
    _vtk_module_real_target(_vtk_client_server_target_name "${_vtk_client_server_module}")
    _vtk_module_get_module_property("${_vtk_client_server_module}"
      PROPERTY  "library_name"
      VARIABLE  _vtk_client_server_library_name)
    _vtk_module_wrap_client_server_library("${_vtk_client_server_library_name}CS" "${_vtk_client_server_module}")

    if (TARGET "${_vtk_client_server_library_name}CS")
      list(APPEND _vtk_client_server_all_modules
        "${_vtk_client_server_library_name}CS")
      list(APPEND _vtk_client_server_all_wrapped_modules
        "${_vtk_client_server_module}")
    endif ()
  endforeach ()

  if (NOT _vtk_client_server_all_modules)
    message(FATAL_ERROR
      "No modules given could be wrapped.")
  endif ()

  if (DEFINED _vtk_client_server_WRAPPED_MODULES)
    set("${_vtk_client_server_WRAPPED_MODULES}"
      "${_vtk_client_server_all_wrapped_modules}"
      PARENT_SCOPE)
  endif ()

  if (_vtk_client_server_TARGET)
    add_library("${_vtk_client_server_TARGET}" INTERFACE)
    target_include_directories("${_vtk_client_server_TARGET}"
      INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_client_server_TARGET}>")

    set(_vtk_client_server_all_modules_include_file
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_client_server_TARGET}/${_vtk_client_server_TARGET}.h")

    set(_vtk_client_server_declarations)
    set(_vtk_client_server_calls)
    foreach (_vtk_client_server_module IN LISTS _vtk_client_server_all_modules)
      string(APPEND _vtk_client_server_declarations
        "extern \"C\" void ${_vtk_client_server_module}_Initialize(vtkClientServerInterpreter*);\n")
      string(APPEND _vtk_client_server_calls
        "  ${_vtk_client_server_module}_Initialize(csi);\n")
    endforeach ()

    set(_vtk_client_server_all_modules_include_content
      "#ifndef ${_vtk_client_server_TARGET}_h
#define ${_vtk_client_server_TARGET}_h

#include \"vtkClientServerInterpreter.h\"

${_vtk_client_server_declarations}
void ${_vtk_client_server_FUNCTION_NAME}(vtkClientServerInterpreter* csi)
{
  (void)csi;
${_vtk_client_server_calls}}

#endif\n")

    file(GENERATE
      OUTPUT  "${_vtk_client_server_all_modules_include_file}"
      CONTENT "${_vtk_client_server_all_modules_include_content}")

    target_link_libraries("${_vtk_client_server_TARGET}"
      INTERFACE
        ${_vtk_client_server_all_modules})
  endif ()
endfunction ()

#[==[.md
## Excluding a module from wrapping

Some modules should not be wrapped using client server bindings. Since this is
independent of general wrapping facilities, an additional property is used to
check. This may be set using the `vtk_module_client_server_exclude` function.

```
vtk_module_client_server_exclude(
  [MODULE <module>])
```

The `MODULE` defaults to the module currently being built. If a module is not
being built when this function is called, it must be provided.
#]==]
function (vtk_module_client_server_exclude)
  cmake_parse_arguments(_vtk_client_server_exclude
    ""
    "MODULE"
    ""
    ${ARGN})

  if (_vtk_client_server_exclude_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_wrap_client_server_exclude: "
      "${_vtk_client_server_exclude_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_client_server_exclude_MODULE)
    if (NOT DEFINED _vtk_build_module)
      message(FATAL_ERROR
        "The `MODULE` argument must be provided outside of a module build.")
    endif ()
    set(_vtk_client_server_exclude_MODULE "${_vtk_build_module}")
  endif ()

  _vtk_module_set_module_property("${_vtk_client_server_exclude_MODULE}"
    PROPERTY  "client_server_exclude"
    VALUE     1)
endfunction ()
