PROJECT (QtWidgets)

INCLUDE_DIRECTORIES(
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
)

## Add the source (*.cxx,*.h) files here.
SET(QtWidgets_SRCS
  pqChartContextMenu.cxx
  pqChartContextMenu.h
  pqCollapsedGroup.cxx
  pqCollapsedGroup.h
  pqColorChooserButton.cxx
  pqColorChooserButton.h
  pqConnect.cxx
  pqConnect.h
  pqConsoleWidget.cxx
  pqConsoleWidget.h
  pqDelimitedTextParser.cxx
  pqDelimitedTextParser.h
  pqFileDialog.cxx
  pqFileDialog.h
  pqFileDialogFilter.cxx
  pqFileDialogFilter.h
  pqFileDialogModel.cxx
  pqFileDialogModel.h
  pqFlatTreeView.cxx
  pqFlatTreeView.h
  pqListWidgetItemObject.cxx
  pqListWidgetItemObject.h
  pqLocalFileDialogModel.cxx
  pqLocalFileDialogModel.h
  pqPlayControlsWidget.cxx
  pqPlayControlsWidget.h
  pqProgressBar.cxx
  pqProgressBar.h
  pqSectionVisibilityContextMenu.cxx
  pqSectionVisibilityContextMenu.h
  pqSetData.cxx
  pqSetData.h
  pqSetName.cxx
  pqSetName.h
  pqSignalAdaptors.cxx
  pqSignalAdaptors.h
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
  pqChartContextMenu.h
  pqCollapsedGroup.h
  pqColorChooserButton.h
  pqConsoleWidget.h
  pqDelimitedTextParser.h
  pqFileDialog.h
  pqFileDialogFilter.h
  pqFileDialogModel.h
  pqFlatTreeView.h
  pqListWidgetItemObject.h
  pqLocalFileDialogModel.h
  pqPlayControlsWidget.h
  pqProgressBar.h
  pqSectionVisibilityContextMenu.h
  pqSignalAdaptors.h
  pqTreeWidgetItemObject.h
  pqViewMenu.h
)

##########################################################################
## Add *.ui files to be uic processed.
SET(QtWidgets_UI_FORMS
  pqFileDialog.ui
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


##########################################################################
ADD_LIBRARY(QtWidgets
  ${QtWidgets_SRCS}  
  ${QtWidgets_MOC_SOURCES}
  ${RCS_SOURCES}
  ${QtWidgets_UI_BUILT_SOURCES}
)


## Widgets does not depend on VTK/ParaView. Only Qt.
TARGET_LINK_LIBRARIES(QtWidgets
  ${QT_LIBRARIES}
)

INSTALL(TARGETS QtWidgets
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)

# -----------------------------------------------------------------------------
# plugin
# -----------------------------------------------------------------------------

SET(QtWidgetsPlugin_SRCS
  QtWidgetsPlugin.cxx
  QtWidgetsPlugin.h
)

QT4_WRAP_CPP(QtWidgetsPlugin_MOC_SRCS
  QtWidgetsPlugin.h
)

ADD_DEFINITIONS(-DQT_PLUGIN -DQT_NO_DEBUG)

ADD_LIBRARY(QtWidgetsPlugin
  SHARED
  ${QtWidgetsPlugin_SRCS}
  ${QtWidgetsPlugin_MOC_SRCS}
)

TARGET_LINK_LIBRARIES(QtWidgetsPlugin 
  QtWidgets
  ${QT_LIBRARIES}
)

INSTALL(TARGETS QtWidgetsPlugin
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
