INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(
  MOC_SRCS 
  ClientGraphViewFrameActions.h
  )

SET(UI_RESOURCES
  ClientGraphViewFrame.qrc
  )

QT4_ADD_RESOURCES(
  RCS_SOURCES
  ${UI_RESOURCES}
  )

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  )

SOURCE_GROUP("Generated" FILES
  ${RCS_SOURCES}
  )

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${OverViewCore_BINARY_DIR}
  ${OverViewCore_SOURCE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_BINARY_DIR}/VTK/GUISupport/Qt
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${QtWidgets_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${VTK_INCLUDE_DIR}
  ${pqComponents_BINARY_DIR}
  ${pqComponents_SOURCE_DIR}
  ${pqCore_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  )

ADD_PARAVIEW_VIEW_FRAME_ACTION_GROUP(
  TOOLBARIFACE TOOLBAR_IFACE_SRCS 
  CLASS_NAME ClientGraphViewFrameActions
  )

ADD_PARAVIEW_PLUGIN(
  ClientGraphViewFramePlugin "1.0" 
  GUI_INTERFACES ${TOOLBARIFACE}
  GUI_SOURCES ClientGraphViewFrameActions.cxx ${RCS_SOURCES} ${MOC_SRCS} ${TOOLBAR_IFACE_SRCS}
  )

TARGET_LINK_LIBRARIES(
  ClientGraphViewFramePlugin
  pqComponents
  pqCore
  )

