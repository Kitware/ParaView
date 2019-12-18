cmake_dependent_option(PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION
  "Build the ParaView help menu documentation for ParaView and its plugins" ON
  "PARAVIEW_USE_QT" ON)
mark_as_advanced(PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)

set(xmls
  "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewFilters.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewSources.xml")

if (PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
  #------------------------------------------------------------------------------
  # Build Online-Help (aka Embedded Help) for the ParaView application.
  # This is done after the above piece of code that sets the application_gui_xmls
  # variable. Documentation/CMakeLists.txt depends on it.
  add_subdirectory(Documentation)
endif ()

if (DEFINED PARAVIEW_VERSION_NICKNAME)
  set(paraview_version "${PARAVIEW_VERSION_NICKNAME}")
else ()
  set(paraview_version "${PARAVIEW_VERSION_FULL}")
endif ()
set(paraview_title "")

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTOUIC 1)

set(sources
  ParaViewMainWindow.cxx
  ParaViewMainWindow.h
  ParaViewMainWindow.ui)

find_package(Qt5 REQUIRED QUIET COMPONENTS Core Widgets)

if (APPLE)
  # Enable high resolution when using Qt5
  # This configuring of MacOSXBundleInfo may no longer be needed, but I am
  # leaving it in since in future we could build the supported extensions list
  # more exhaustively here.
  set(high_resolution_capable "true")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MacOSXBundleInfo.plist.in"
    @ONLY)
  unset(high_resolution_capable)
endif ()

set(default_style)
if (WIN32)
  set(default_style "fusion")
endif ()

set(style_args)
if (default_style)
  list(APPEND style_args
    DEFAULT_STYLE "${default_style}")
endif ()

paraview_client_add(
  NAME              paraview
  NAMESPACE         "ParaView"
  EXPORT            "ParaViewClient"
  APPLICATION_NAME  "ParaView"
  TITLE             "ParaView ${paraview_version}"
  ORGANIZATION      "ParaView"
  VERSION           "${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}.${PARAVIEW_VERSION_PATCH}"
  ${style_args}
  MAIN_WINDOW_CLASS "ParaViewMainWindow"
  BUNDLE_ICON       "${CMAKE_CURRENT_SOURCE_DIR}/pvIcon.icns"
  BUNDLE_PLIST      "${CMAKE_CURRENT_BINARY_DIR}/MacOSXBundleInfo.plist.in"
  APPLICATION_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/pvIcon.ico"
  SOURCES           ${sources}
  PLUGINS_TARGETS   ParaView::paraview_plugins
  APPLICATION_XMLS  ${xmls})

find_package(Qt5 REQUIRED QUIET COMPONENTS Widgets)

target_link_libraries(paraview
  PRIVATE
    ParaView::RemotingSettings
    Qt5::Core
    Qt5::Widgets)

if (PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
  target_compile_definitions(paraview
    PRIVATE
      PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
  target_link_libraries(paraview
    PRIVATE
      vtkParaViewDocumentation)
endif ()

if (PARAVIEW_USE_PYTHON)
  target_compile_definitions(paraview
    PRIVATE
      PARAVIEW_USE_PYTHON)
  target_link_libraries(paraview
    PRIVATE
      ParaView::pvpythonmodules
      ParaView::pqPython
      ParaView::PythonInitializer)
endif ()

if (PARAVIEW_INSTALL_DEVELOPMENT_FILES)
  export(
    EXPORT    ParaViewClient
    NAMESPACE ParaView::
    FILE      "${CMAKE_BINARY_DIR}/${paraview_cmake_destination}/ParaViewClient-targets.cmake")
  install(
    EXPORT      ParaViewClient
    NAMESPACE   ParaView::
    FILE        ParaViewClient-targets.cmake
    DESTINATION "${paraview_cmake_destination}"
    COMPONENT   "development")
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()

if (UNIX AND NOT APPLE)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/org.paraview.ParaView.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/org.paraview.ParaView.desktop"
    @ONLY)
  install(
    FILES       "${CMAKE_CURRENT_BINARY_DIR}/org.paraview.ParaView.desktop"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    COMPONENT   runtime)
  foreach (iconsize IN ITEMS 22x22 32x32 96x96)
    install(
      FILES       "pvIcon-${iconsize}.png"
      DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${iconsize}/apps"
      RENAME      paraview.png
      COMPONENT   runtime)
  endforeach ()
  install(
    FILES       org.paraview.ParaView.appdata.xml
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/metainfo"
    COMPONENT   runtime)
endif ()

set(paraview_xml_destination
  "${CMAKE_INSTALL_DATAROOTDIR}/paraview${paraview_version_suffix}/xmls")
install(
  FILES       ${xmls}
  DESTINATION "${paraview_xml_destination}"
  COMPONENT   "development")

set_property(GLOBAL APPEND
  PROPERTY
    paraview_client_xml_files "${xmls}")
set_property(GLOBAL
  PROPERTY
    paraview_client_xml_destination "${paraview_xml_destination}")
