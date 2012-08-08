project(EyeDomeLighting)
cmake_minimum_required(VERSION 2.8.8)

if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView)
  include("${PARAVIEW_USE_FILE}")

  # Setup enviroment to build VTK modules outside of VTK source tree.
  set (VTK_INSTALL_EXPORT_NAME "EyeDomeLightingTargets")
  set (VTK_INSTALL_RUNTIME_DIR "bin")
  set (VTK_INSTALL_LIBRARY_DIR "lib")
  set (VTK_INSTALL_ARCHIVE_DIR "lib")
  set (VTK_INSTALL_INCLUDE_DIR "include")
  set (VTK_INSTALL_PACKAGE_DIR "lib/cmake/EyeDomeLighting")
  set (VTK_MODULES_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/${VTK_INSTALL_PACKAGE_DIR}/Modules")
  set (VTK_EXPORTS_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/EyeDomeLightingTargets.cmake")
  set (BUILD_SHARED_LIBS ${VTK_BUILD_SHARED_LIBS})

  include(vtkModuleMacros)
  include(vtkModuleAPI)
  include(vtkClientServerWrapping)

  # load information about existing modules.
  foreach (mod IN LISTS VTK_MODULES_ENABLED)
    vtk_module_load("${mod}")
  endforeach()
else()
  # we're building from within ParaView Source.
endif()

# process all vtk-modules defined in this plugin. This generates the necessary
# wrappings for all these modules as well.
pv_process_modules()
add_subdirectory(ParaViewPlugin)
