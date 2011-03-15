
cmake_minimum_required(VERSION 2.6)

project(Clone2)

FIND_PACKAGE(ParaView REQUIRED)
FIND_PACKAGE(Qt4)
INCLUDE(${PARAVIEW_USE_FILE})

INCLUDE(${ParaView_CMAKE_DIR}/ParaViewBranding.cmake)
INCLUDE(${ParaView_CMAKE_DIR}/ParaViewPlugins.cmake)

#------------------------------------------------------------------------------
# Add extra library containing custom code for the client.
QT4_WRAP_CPP(MOC_BUILT_SOURCES
  myMainWindow.h)

QT4_WRAP_UI(UI_BUILT_SOURCES
  myMainWindow.ui
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

SET(ParaView_SOURCE_FILES
  myMainWindow.cxx
  myMainWindow.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES})

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

build_paraview_client(paraview_revised_2
  TITLE "ParaView (ReVisEd)"
  ORGANIZATION  "Kitware Inc."
  VERSION_MAJOR 1 
  VERSION_MINOR 1
  VERSION_PATCH 1
#  SPLASH_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/RSplash.png"
  PVMAIN_WINDOW myMainWindow
  PVMAIN_WINDOW_INCLUDE myMainWindow.h
  GUI_CONFIGURATION_XMLS
    ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewSources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewFilters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewReaders.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/ParaViewWriters.xml
  SOURCES ${ParaView_SOURCE_FILES}
)

TARGET_LINK_LIBRARIES(paraview_revised_2 ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY}  vtkCommon vtkPVCommon pqCore pqComponents)
