#------------------------------------------------------------------------------
# Add extra library containing custom code for the client.
QT4_WRAP_CPP(MOC_BUILT_SOURCES
  ParaViewMainWindow.h)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ParaViewMainWindow.ui
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

SET(ParaView_SOURCE_FILES
  ParaViewMainWindow.cxx
  ParaViewMainWindow.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES})

#------------------------------------------------------------------------------
# Build the client
build_paraview_client(paraview
    APPLICATION_NAME "ParaView"
    TITLE "ParaView ${PARAVIEW_VERSION_FULL}"
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
    COMPRESSED_HELP_FILE "${ParaView_BINARY_DIR}/Documentation/paraview.qch"
    SOURCES ${ParaView_SOURCE_FILES}
)

# Configure the ".plugins" configuration xml for making paraview aware of the
# distributed plugins.
SET (plugins_ini "<?xml version=\"1.0\"?>\n<Plugins>\n")
FOREACH(pluginname ${PARAVIEW_PLUGINLIST})
  set (plugins_ini "${plugins_ini}  <Plugin name=\"${pluginname}\" auto_load=\"0\"/>\n")
ENDFOREACH(pluginname ${PARAVIEW_PLUGINLIST})
set (plugins_ini "${plugins_ini}</Plugins>\n")

FILE(WRITE "${EXECUTABLE_OUTPUT_PATH}/.plugins" "${plugins_ini}")

# This ensure that the paraview.qch file is generated.
ADD_DEPENDENCIES(paraview
  ParaViewOnlineHelp)

IF (BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF (BUILD_TESTING)


#########################################################################
# Add install rules for installing certain dependencies.

#------------------------------------------------------------------------
# Install Qt 
SET (qtliblist QTCORE QTGUI QTNETWORK QTXML QTSQL QTHELP QTWEBKIT QTCLUCENE PHONON QTXMLPATTERNS)

# QTDBUS is Unix only
IF(NOT WIN32)
  LIST(APPEND qtliblist QTDBUS)
ENDIF(NOT WIN32)  

IF (VTK_USE_QVTK_QTOPENGL)
  LIST_APPEND(qtliblist QTOPENGL)
ENDIF (VTK_USE_QVTK_QTOPENGL)

install_qt_libs("${qtliblist}" BrandedRuntime)

#------------------------------------------------------------------------
# Install ffmpeg, if used.
FOREACH(fmpglib avcodec avformat avutil)
  IF (FFMPEG_${fmpglib}_LIBRARY)
    INSTALL(FILES ${FFMPEG_${fmpglib}_LIBRARY}
            DESTINATION ${PV_INSTALL_LIB_DIR}
            COMPONENT BrandedRuntime)
  ENDIF(FFMPEG_${fmpglib}_LIBRARY)
ENDFOREACH(fmpglib)

#------------------------------------------------------------------------
# Import site specific installation rules file, if any.
SET (PARAVIEW_EXTRA_INSTALL_RULES_FILE "" CACHE FILEPATH 
  "Path to site-specific install rules for ParaView.")

IF (EXISTS "${PARAVIEW_EXTRA_INSTALL_RULES_FILE}")
  INCLUDE("${PARAVIEW_EXTRA_INSTALL_RULES_FILE}")
ENDIF (EXISTS "${PARAVIEW_EXTRA_INSTALL_RULES_FILE}")

#------------------------------------------------------------------------
# Install the .plugins configuration file.
INSTALL(FILES "${EXECUTABLE_OUTPUT_PATH}/.plugins"
        DESTINATION ${PV_INSTALL_LIB_DIR}
        COMPONENT BrandedRuntime)

# Call this last so the install script runs after the above targets are installed:
#
IF(APPLE)
  SET(distributed_plugins ${PARAVIEW_PLUGINLIST})
  SET(PV_EXE_NAME "paraview")
  SET(OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT
    "${CMAKE_CURRENT_BINARY_DIR}/ParaView_OSX_MakeStandAloneBundle.cmake")
  SET(OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT
    "${CMAKE_CURRENT_BINARY_DIR}/ParaView_OSX_MakeStandAloneBundle.sh")

  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CompleteBundle.cmake.in"
    "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" @ONLY IMMEDIATE)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CreateBundle.sh.in"
    "${OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT}" @ONLY IMMEDIATE)

  INSTALL(SCRIPT "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" COMPONENT Bundle)
ENDIF(APPLE)

#------------------------------------------------------------------------
# Install ParaView client
build_paraview_client_cpack_config_init(
    PACKAGE_NAME "ParaView"
    ORGANIZATION "Kitware Inc."
    VERSION_MAJOR "${PARAVIEW_VERSION_MAJOR}"
    VERSION_MINOR "${PARAVIEW_VERSION_MINOR}"
    VERSION_PATCH "${PARAVIEW_VERSION_PATCH}"
    DESCRIPTION "ParaView is a scientific visualization tool"
    LICENSE_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
    DESCRIPTION_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
    PACKAGE_EXECUTABLES "paraview;ParaView"
)

# This ensure that the only component that is used by cpack when building the
# drag-n-drop on Mac is the "Bundle" component.
SET (CPACK_INSTALL_CMAKE_PROJECTS_DRAGNDROP
  "${CMAKE_CURRENT_BINARY_DIR}" "ParaView Mac Bundle" "Bundle" "/")

build_paraview_client_cpack_config()
#########################################################################
