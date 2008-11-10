PROJECT (QtWidgets)

INCLUDE_DIRECTORIES(
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QT_QTDESIGNER_INCLUDE_DIR}
)

## Add the source (*.cxx,*.h) files here.
SET(QtWidgets_SRCS
  pqAnimationKeyFrame.cxx
  pqAnimationKeyFrame.h
  pqAnimationModel.cxx
  pqAnimationModel.h
  pqAnimationTrack.cxx
  pqAnimationTrack.h
  pqAnimationWidget.cxx
  pqAnimationWidget.h
  pqChartPrintSave.cxx
  pqChartPrintSave.h
  pqCheckableHeaderModel.cxx
  pqCheckableHeaderModel.h
  pqCheckableHeaderView.cxx
  pqCheckableHeaderView.h
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
  pqDoubleRangeWidget.cxx
  pqDoubleRangeWidget.h
  pqDoubleSpinBox.cxx
  pqDoubleSpinBox.h
  pqFlatTreeView.cxx
  pqFlatTreeView.h
  pqIntRangeWidget.cxx
  pqIntRangeWidget.h
  pqLineEdit.cxx
  pqLineEdit.h
  pqListWidgetCheckHelper.cxx
  pqListWidgetCheckHelper.h
  pqListWidgetItemObject.cxx
  pqListWidgetItemObject.h
  pqLookmarkToolbar.cxx
  pqLookmarkToolbar.h
  pqProgressBar.cxx
  pqProgressBar.h
  pqProgressWidget.cxx
  pqProgressWidget.h
  pqQuickLaunchDialog.cxx
  pqQuickLaunchDialog.h
  pqSectionVisibilityContextMenu.cxx
  pqSectionVisibilityContextMenu.h
  pqSetData.cxx
  pqSetData.h
  pqSetName.cxx
  pqSetName.h
  pqSignalAdaptors.cxx
  pqSignalAdaptors.h
  pqSpinBox.cxx
  pqSpinBox.h
  pqTreeView.cxx
  pqTreeView.h
  pqTreeWidget.cxx
  pqTreeWidget.h
  pqTreeWidgetCheckHelper.cxx
  pqTreeWidgetCheckHelper.h
  pqTreeWidgetItem.cxx
  pqTreeWidgetItem.h
  pqTreeWidgetItemObject.cxx
  pqTreeWidgetItemObject.h
  pqTreeWidgetSelectionHelper.cxx
  pqTreeWidgetSelectionHelper.h
  pqTreeViewSelectionHelper.cxx
  pqTreeViewSelectionHelper.h
  pqViewMenu.cxx
  pqViewMenu.h
  pqWaitCursor.cxx
  pqWaitCursor.h
  QtWidgetsExport.h
  QtWidgetsPlugin.cxx
  QtWidgetsPlugin.h
)

##########################################################################
## Add the header files that need to moc processed.

QT4_WRAP_CPP(QtWidgets_MOC_SOURCES
  QtWidgetsPlugin.h

  pqAnimationKeyFrame.h
  pqAnimationModel.h
  pqAnimationTrack.h
  pqAnimationWidget.h
  pqChartPrintSave.h
  pqCheckableHeaderModel.h
  pqCheckableHeaderView.h
  pqCollapsedGroup.h
  pqColorChooserButton.h
  pqColorTableModel.h
  pqConsoleWidget.h
  pqDelimitedTextParser.h
  pqDoubleRangeWidget.h
  pqDoubleSpinBox.h
  pqFlatTreeView.h
  pqIntRangeWidget.h
  pqLineEdit.h
  pqListWidgetCheckHelper.h
  pqListWidgetItemObject.h
  pqLookmarkToolbar.h
  pqProgressBar.h
  pqProgressWidget.h
  pqQuickLaunchDialog.h
  pqSectionVisibilityContextMenu.h
  pqSignalAdaptors.h
  pqSpinBox.h
  pqTreeView.h
  pqTreeWidget.h
  pqTreeWidgetCheckHelper.h
  pqTreeWidgetItemObject.h
  pqTreeWidgetSelectionHelper.h
  pqTreeViewSelectionHelper.h
  pqViewMenu.h
)

##########################################################################
## Add *.ui files to be uic processed.
SET(QtWidgets_UI_FORMS
  Resources/UI/pqQuickLaunchDialog.ui
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
ADD_LIBRARY(pqWidgets ${QT_WIDGETS_ALL_SRCS})


## Widgets does not depend on VTK/ParaView. Only Qt.
TARGET_LINK_LIBRARIES(pqWidgets
  ${QT_LIBRARIES}
)

SET(QTWIDGETS_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/QtWidgetsExport.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/QtWidgetsExport.h")

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

INSTALL(TARGETS pqWidgets
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)

