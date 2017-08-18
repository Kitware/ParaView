#------------------------------------------------------------------------------
# Add extra library containing custom code for the client.
include(ParaViewQt)
set(qt5_comps Widgets)
set(qt4_comps QtGui)

pv_find_package_qt(qt_targets REQUIRED QUIET
                QT4_COMPONENTS ${qt4_comps}
                QT5_COMPONENTS ${qt5_comps})
pv_qt_wrap_cpp(MOC_BUILT_SOURCES ParaViewMainWindow.h)
pv_qt_wrap_ui(UI_BUILT_SOURCES ParaViewMainWindow.ui)
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Documentation)

set(ParaView_SOURCE_FILES
  ParaViewMainWindow.cxx
  ParaViewMainWindow.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES})

#------------------------------------------------------------------------------
# ParaView applications provides a mechanism to add gui xmls from modules.
# This is done by defining variables named ${vtk-module}_PARAVIEW_GUI_XMLS in
# the module.cmake file for the modules pointing to the paths for the
# GUI-xmls.  We process those here.
set (application_gui_xmls
       "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewSources.xml"
       "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewFilters.xml")
foreach (module IN LISTS VTK_MODULES_ENABLED)
  get_property(gui_xml GLOBAL PROPERTY ${module}_PARAVIEW_GUI_XMLS)
  if (gui_xml)
    foreach(xml IN LISTS gui_xml)
      list(APPEND application_gui_xmls ${xml})
    endforeach()
  endif()
endforeach()

if (PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
  #------------------------------------------------------------------------------
  # Build Online-Help (aka Embedded Help) for the ParaView application.
  # This is done after the above piece of code that sets the application_gui_xmls
  # variable. Documentation/CMakeLists.txt depends on it.
  add_subdirectory(Documentation)
endif()

#------------------------------------------------------------------------------
# FIXME: This is necessary because the vtkPVStaticPluginsInit library cannot be
# exported (because exporting plugins had other issues). We can't put the
# paraview_static_plugins_init function call anywhere ParaView-specific because
# anything we do would be too late to have any effect (it must be before
# `.plugins` is loaded from the pq@BPC_NAME@Initializer.Initialize ->
# pqApplicationCore::pqApplicationCore -> vtkPVPluginTracker::GetInstance call
# chain occurs).
#
# Basically, what needs to happen to avoid this hack is for, in static builds,
# branded clients to build their own vtkPVStaticPluginsInit library with any
# plugins available at that time and link to it. We cannot do this in
# build_paraview_client right now because the command line executables in
# ParaView itself needs vtkPVStaticPluginsInit as well.
set(SUPPORT_STATIC_PLUGINS TRUE)

set(title "ParaView ${PARAVIEW_VERSION_FULL} ${PARAVIEW_BUILD_ARCHITECTURE}-bit")
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL")
  set(title "${title} (Legacy Rendering Backend)")
endif()
#------------------------------------------------------------------------------
# Build the client
build_paraview_client(paraview
    APPLICATION_NAME "ParaView"
    TITLE ${title}
    ORGANIZATION  "ParaView"
    VERSION_MAJOR ${PARAVIEW_VERSION_MAJOR}
    VERSION_MINOR ${PARAVIEW_VERSION_MINOR}
    VERSION_PATCH ${PARAVIEW_VERSION_PATCH}
    PVMAIN_WINDOW ParaViewMainWindow
    PVMAIN_WINDOW_INCLUDE ParaViewMainWindow.h
    BUNDLE_ICON   "${CMAKE_CURRENT_SOURCE_DIR}/pvIcon.icns"
    APPLICATION_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/pvIcon.ico"
    GUI_CONFIGURATION_XMLS
      ${application_gui_xmls} # Add any compiled in GUI configuration xmls
    SOURCES ${ParaView_SOURCE_FILES}
    INSTALL_RUNTIME_DIR "${VTK_INSTALL_RUNTIME_DIR}"
    INSTALL_LIBRARY_DIR "${VTK_INSTALL_LIBRARY_DIR}"
    INSTALL_ARCHIVE_DIR "${VTK_INSTALL_ARCHIVE_DIR}"
    )

target_link_libraries(paraview
  LINK_PRIVATE ${qt_targets} vtkPVServerManagerDefault)

if (PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
  # Link against the documentation module.
  target_link_libraries(paraview
    LINK_PRIVATE vtkParaViewDocumentation)
endif()

# link enabled plugins if not building in shared library mode and
# add dependecies to linked python modules These are non-empty only when
# building statically.
if (PARAVIEW_ENABLE_PYTHON)
  target_link_libraries(paraview 
    LINK_PRIVATE vtkUtilitiesPythonInitializer)
endif()

if(NOT BUILD_SHARED_LIBS)
  target_link_libraries(paraview
    LINK_PRIVATE vtkPVStaticPluginsInit)
endif()


if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()

#------------------------------------------------------------------------------
if (APPLE AND NOT PARAVIEW_DO_UNIX_STYLE_INSTALLS)
  # For Macs, we add install rule to package everything that's built into a single
  # App. Look at the explanation of MACOSX_APP_INSTALL_PREFIX in the top-level
  # CMakeLists.txt file for details.

  # add install rules to generate the App bundle.
  install(CODE "
   include(\"${ParaView_CMAKE_DIR}/ParaViewBrandingInstallApp.cmake\")

   #fillup bundle with all the libraries and plugins.
   cleanup_bundle(
     \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_RUNTIME_DIR}/paraview.app/Contents/MacOS/paraview
     \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_RUNTIME_DIR}/paraview.app
     \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_LIBRARY_DIR}
     \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_PLUGIN_DIR}
     \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_DATA_DIR})

   # Place the App at the requested location.
   file(INSTALL DESTINATION \"${MACOSX_APP_INSTALL_PREFIX}\"
        TYPE DIRECTORY FILES
          \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_RUNTIME_DIR}/paraview.app\"
        USE_SOURCE_PERMISSIONS)
   "
   COMPONENT Runtime)
elseif (APPLE AND PARAVIEW_DO_UNIX_STYLE_INSTALLS)
  # This is a unix style install on OsX. Purge the bundle.
  install(CODE
    "
      include(\"${ParaView_CMAKE_DIR}/ParaViewBrandingInstallApp.cmake\")
      convert_bundle_to_executable(
          \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_RUNTIME_DIR}/paraview.app/Contents/MacOS/paraview
          \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_RUNTIME_DIR}/paraview.app
          \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_RUNTIME_DIR})
    "
    COMPONENT Runtime)
elseif (UNIX)
  configure_file(
    paraview.desktop.in
    "${CMAKE_CURRENT_BINARY_DIR}/paraview.desktop"
    @ONLY)
  install(
    FILES       "${CMAKE_CURRENT_BINARY_DIR}/paraview.desktop"
    DESTINATION share/applications
    COMPONENT   runtime)
  foreach (iconsize 22x22 32x32 96x96)
    install(
      FILES       "pvIcon-${iconsize}.png"
      DESTINATION "share/icons/hicolor/${iconsize}/apps"
      RENAME      paraview.png
      COMPONENT   runtime)
  endforeach ()
  install(
    FILES       paraview.appdata.xml
    DESTINATION share/appdata
    COMPONENT   runtime)
endif()
