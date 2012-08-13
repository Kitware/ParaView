#------------------------------------------------------------------------------
# Add extra library containing custom code for the client.
QT4_WRAP_CPP(MOC_BUILT_SOURCES
  ParaViewMainWindow.h)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ParaViewMainWindow.ui
)

source_group("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

set (QT_USE_QTNETWORK TRUE)
include(${QT_USE_FILE})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

set(ParaView_SOURCE_FILES
  ParaViewMainWindow.cxx
  ParaViewMainWindow.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES})

set (python_dependencies)
if (PARAVIEW_ENABLE_PYTHON)
  include_directories("${PYTHON_INCLUDE_PATH}")

  # when building static ParaView, generate a header file to initialize
  # all the python wrapped modules statically.
  vtk_write_python_modules_header_for_wrapped_modules(
    "${CMAKE_CURRENT_BINARY_DIR}/paraviewpythonmodules.h"
    python_dependencies)
endif()

#------------------------------------------------------------------------------
# ParaView applications provides a mechanism to add gui xmls from modules.
# This is done by defining variables named ${vtk-module}_PARAVIEW_GUI_XMLS in
# the module.cmake file for the modules pointing to the paths for the
# GUI-xmls.  We process those here.
set (module_gui_xmls)
foreach (module IN LISTS VTK_MODULES_ENABLED)
  get_property(gui_xml GLOBAL PROPERTY ${module}_PARAVIEW_GUI_XMLS)
  if (gui_xml)
    foreach(xml IN LISTS gui_xml)
      list(APPEND module_gui_xmls ${xml})
    endforeach()
  endif()
endforeach()

#------------------------------------------------------------------------------
# Build the client
build_paraview_client(paraview
    APPLICATION_NAME "ParaView"
    TITLE "ParaView ${PARAVIEW_VERSION_FULL} ${PARAVIEW_BUILD_ARCHITECTURE}-bit"
    ORGANIZATION  "ParaView"
    VERSION_MAJOR ${PARAVIEW_VERSION_MAJOR} 
    VERSION_MINOR ${PARAVIEW_VERSION_MINOR}
    VERSION_PATCH ${PARAVIEW_VERSION_PATCH}
    SPLASH_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/PVSplashScreen.png"
    PVMAIN_WINDOW ParaViewMainWindow
    PVMAIN_WINDOW_INCLUDE ParaViewMainWindow.h
    BUNDLE_ICON   "${CMAKE_CURRENT_SOURCE_DIR}/MacIcon.icns"
    APPLICATION_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/WinIcon.ico"
    GUI_CONFIGURATION_XMLS
      ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewSources.xml
      ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewFilters.xml
      ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewReaders.xml
      ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewWriters.xml
      ${module_gui_xmls} # Add any compiled in GUI configuration xmls
    COMPRESSED_HELP_FILE "${ParaView_BINARY_DIR}/Documentation/paraview.qch"
    SOURCES ${ParaView_SOURCE_FILES}
    INSTALL_BIN_DIR "${VTK_INSTALL_RUNTIME_DIR}"
    INSTALL_LIB_DIR "${VTK_INSTALL_LIBRARY_DIR}")

# link enabled plugins if not building in shared library mode
if(NOT BUILD_SHARED_LIBS)
  target_link_libraries(paraview ${PARAVIEW_PLUGINLIST})
endif(NOT BUILD_SHARED_LIBS)

# add dependecies to linked python modules These are non-empty only when
# building statically.
target_link_libraries(paraview ${python_dependencies})

# This ensure that the paraview.qch file is generated.
add_dependencies(paraview
  ParaViewOnlineHelp)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif (BUILD_TESTING)
