PROJECT (QtWidgets)

INCLUDE_DIRECTORIES(
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QT_QTDESIGNER_INCLUDE_DIR}
)

IF(NOT BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DQT_STATICPLUGIN)
ENDIF(NOT BUILD_SHARED_LIBS)

## Add the source (*.cxx,*.h) files here.
SET(QtWidgets_SRCS
  QtWidgetsPlugin.cxx
  QtWidgetsPlugin.h

  pqChartContextMenu.cxx
  pqChartContextMenu.h
  pqCollapsedGroup.cxx
  pqCollapsedGroup.h
  pqColorChooserButton.cxx
  pqColorChooserButton.h
  pqColorTableDelegate.cxx
  pqColorTableDelegate.h
  pqColorTableModel.cxx
  pqColorTableModel.h
  pqConnect.cxx
  pqConnect.h
  pqConsoleWidget.cxx
  pqConsoleWidget.h
  pqDelimitedTextParser.cxx
  pqDelimitedTextParser.h
  pqFlatTreeView.cxx
  pqFlatTreeView.h
  pqListWidgetItemObject.cxx
  pqListWidgetItemObject.h
  pqLookmarkToolbar.cxx
  pqLookmarkToolbar.h
  pqProgressBar.cxx
  pqProgressBar.h
  pqProgressWidget.cxx
  pqProgressWidget.h
  pqSectionVisibilityContextMenu.cxx
  pqSectionVisibilityContextMenu.h
  pqSetData.cxx
  pqSetData.h
  pqSetName.cxx
  pqSetName.h
  pqSignalAdaptors.cxx
  pqSignalAdaptors.h
  pqListWidgetCheckHelper.cxx
  pqListWidgetCheckHelper.h
  pqSelectionTreeWidget.cxx
  pqSelectionTreeWidget.h
  pqTreeWidgetCheckHelper.cxx
  pqTreeWidgetCheckHelper.h
  pqTreeWidgetItemObject.cxx
  pqTreeWidgetItemObject.h
  pqViewMenu.cxx
  pqViewMenu.h
  pqWaitCursor.cxx
  pqWaitCursor.h
  QtWidgetsExport.h
)

##########################################################################
## Add the header files that need to moc processed.

QT4_WRAP_CPP(QtWidgets_MOC_SOURCES
  QtWidgetsPlugin.h

  pqChartContextMenu.h
  pqCollapsedGroup.h
  pqColorChooserButton.h
  pqColorTableModel.h
  pqConsoleWidget.h
  pqDelimitedTextParser.h
  pqFlatTreeView.h
  pqListWidgetItemObject.h
  pqLookmarkToolbar.h
  pqProgressBar.h
  pqProgressWidget.h
  pqSectionVisibilityContextMenu.h
  pqSignalAdaptors.h
  pqListWidgetCheckHelper.h
  pqSelectionTreeWidget.h
  pqTreeWidgetCheckHelper.h
  pqTreeWidgetItemObject.h
  pqViewMenu.h
)

##########################################################################
## Add *.ui files to be uic processed.
SET(QtWidgets_UI_FORMS
)

QT4_WRAP_UI(QtWidgets_UI_BUILT_SOURCES
  ${QtWidgets_UI_FORMS}
)

##########################################################################
## Add resources.
SET(UI_RESOURCES
  Resources/QtWidgets.qrc
)

QT4_ADD_RESOURCES(RCS_SOURCES
  ${UI_RESOURCES}
)


SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${QtWidgets_UI_FORMS}
)

SOURCE_GROUP("Generated" FILES
  ${QtWidgets_MOC_SOURCES}
  ${RCS_SOURCES}
  ${QtWidgets_UI_BUILT_SOURCES}
)

SET(QT_WIDGETS_ALL_SRCS ${QtWidgets_SRCS} ${QtWidgets_MOC_SOURCES}
  ${RCS_SOURCES} ${QtWidgets_UI_BUILT_SOURCES})

##########################################################################
ADD_LIBRARY(ParaViewQtWidgets ${QT_WIDGETS_ALL_SRCS})


## Widgets does not depend on VTK/ParaView. Only Qt.
TARGET_LINK_LIBRARIES(ParaViewQtWidgets
  ${QT_LIBRARIES}
)

INSTALL(TARGETS ParaViewQtWidgets
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)

