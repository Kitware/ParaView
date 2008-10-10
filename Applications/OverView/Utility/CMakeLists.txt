PROJECT(OverViewUtility)

FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_SRCS
  IconDialog.h
  ToolbarActions.h
  pqMultiInputView.h
  pqSingleInputView.h
  pqStandardGraphLayoutStrategies.h
  )

SET(UI_FORMS
  IconDialog.ui
  )

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
  )

FILE(GLOB HEADERS *.h)
FILE(GLOB SOURCES *.cxx)

# Setup vtkInstantiator registration for this library's classes.
SET(KIT ParaViewUtility)
INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
  vtkSMCheckableArrayListInformationHelper.cxx
  VTK_EXPORT
  "${CMAKE_CURRENT_BINARY_DIR}" "")

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

ADD_LIBRARY(
  OverViewUtility
  ${HEADERS}
  ${SOURCES}
  ${MOC_SRCS}
  ${UI_BUILT_SOURCES}
  ${KitInstantiator_SRCS}
  )

TARGET_LINK_LIBRARIES(
  OverViewUtility
  ${QT_LIBRARIES}
  QVTK
  pqComponents
  pqCore
  pqWidgets
  vtkPVServerManager
  )
 
