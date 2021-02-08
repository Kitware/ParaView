cmake_minimum_required(VERSION 3.8)
project(ComplexPluginArchitecture)

find_package(ParaView REQUIRED)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

# Adding both plugins
set("_paraview_plugin_default_ComplexModuleArchitecture" ON)
set("_paraview_plugin_default_ComplexGUIMyToolBar" ON)
paraview_plugin_scan(
  PLUGIN_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ComplexModuleArchitecture/paraview.plugin"
    "${CMAKE_CURRENT_SOURCE_DIR}/ComplexGUIMyToolBar/paraview.plugin"
  PROVIDES_PLUGINS  plugins
  REQUIRES_MODULES  required_modules)

# Adding a vtk module that will be used by both plugins
vtk_module_scan(
  MODULE_FILES        "${CMAKE_CURRENT_SOURCE_DIR}/Shared/vtk.module"
  REQUEST_MODULES     ComplexPluginArchitecture::Shared
  PROVIDES_MODULES    my_modules)

vtk_module_build(
  MODULES             ${my_modules}
  INSTALL_EXPORT      ComplexPluginArchitecture)

foreach (module IN LISTS required_modules)
  if (NOT TARGET "${module}")
    message(#FATAL_ERROR
      "Skipping example ${CMAKE_PROJECT_NAME}: Missing required module: "
      "${module}")
    return ()
  endif ()
endforeach ()

paraview_plugin_build(
  RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY_SUBDIRECTORY "${PARAVIEW_PLUGIN_SUBDIR}"
  PLUGINS ${plugins})
