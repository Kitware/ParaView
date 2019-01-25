# New CMake Module System

VTK's module system has been updated and ParaView now works with it. CMake
variables have changed names:

  - `Module_X` becomes `VTK_MODULE_ENABLE_M` where X is the "library name"
    (e.g., `vtkPVCore` and `M` is the sanitized module name (e.g.,
    `ParaView::Core`'s sanitized name is `ParaView_Core`).
  - `PARAVIEW_BUILD_PLUGIN_P` becomes `PARAVIEW_PLUGIN_ENABLE_P`. Enabling a
    plugin now requests that its required modules are built.
  - ParaView's flags forcefully disable some modules. For example, ParaView
    can no longer be built with `PARAVIEW_USE_MPI` and still build some
    MPI-releated modules.

## Server Manager XML

Server Manager XML files may now be attached to modules using
`paraview_server_manager_add_xmls`:

```cmake
paraview_server_manager_add_xmls(
  XMLS mysm.xml)
```

for the current module or:

```cmake
paraview_server_manager_add_xmls(
  MODULE Some::VTKModule
  XMLS mysm.xml)
```

to attach it to an external module.

Server Manager XML contents are then provided via the
`paraview_server_manager_process` function:

```cmake
paraview_server_manager_process(
  MODULES module1 module2
  TARGET  a_target_name
  XML_FILES list_of_xml_files_variable)
```

which provides a target which may be linked to in order to access the XML files
attached to the given modules. See `CMake/ParaViewServerManager.cmake` for
details.

## Plugins

ParaView's plugin CMake API has also been improved. Documentation is in
`CMake/ParaViewPlugin.cmake`. The biggest change is that for a plugin to
provide its own classes which are used from Server Manager XML files, the
classes must be part of a VTK module. ParaView's plugins have been updated to
use this new pattern, so looking in `Plugins` for examples is recommended.

In shared builds, plugins are now built using `add_library(MODULE)` which
means that they may not be directly linked to. When building clients, there
are options to specify plugins to load on startup.

In static builds, the `paraview_plugin_build`'s `TARGET` parameter creates a
target which may be linked to in order to initialize the built plugins. See
its documentation for details.

## Clients

The CMake API has been updated to be use CMake targets and stricter argument
parsing. See `CMake/ParaViewClient.cmake` for documentation.
