cmake_minimum_required(VERSION 3.8)
project(SimpleParaView)

find_package(ParaView REQUIRED)

if (NOT PARAVIEW_USE_QT)
  message(STATUS
    "Skipping example: ${CMAKE_PROJECT_NAME} example requires PARAVIEW_USE_QT "
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
  myMainWindow.cxx
  myMainWindow.h)
set(ui_files
  myMainWindow.ui)
set(xml_files
  ParaViewFilters.xml)

paraview_client_add(
  NAME simple_paraview
  VERSION "1.0.0"
  SOURCES ${sources} ${ui_files}
  APPLICATION_XMLS ${xml_files}
  MAIN_WINDOW_CLASS myMainWindow
  ORGANIZATION "Kitware SAS."
  TITLE "Simple ParaView")
