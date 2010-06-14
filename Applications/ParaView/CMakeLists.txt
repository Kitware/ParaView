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
# Add install rules for required system runtimes such as MSVCRxx.dll
SET (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1)
INCLUDE(InstallRequiredSystemLibraries)
IF (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
  INSTALL(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
    DESTINATION ${PV_INSTALL_BIN_DIR}
    PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
    COMPONENT Runtime)

  # Install the runtimes to the lib dir as well since python modules are
  # installed in that directory and the manifest files need to present there as
  # well.
  INSTALL(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
    DESTINATION ${PV_INSTALL_LIB_DIR}
    PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
    COMPONENT Runtime)
ENDIF (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)

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
      ${PARAVIEW_EXTRA_GUI_RESOURCES} # Add any compiled in GUI configuration xmls
    COMPRESSED_HELP_FILE "${ParaView_BINARY_DIR}/Documentation/paraview.qch"
    SOURCES ${ParaView_SOURCE_FILES}
    INSTALL_BIN_DIR "${PV_INSTALL_BIN_DIR}"
    INSTALL_LIB_DIR "${PV_INSTALL_LIB_DIR}"
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
ADD_DEPENDENCIES(${paraview_client_real_exe_name}
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
  LIST(APPEND qtliblist QTOPENGL)
ENDIF (VTK_USE_QVTK_QTOPENGL)

IF (PARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES)
   install_qt_libs("${qtliblist}" BrandedRuntime)
  
  if(WIN32)
    # Now install the plugins for sqldrivers. For use by the help system
    INSTALL(DIRECTORY ${QT_PLUGINS_DIR}/sqldrivers 
            DESTINATION ${PV_INSTALL_BIN_DIR}
            COMPONENT Runtime
            FILES_MATCHING
              PATTERN "*.so"
              PATTERN "*.dll")
  else(WIN32)
    # Now install the plugins for sqldrivers. For use by the help system
    INSTALL(DIRECTORY ${QT_PLUGINS_DIR}/sqldrivers 
            DESTINATION ${PV_INSTALL_LIB_DIR}
            COMPONENT Runtime
            FILES_MATCHING
              PATTERN "*.so"
              PATTERN "*.dll")
  endif(WIN32)

  #------------------------------------------------------------------------
  # Install ffmpeg, if used.
  IF (VTK_USE_FFMPEG_ENCODER)
    FOREACH(fmpglib avcodec avformat avutil swscale)
      IF (FFMPEG_${fmpglib}_LIBRARY)
        pv_install_library(${FFMPEG_${fmpglib}_LIBRARY}
          ${PV_INSTALL_LIB_DIR}
          BrandedRuntime)
      ENDIF(FFMPEG_${fmpglib}_LIBRARY)
    ENDFOREACH(fmpglib)
  ENDIF (VTK_USE_FFMPEG_ENCODER)
ENDIF(PARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES)

#------------------------------------------------------------------------
# Import site specific installation rules file, if any.
SET (PARAVIEW_EXTRA_INSTALL_RULES_FILE "" CACHE FILEPATH 
  "Path to site-specific install rules for ParaView.")
  
MARK_AS_ADVANCED(PARAVIEW_EXTRA_INSTALL_RULES_FILE)

IF (EXISTS "${PARAVIEW_EXTRA_INSTALL_RULES_FILE}")
  INCLUDE("${PARAVIEW_EXTRA_INSTALL_RULES_FILE}")
ENDIF (EXISTS "${PARAVIEW_EXTRA_INSTALL_RULES_FILE}")

#------------------------------------------------------------------------
# Install the .plugins configuration file.
INSTALL(FILES "${EXECUTABLE_OUTPUT_PATH}/.plugins"
        DESTINATION ${PV_INSTALL_PLUGIN_DIR}
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
    
  SET (QT_SQLLITE_PLUGINS_DIR ${QT_PLUGINS_DIR}/sqldrivers)

  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CompleteBundle.cmake.in"
    "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" @ONLY IMMEDIATE)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CreateBundle.sh.in"
    "${OSX_MAKE_STANDALONE_BUNDLE_BASH_SCRIPT}" @ONLY IMMEDIATE)

  INSTALL(SCRIPT "${OSX_MAKE_STANDALONE_BUNDLE_CMAKE_SCRIPT}" COMPONENT Bundle)
ENDIF(APPLE)

SET(_VERSION_PATCH "${PARAVIEW_VERSION_PATCH}")
IF(PARAVIEW_VERSION_RC)
  SET(_VERSION_PATCH "${PARAVIEW_VERSION_PATCH}-RC${PARAVIEW_VERSION_RC}")
ENDIF(PARAVIEW_VERSION_RC)

SET(_package_name "ParaView")
IF(PARAVIEW_INSTALL_DEVELOPMENT)
  SET(_package_name "ParaView-Development")
ENDIF(PARAVIEW_INSTALL_DEVELOPMENT)

#------------------------------------------------------------------------
# Install ParaView client
build_paraview_client_cpack_config_init(
    PACKAGE_NAME ${_package_name}
    ORGANIZATION "Kitware Inc."
    VERSION_MAJOR "${PARAVIEW_VERSION_MAJOR}"
    VERSION_MINOR "${PARAVIEW_VERSION_MINOR}"
    VERSION_PATCH ${_VERSION_PATCH}
    DESCRIPTION "ParaView is a scientific visualization tool"
    LICENSE_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
    DESCRIPTION_FILE "${ParaView_SOURCE_DIR}/License_v1.2.txt"
    PACKAGE_EXECUTABLES "paraview;ParaView"
)

# Set NSIS specific CPACK vars for install icons, strings etc..
IF(WIN32 AND NOT UNIX)
    # There is a bug in NSI that does not handle full unix paths properly. Make
    # sure there is at least one set of four (4) backlasshes.
    SET(CPACK_PACKAGE_ICON "${ParaView_SOURCE_DIR}/Applications/ParaView\\\\pvIcon.png")
    SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\paraview.exe")
    SET(CPACK_NSIS_DISPLAY_NAME "${_package_name}-${PARAVIEW_VERSION_FULL} a cross-platform, open-source visualization system")
    SET(CPACK_NSIS_PACKAGE_NAME "${_package_name}-${PARAVIEW_VERSION_FULL}")
    SET(CPACK_NSIS_HELP_LINK "http://www.paraview.org")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http://www.kitware.com")
    SET(CPACK_NSIS_CONTACT "webmaster@paraview.org")
    SET(CPACK_NSIS_MODIFY_PATH OFF)
    SET(CPACK_NSIS_MUI_ICON "${ParaView_SOURCE_DIR}/Applications/ParaView\\\\WinIcon.ico")
    SET(CPACK_NSIS_MUI_UNIICON "${ParaView_SOURCE_DIR}/Applications/ParaView\\\\WinIcon.ico")
  
ENDIF(WIN32 AND NOT UNIX)

# This ensure that the only component that is used by cpack when building the
# drag-n-drop on Mac is the "Bundle" component.
SET (CPACK_INSTALL_CMAKE_PROJECTS_DRAGNDROP
  "${CMAKE_CURRENT_BINARY_DIR}" "ParaView Mac Bundle" "Bundle" "/")

build_paraview_client_cpack_config()
#########################################################################
