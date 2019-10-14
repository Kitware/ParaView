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

include(ParaViewInstallCMakePackageHelpers)
if (NOT PARAVIEW_RELOCATABLE_INSTALL)
  list(APPEND paraview_cmake_files_to_install
    "${paraview_cmake_build_dir}/paraview-find-package-helpers.cmake")
endif ()

foreach (paraview_cmake_file IN LISTS paraview_cmake_files_to_install)
  if (IS_ABSOLUTE "${paraview_cmake_file}")
    file(RELATIVE_PATH paraview_cmake_subdir_root "${paraview_cmake_build_dir}" "${paraview_cmake_file}")
    get_filename_component(paraview_cmake_subdir "${paraview_cmake_subdir_root}" DIRECTORY)
    set(paraview_cmake_original_file "${paraview_cmake_file}")
  else ()
    get_filename_component(paraview_cmake_subdir "${paraview_cmake_file}" DIRECTORY)
    set(paraview_cmake_original_file "${paraview_cmake_dir}/${paraview_cmake_file}")
  endif ()
  install(
    FILES       "${paraview_cmake_original_file}"
    DESTINATION "${paraview_cmake_destination}/${paraview_cmake_subdir}"
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

if (PARAVIEW_BUILD_QT_GUI)
  get_property(paraview_client_xml_files GLOBAL
    PROPERTY paraview_client_xml_files)
  get_property(paraview_client_xml_destination GLOBAL
    PROPERTY paraview_client_xml_destination)

  set(cmake_xml_file_name "ParaView-client-xml.cmake")
  set(cmake_xml_build_file "${paraview_cmake_build_dir}/${cmake_xml_file_name}")
  set(cmake_xml_install_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${cmake_xml_file_name}.install")

  file(WRITE "${cmake_xml_build_file}" "")
  file(WRITE "${cmake_xml_install_file}" "")
  _vtk_module_write_import_prefix("${cmake_xml_install_file}" "${paraview_client_xml_destination}")
  file(APPEND "${cmake_xml_build_file}"
    "set(\"\${CMAKE_FIND_PACKAGE_NAME}_CLIENT_XML_FILES\")\n")
  file(APPEND "${cmake_xml_install_file}"
    "set(\"\${CMAKE_FIND_PACKAGE_NAME}_CLIENT_XML_FILES\")\n")

  foreach (paraview_client_xml_file IN LISTS paraview_client_xml_files)
    get_filename_component(basename "${paraview_client_xml_file}" NAME)
    file(APPEND "${cmake_xml_build_file}"
      "list(APPEND \"\${CMAKE_FIND_PACKAGE_NAME}_CLIENT_XML_FILES\"
  \"${paraview_client_xml_file}\")\n")
    file(APPEND "${cmake_xml_install_file}"
      "list(APPEND \"\${CMAKE_FIND_PACKAGE_NAME}_CLIENT_XML_FILES\"
  \"\${_vtk_module_import_prefix}/${paraview_client_xml_destination}/${basename}\")\n")
  endforeach ()

  install(
    FILES       "${cmake_xml_install_file}"
    DESTINATION "${paraview_cmake_destination}"
    RENAME      "${cmake_xml_file_name}"
    COMPONENT   "development")
endif ()
