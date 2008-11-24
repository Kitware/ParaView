INCLUDE(${QT_USE_FILE})

IF(NOT VTK_USE_N_WAY_ARRAYS)
  MESSAGE(SEND_ERROR "TableToSparseArrayPanel plugin requires VTK_USE_N_WAY_ARRAYS")
ENDIF(NOT VTK_USE_N_WAY_ARRAYS)

QT4_WRAP_CPP(MOC_SRCS TableToSparseArrayPanel.h)

SET(UI_FORMS
  TableToSparseArrayPanel.ui
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
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

ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME TableToSparseArrayPanel
  XML_NAME TableToSparseArray
  XML_GROUP filters
  )

ADD_PARAVIEW_PLUGIN(
  TableToSparseArrayPanelPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SOURCES ${MOC_SRCS} ${UI_BUILT_SOURCES} ${IFACE_SRCS} TableToSparseArrayPanel.cxx
  )

TARGET_LINK_LIBRARIES(
  TableToSparseArrayPanelPlugin
  OverViewCore
  )

