# This plugin contains tools developed for the Stanford Linear Accelerator
# Center to make it easier to visualize finite element simulation results.

SET(PLUGIN_NAME SLACTools)
SET(PLUGIN_VERSION "1.1")

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIR}
  )

SET(SM_XML
  SLACToolsSM.xml
  )

SET(SM_SRC
  vtkPTemporalRanges.cxx
  vtkSLACPlaneGlyphs.cxx
  vtkSamplePlaneSource.cxx
  vtkTemporalRanges.cxx
  )

SET(UI_FILES
  pqSLACActionHolder.ui
  pqSLACDataLoadManager.ui
  )

SET(MOC_HEADERS
  pqSLACActionGroup.h
  pqSLACDataLoadManager.h
  pqSLACManager.h
  )

SET(CLIENT_SRCS
  pqSLACActionGroup.cxx
  pqSLACDataLoadManager.cxx
  pqSLACManager.cxx
  )

IF (PARAVIEW_BUILD_QT_GUI)

  INCLUDE(${QT_USE_FILE})

  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
    ${pqCore_SOURCE_DIR}
    ${pqCore_BINARY_DIR}
    ${pqComponents_SOURCE_DIR}
    ${pqComponents_BINARY_DIR}
    ${QtWidgets_SOURCE_DIR}
    ${QtWidgets_BINARY_DIR}
    )

  QT4_WRAP_CPP(MOC_SRCS ${MOC_HEADERS})
  QT4_WRAP_UI(UI_SRCS ${UI_FILES})

  ADD_PARAVIEW_ACTION_GROUP(SLAC_TOOLBAR_IFACE SLAC_TOOLBAR_IFACE_SRCS
    CLASS_NAME pqSLACActionGroup
    GROUP_NAME "ToolBar/SLAC"
    )

  ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
    SERVER_MANAGER_XML ${SM_XML}
    SERVER_MANAGER_SOURCES ${SM_SRC}
    GUI_INTERFACES ${SLAC_TOOLBAR_IFACE}
    GUI_RESOURCES SLACTools.qrc
    SOURCES ${MOC_SRCS} ${UI_SRCS} ${SLAC_TOOLBAR_IFACE_SRCS} ${CLIENT_SRCS}
    )

ELSE (PARAVIEW_BUILD_QT_GUI)
  ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
    SERVER_MANAGER_XML ${SM_XML}
    SERVER_MANAGER_SOURCES ${SM_SRC}
    )
ENDIF (PARAVIEW_BUILD_QT_GUI)
