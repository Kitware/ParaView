INCLUDE(${ParaView_SOURCE_DIR}/CMake/ParaViewBranding.cmake)
INCLUDE(${ParaView_SOURCE_DIR}/CMake/ParaViewPlugins.cmake)

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
    SOURCES ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES}
)


