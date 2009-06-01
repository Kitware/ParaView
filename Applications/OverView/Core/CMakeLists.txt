PROJECT(OverViewCore)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

IF(NOT BUILD_SHARED_LIBS)
  MESSAGE(SEND_ERROR "PARAVIEW_BUILD_OverView requires BUILD_SHARED_LIBS")
ENDIF(NOT BUILD_SHARED_LIBS)

# Set global configuration options ...
SET(OVERVIEW_VERSION_MAJOR 0)
SET(OVERVIEW_VERSION_MINOR 7)
SET(OVERVIEW_VERSION_PATCH 0)
SET(OVERVIEW_VERSION_TYPE "alpha")
SET(OVERVIEW_VERSION "${OVERVIEW_VERSION_MAJOR}.${OVERVIEW_VERSION_MINOR}.${OVERVIEW_VERSION_PATCH}")
SET(OVERVIEW_VERSION_FULL "${OVERVIEW_VERSION} ${OVERVIEW_VERSION_TYPE}")

FIND_PACKAGE(Qt4)
SET(QT_USE_QTASSISTANT TRUE)
INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/OverViewCoreExport.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/OverViewCoreExport.h
  )

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/Config.h
  )

QT4_WRAP_CPP(MOC_SRCS
  AboutDialog.h
  AnnotationLink.h
  AnnotationManagerPanel.h
  ApplicationOptionsDialog.h
  DisplayPolicy.h
  GlobalGraphViewOptions.h
  IconDialog.h
  MainWindow.h
  StopListDialog.h
  ToolbarActions.h
  pqAnnotationLayersModel.h
  pqMultiInputView.h
  pqSingleInputView.h
  pqStandardGraphLayoutStrategies.h
  pqStandardTreeLayoutStrategies.h
  )

SET(UI_FORMS
  AboutDialog.ui
  AnnotationManagerPanel.ui
  GlobalGraphViewOptions.ui
  IconDialog.ui
  MainWindow.ui
  StopListDialog.ui
  )

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
  )

FILE(GLOB HEADERS *.h)
FILE(GLOB SOURCES *.cxx)

# Setup vtkInstantiator registration for this library's classes.
SET(KIT OverViewCore)
INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
  vtkSMCheckableArrayListInformationHelper.cxx
  VTK_EXPORT
  "${CMAKE_CURRENT_BINARY_DIR}" "")

ADD_LIBRARY(
  OverViewCore
  ${HEADERS}
  ${SOURCES}
  ${MOC_SRCS}
  ${UI_BUILT_SOURCES}
  ${KitInstantiator_SRCS}
  )

TARGET_LINK_LIBRARIES(
  OverViewCore
  ${QT_LIBRARIES}
  QVTK
  pqComponents
  pqCore
  pqWidgets
  vtkPVServerManager
  )
 
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/OverViewConfig.cmake.in" "${ParaView_BINARY_DIR}/OverViewConfig.cmake" @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/OverViewOSXDistribution.cmake.in" "${ParaView_BINARY_DIR}/OverViewOSXDistribution.cmake" @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/OverViewWin32Distribution.cmake.in" "${ParaView_BINARY_DIR}/OverViewWin32Distribution.cmake" @ONLY)

