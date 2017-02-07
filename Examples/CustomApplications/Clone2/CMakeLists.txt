cmake_minimum_required(VERSION 3.3)
project(Clone2)

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})
if (NOT PARAVIEW_BUILD_QT_GUI)
  message(FATAL_ERROR
    "Clone2 example requires PARAVIEW_BUILD_QT_GUI to be enabled. "
    "Please rebuild ParaView (or point to a different build of ParaView) "
    "with PARAVIEW_BUILD_QT_GUI set to TRUE")
endif()

# include rules for creating custom applications.
include(ParaViewBranding)

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

#------------------------------------------------------------------------------
# Add extra library containing custom code for the client.
IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(MOC_BUILT_SOURCES
    myMainWindow.h)
  QT5_WRAP_UI(UI_BUILT_SOURCES
    myMainWindow.ui)
ELSE ()
  QT4_WRAP_CPP(MOC_BUILT_SOURCES
    myMainWindow.h)
  QT4_WRAP_UI(UI_BUILT_SOURCES
    myMainWindow.ui)
ENDIF ()

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
  SOURCES ${ParaView_SOURCE_FILES}
)
