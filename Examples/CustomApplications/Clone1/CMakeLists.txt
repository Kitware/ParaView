cmake_minimum_required(VERSION 3.8)
project(Clone1)

find_package(ParaView REQUIRED)

if (NOT PARAVIEW_USE_QT)
  message(STATUS
    "Skipping example: ${PROJECT_NAME} example requires PARAVIEW_USE_QT "
    "to be enabled. Please rebuild ParaView (or point to a different build of "
    "ParaView) with PARAVIEW_USE_QT set to TRUE")
  return ()
endif()

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTOUIC 1)

set(sources
  myApplication.cxx
  myApplication.h
  myMainWindow.cxx
  myMainWindow.h)
set(ui_files
  myMainWindow.ui)
set(xml_files
  ParaViewFilters.xml
  ParaViewSources.xml)

# also used in testing
set (app_name "paraview_clone1")
set (organization_name "Kitware, Inc")
set (app_version "1.1.1")

paraview_client_add(
  NAME ${app_name}
  VERSION ${app_version}
  SOURCES ${sources} ${ui_files}
  APPLICATION_XMLS ${xml_files}
  MAIN_WINDOW_CLASS myMainWindow
  APPLICATION_CORE_CLASS myApplicationCore
  APPLICATION_CORE_INCLUDE "myApplication.h"
  ORGANIZATION ${organization_name}
  TITLE "ParaView (ReVisEd)")

enable_testing()
option(BUILD_TESTING "Build Clone1 Testing" OFF)
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
