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
  pv_find_package_qt(qt_targets REQUIRED QUIET
    QT4_COMPONENTS QtGui
    QT5_COMPONENTS Widgets)
  pv_qt_wrap_cpp(MOC_SRCS ${MOC_HEADERS})
  pv_qt_wrap_ui(UI_SRCS ${UI_FILES})
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

ELSE ()
  ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
    SERVER_MANAGER_XML ${SM_XML}
    SERVER_MANAGER_SOURCES ${SM_SRC}
    )
ENDIF ()

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
