cmake_minimum_required(VERSION 3.8)
project(SpreadSheet)

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
  pqSpreadsheetLoadDataReaction.cxx
  pqSpreadsheetLoadDataReaction.h
  SpreadSheetMainWindow.cxx
  SpreadSheetMainWindow.h)
set(ui_files
  SpreadSheetMainWindow.ui)

# The main cmake macro to create a paraview based application
paraview_client_add(
  NAME spreadsheet_paraview
  VERSION "1.0.1"
  SOURCES ${sources} ${ui_files}
  MAIN_WINDOW_CLASS SpreadSheetMainWindow
  ORGANIZATION "Kitware SAS."
  TITLE "Spreadsheet ParaView")

target_link_libraries(spreadsheet_paraview
  PRIVATE
    ParaView::RemotingSettings)
