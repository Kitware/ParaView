#------------------------------------------------------------------------------
# Find Qt5 libraries and headers.
# If Qt5_FIND_COMPONENTS is defined as a list of Qt5 modules, only those modules
# will be looked for and added. If it is not defined, all modules will be looked
# for and added to the project.
#
# This file sets the following variables:
#
# QT5_FOUND (Indicating that Qt5 libraries found)
# QT_LIBRARIES (List of Qt5 libraries)
# QT_QMAKE_EXECUTABLE
# QT_RCC_EXECUTABLE
# QT_UIC_EXECUTABLE
# QT_QDBUSCPP2XML_EXECUTABLE
# QT_QDBUSXML2CPP_EXECUTABLE
#
# In addition, this file also includes the Qt5 library include dirs and adds the
# Qt5 library definitions
#------------------------------------------------------------------------------

FIND_PACKAGE( Qt5Core QUIET )

IF( NOT Qt5_FIND_COMPONENTS )
  SET (Qt5_FIND_COMPONENTS
    Concurrent
    Core
    DBus
    Declarative
    Designer
    Gui
    Help
    Network
    OpenGL
    PrintSupport
    Script
    ScriptTools
    Svg
    Test
    UiTools
    WebKit
    WebKitWidgets
    Widgets
    Xml
    )
ENDIF()

IF( Qt5Core_FOUND )
  # Check if Qtversion is >=QT_OFFICIAL_VERSION. If so, we are good. Otherwise we will post a
  # warning of versions (<QT_OFFICIAL_VERSION).

  STRING( REGEX MATCH "^5\\.[0]\\.[0-1]+" QT_VERSION_MATCH
    "${Qt5Core_VERSION_STRING}" )
  IF( QT_VERSION_MATCH )
    MESSAGE( WARNING "Warning: You are using Qt ${Qt5Core_VERSION_STRING}. "
      "Officially supported version is Qt ${QT_OFFICIAL_VERSION}" )
  ENDIF()
  FOREACH( _COMPONENT ${Qt5_FIND_COMPONENTS} )
    FIND_PACKAGE( Qt5${_COMPONENT} REQUIRED )
    INCLUDE_DIRECTORIES( ${Qt5${_COMPONENT}_INCLUDE_DIRS} )
    ADD_DEFINITIONS( ${Qt5${_COMPONENT}_DEFINITIONS} )
    LIST( APPEND QT_LIBRARIES ${Qt5${_COMPONENT}_LIBRARIES} )
  ENDFOREACH()
  SET( QT5_FOUND TRUE )

  GET_TARGET_PROPERTY( QT_QMAKE_EXECUTABLE Qt5::qmake LOCATION )
  GET_TARGET_PROPERTY( QT_RCC_EXECUTABLE Qt5::rcc LOCATION )
  GET_TARGET_PROPERTY( QT_UIC_EXECUTABLE Qt5::uic LOCATION )

  IF( TARGET Qt5::qdbuscpp2xml )
    GET_TARGET_PROPERTY( QT_QDBUSCPP2XML_EXECUTABLE Qt5::qdbuscpp2xml LOCATION )
  ENDIF()

  IF( TARGET Qt5::qdbusxml2cpp )
    GET_TARGET_PROPERTY(QT_QDBUSXML2CPP_EXECUTABLE Qt5::qdbusxml2cpp LOCATION )
  ENDIF()
ELSE()
  MESSAGE(SEND_ERROR "Error: Could not find Qt5.\nIf Qt5 is not installed in"
  " a standard location, a custom prefix for 'find_package' should be passed."
  "\nFor example, -DCMAKE_PREFIX_PATH:STRING=/path/to/qt5/build"
  "\nOr the prefix environment variable must be set."
  "\nFor example, export CMAKE_PREFIX_PATH=/path/to/qt5/build\n")
ENDIF()
