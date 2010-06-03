cmake_minimum_required(VERSION 2.6)

project(SpreadSheet)

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

# Variables that need to be set for installation code to work correctly.
SET (BUILD_SHARED_LIBS ${PARAVIEW_BUILD_SHARED_LIBS}) # enable shared builds.

SET (SOURCE_FILES
  SpreadSheetMainWindow.cxx
  SpreadSheetMainWindow.h
)

QT4_WRAP_CPP(MOC_SRCS
  SpreadSheetMainWindow.h
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  SpreadSheetMainWindow.ui
)

##########################################################################
## Add resources (icons etc.)
SET (UI_RESOURCES
  SpreadSheet.qrc
)

QT4_ADD_RESOURCES(RCS_BUILT_SOURCES
  ${UI_RESOURCES}
)

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})


#------------------------------------------------------------------------------
# Build the client
build_paraview_client(spreadsheet
    TITLE "Raw Data Viewer (ParaView-based Example Application)"
    ORGANIZATION  "Kitware Inc"
    VERSION_MAJOR 1
    VERSION_MINOR 0
    VERSION_PATCH 0
    PVMAIN_WINDOW SpreadSheetMainWindow
    PVMAIN_WINDOW_INCLUDE SpreadSheetMainWindow.h
    GUI_CONFIGURATION_XMLS
      ${CMAKE_CURRENT_SOURCE_DIR}/SupportedReader.xml
    SOURCES ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES} ${RCS_BUILT_SOURCES}
)

#------------------------------------------------------------------------
# Install ParaView client
build_paraview_client_cpack_config_init(
    PACKAGE_NAME "SpreadSheet"
    ORGANIZATION "Kitware Inc."
    VERSION_MAJOR "1"
    VERSION_MINOR "0"
    VERSION_PATCH "0"
    DESCRIPTION "ParaView is a scientific visualization tool"
    PACKAGE_EXECUTABLES "spreadsheet;SpreadSheet"
    LICENSE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/README.txt
    DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/README.txt
)

build_paraview_client_cpack_config()
