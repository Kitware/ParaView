if (NOT (DEFINED paraview_cmake_dir AND
         DEFINED paraview_cmake_build_dir AND
         DEFINED paraview_cmake_destination AND
         DEFINED paraview_modules))
  message(FATAL_ERROR
    "ParaViewInstallCMakePackage is missing input variables.")
endif ()

configure_file(
  "${paraview_cmake_dir}/paraview-config.cmake.in"
  "${paraview_cmake_build_dir}/paraview-config.cmake"
  @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${paraview_cmake_build_dir}/paraview-config-version.cmake"
  VERSION "${PARAVIEW_VERSION_FULL}"
  COMPATIBILITY AnyNewerVersion)

# For convenience, a package is written to the top of the build tree. At some
# point, this should probably be deprecated and warn when it is used.
file(GENERATE
  OUTPUT  "${CMAKE_BINARY_DIR}/paraview-config.cmake"
  CONTENT "include(\"${paraview_cmake_build_dir}/paraview-config.cmake\")\n")
configure_file(
  "${paraview_cmake_build_dir}/paraview-config-version.cmake"
  "${CMAKE_BINARY_DIR}/paraview-config-version.cmake"
  COPYONLY)

set(paraview_cmake_module_files
  FindCGNS.cmake

  # Client API
  paraview_client_initializer.cxx.in
  paraview_client_initializer.h.in
  paraview_client_main.cxx.in
  ParaViewClient.cmake
  paraview_servermanager_convert_categoryindex.xsl
  paraview_servermanager_convert_html.xsl
  paraview_servermanager_convert_wiki.xsl.in
  paraview_servermanager_convert_xml.xsl

  # Plugin API
  paraview_plugin.cxx.in
  paraview_plugin.h.in
  ParaViewPlugin.cmake
  pqActionGroupImplementation.cxx.in
  pqActionGroupImplementation.h.in
  pqAutoStartImplementation.cxx.in
  pqAutoStartImplementation.h.in
  pqDockWindowImplementation.cxx.in
  pqDockWindowImplementation.h.in
  pqGraphLayoutStrategyImplementation.cxx.in
  pqGraphLayoutStrategyImplementation.h.in
  pqPropertyWidgetInterface.cxx.in
  pqPropertyWidgetInterface.h.in
  pqServerManagerModelImplementation.cxx.in
  pqServerManagerModelImplementation.h.in
  pqToolBarImplementation.cxx.in
  pqToolBarImplementation.h.in
  pqTreeLayoutStrategyImplementation.cxx.in
  pqTreeLayoutStrategyImplementation.h.in
  pqViewFrameActionGroupImplementation.cxx.in
  pqViewFrameActionGroupImplementation.h.in

  # ServerManager API
  ParaViewServerManager.cmake

  # Testing
  ParaViewTesting.cmake

  # Client Server
  vtkModuleWrapClientServer.cmake)

set(paraview_cmake_files_to_install)
foreach (paraview_cmake_module_file IN LISTS paraview_cmake_module_files)
  configure_file(
    "${paraview_cmake_dir}/${paraview_cmake_module_file}"
    "${paraview_cmake_build_dir}/${paraview_cmake_module_file}"
    COPYONLY)
  list(APPEND paraview_cmake_files_to_install
    "${paraview_cmake_module_file}")
endforeach ()

foreach (paraview_cmake_file IN LISTS paraview_cmake_files_to_install)
  get_filename_component(subdir "${paraview_cmake_file}" DIRECTORY)
  install(
    FILES       "${paraview_cmake_dir}/${paraview_cmake_file}"
    DESTINATION "${paraview_cmake_destination}/${subdir}"
    COMPONENT   "development")
endforeach ()

install(
  FILES       "${paraview_cmake_build_dir}/paraview-config.cmake"
              "${paraview_cmake_build_dir}/paraview-config-version.cmake"
  DESTINATION "${paraview_cmake_destination}"
  COMPONENT   "development")

vtk_module_export_find_packages(
  CMAKE_DESTINATION "${paraview_cmake_destination}"
  FILE_NAME         "ParaView-vtk-module-find-packages.cmake"
  MODULES           ${paraview_modules})
