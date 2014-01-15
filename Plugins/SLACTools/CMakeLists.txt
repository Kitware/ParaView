# This plugin contains tools developed for the Stanford Linear Accelerator
# Center to make it easier to visualize finite element simulation results.

SET(PLUGIN_NAME SLACTools)
SET(PLUGIN_VERSION "1.1")

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIRS}
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

  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    SET (Qt5_FIND_COMPONENTS Widgets)
    INCLUDE (ParaViewQt5)
    QT5_WRAP_CPP(MOC_SRCS ${MOC_HEADERS})
    QT5_WRAP_UI(UI_SRCS ${UI_FILES})
  ELSE ()
    INCLUDE (${QT_USE_FILE})
    QT4_WRAP_CPP(MOC_SRCS ${MOC_HEADERS})
    QT4_WRAP_UI(UI_SRCS ${UI_FILES})
  ENDIF ()

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
