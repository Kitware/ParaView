cmake_minimum_required(VERSION 3.3)
project(SpreadSheet)

find_package(ParaView REQUIRED)
include(${PARAVIEW_USE_FILE})
if (NOT PARAVIEW_BUILD_QT_GUI)
  message(FATAL_ERROR
    "SpreadSheet example requires PARAVIEW_BUILD_QT_GUI to be enabled. "
    "Please rebuild ParaView (or point to a different build of ParaView) "
    "with PARAVIEW_BUILD_QT_GUI set to TRUE")
endif()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

# Variables that need to be set for installation code to work correctly.
SET (BUILD_SHARED_LIBS ${PARAVIEW_BUILD_SHARED_LIBS}) # enable shared builds.

SET (SOURCE_FILES
  SpreadSheetMainWindow.cxx
  SpreadSheetMainWindow.h
)

IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(MOC_SRCS
    SpreadSheetMainWindow.h
  )
  QT5_WRAP_UI(UI_BUILT_SOURCES
    SpreadSheetMainWindow.ui
  )
ELSE ()
  QT4_WRAP_CPP(MOC_SRCS
    SpreadSheetMainWindow.h
  )
  QT4_WRAP_UI(UI_BUILT_SOURCES
    SpreadSheetMainWindow.ui
  )
ENDIF ()

##########################################################################
## Add resources (icons etc.)
SET (UI_RESOURCES
  SpreadSheet.qrc
)

IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_ADD_RESOURCES(RCS_BUILT_SOURCES
    ${UI_RESOURCES}
    )
ELSE ()
  QT4_ADD_RESOURCES(RCS_BUILT_SOURCES
    ${UI_RESOURCES}
    )
ENDIF ()

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})


#------------------------------------------------------------------------------
include(ParaViewBranding)

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
