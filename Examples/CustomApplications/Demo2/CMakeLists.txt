cmake_minimum_required(VERSION 3.8)
project(Demo2)

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

find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTOUIC 1)

set(sources
  myMainWindow.cxx
  myMainWindow.h
  myMainWindow.ui)

add_executable(DemoApp2 DemoApp2.cxx ${sources})
target_link_libraries(DemoApp2
  PRIVATE
    ParaView::pqApplicationComponents
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")

configure_file(ParaViewFilters.xml ${CMAKE_BINARY_DIR} COPYONLY)
