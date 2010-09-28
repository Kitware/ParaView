PROJECT(pqApplicationComponents)

INCLUDE_DIRECTORIES(
  ${pqApplicationComponents_SOURCE_DIR}
  ${pqApplicationComponents_BINARY_DIR}
  ${pqApplicationComponents_BINARY_DIR}/Resources/UI
  ${pqComponents_BINARY_DIR}
  ${pqComponents_SOURCE_DIR}
  ${pqComponents_BINARY_DIR}/Resources/UI
  ${pqCore_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${ParaView_BINARY_DIR}/VTK/GUISupport/Qt
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt/Chart
  ${ParaView_BINARY_DIR}/VTK/GUISupport/Qt/Chart
  ${QtTesting_SOURCE_DIR}
  ${QtTesting_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtPython_BINARY_DIR}
  ${QtPython_SOURCE_DIR})

# List headers to be processed by moc
SET (MOC_HEADERS
  pqAboutDialogReaction.h
  pqAlwaysConnectedBehavior.h
  pqAnimationTimeToolbar.h
  pqApplicationSettingsReaction.h
  pqAutoApplyReaction.h
  pqAutoLoadPluginXMLBehavior.h
  pqAxesToolbar.h
  pqCameraLinkReaction.h
  pqCameraReaction.h
  pqCameraToolbar.h
  pqCameraUndoRedoReaction.h
  pqCategoryToolbarsBehavior.h
  pqChangePipelineInputReaction.h
  pqColorToolbar.h
  pqCommandLineOptionsBehavior.h
  pqCrashRecoveryBehavior.h
  pqCreateCustomFilterReaction.h
  pqDataTimeStepBehavior.h
  pqDataQueryReaction.h
  pqDefaultMainWindow.h
  pqDefaultViewBehavior.h
  pqDeleteBehavior.h
  pqDeleteReaction.h
  pqEditCameraReaction.h
  pqEditColorMapReaction.h
  pqExportReaction.h
  pqFiltersMenuReaction.h
  pqFixPathsInStateFilesBehavior.h
  pqHelpReaction.h
  pqIgnoreSourceTimeReaction.h
  pqListNewProxyDefinitionsBehavior.h
  pqLoadDataReaction.h
  pqLoadStateReaction.h
  pqMainControlsToolbar.h
  pqManageCustomFiltersReaction.h
  pqManageLinksReaction.h
  pqManagePluginsReaction.h
  pqObjectPickingBehavior.h
  pqParaViewBehaviors.h
  pqPersistentMainWindowStateBehavior.h
  pqPluginActionGroupBehavior.h
  pqPluginDockWidgetsBehavior.h
  pqProxyGroupMenuManager.h
  pqPVAnimationWidget.h
  pqPVApplicationCore.h
  pqPVNewSourceBehavior.h
  pqPythonShellReaction.h
  pqQtMessageHandlerBehavior.h
  pqReaction.h
  pqRepresentationToolbar.h
  pqResetScalarRangeReaction.h
  pqSaveAnimationGeometryReaction.h
  pqSaveAnimationReaction.h
  pqSaveDataReaction.h
  pqSaveScreenshotReaction.h
  pqSaveStateReaction.h
  pqScalarBarVisibilityReaction.h
  pqSelectionInspectorWidget.h
  pqSelectionToolbar.h
  pqServerConnectReaction.h
  pqServerDisconnectReaction.h
  pqSourcesMenuReaction.h
  pqSpreadSheetVisibilityBehavior.h
  pqStandardViewFrameActionGroup.h
  pqStatusBar.h
  pqTestingReaction.h
  pqTimerLogReaction.h
  pqUndoRedoBehavior.h
  pqUndoRedoReaction.h
  pqVCRToolbar.h
  pqViewFrameActionsBehavior.h
  pqViewMenuManager.h
  pqViewSettingsManager.h
  pqViewSettingsReaction.h
)

# List UI files.
SET (UI_FILES
  Resources/UI/pqAxesToolbar.ui
  Resources/UI/pqCameraToolbar.ui
  Resources/UI/pqColorToolbar.ui
  Resources/UI/pqDefaultMainWindow.ui
  Resources/UI/pqEditMenuBuilder.ui
  Resources/UI/pqFileMenuBuilder.ui
  Resources/UI/pqMainControlsToolbar.ui
  Resources/UI/pqPipelineBrowserContextMenu.ui
  Resources/UI/pqSelectionToolbar.ui
  Resources/UI/pqVCRToolbar.ui
)

# List qrc files
SET (UI_RESOURCES
  Resources/pqApplicationComponents.qrc
)

# List all source files.
SET (SOURCE_FILES
  pqAboutDialogReaction.cxx
  pqAboutDialogReaction.h
  pqAlwaysConnectedBehavior.cxx
  pqAlwaysConnectedBehavior.h
  pqAnimationTimeToolbar.cxx
  pqAnimationTimeToolbar.h
  pqApplicationSettingsReaction.cxx
  pqApplicationSettingsReaction.h
  pqAutoApplyReaction.cxx
  pqAutoApplyReaction.h
  pqAutoLoadPluginXMLBehavior.cxx
  pqAutoLoadPluginXMLBehavior.h
  pqAxesToolbar.cxx
  pqAxesToolbar.h
  pqCameraLinkReaction.cxx
  pqCameraLinkReaction.h
  pqCameraReaction.cxx
  pqCameraReaction.h
  pqCameraToolbar.cxx
  pqCameraToolbar.h
  pqCameraUndoRedoReaction.cxx
  pqCameraUndoRedoReaction.h
  pqCategoryToolbarsBehavior.cxx
  pqCategoryToolbarsBehavior.h
  pqChangePipelineInputReaction.cxx
  pqChangePipelineInputReaction.h
  pqColorToolbar.cxx
  pqColorToolbar.h
  pqCommandLineOptionsBehavior.cxx
  pqCommandLineOptionsBehavior.h
  pqCrashRecoveryBehavior.cxx
  pqCrashRecoveryBehavior.h
  pqCreateCustomFilterReaction.cxx
  pqCreateCustomFilterReaction.h
  pqDataTimeStepBehavior.cxx
  pqDataTimeStepBehavior.h
  pqDataQueryReaction.h
  pqDataQueryReaction.cxx
  pqDefaultMainWindow.cxx
  pqDefaultMainWindow.h
  pqDefaultViewBehavior.cxx
  pqDefaultViewBehavior.h
  pqDeleteBehavior.cxx
  pqDeleteBehavior.h
  pqDeleteReaction.cxx
  pqDeleteReaction.h
  pqEditCameraReaction.cxx
  pqEditCameraReaction.h
  pqEditColorMapReaction.cxx
  pqEditColorMapReaction.h
  pqExportReaction.cxx
  pqExportReaction.h
  pqFiltersMenuReaction.cxx
  pqFiltersMenuReaction.h
  pqFixPathsInStateFilesBehavior.cxx
  pqFixPathsInStateFilesBehavior.h
  pqHelpReaction.cxx
  pqHelpReaction.h
  pqIgnoreSourceTimeReaction.cxx
  pqIgnoreSourceTimeReaction.h
  pqListNewProxyDefinitionsBehavior.cxx
  pqListNewProxyDefinitionsBehavior.h
  pqLoadDataReaction.cxx
  pqLoadDataReaction.h
  pqLoadStateReaction.cxx
  pqLoadStateReaction.h
  pqMainControlsToolbar.cxx
  pqMainControlsToolbar.h
  pqManageCustomFiltersReaction.cxx
  pqManageCustomFiltersReaction.h
  pqManageLinksReaction.cxx
  pqManageLinksReaction.h
  pqManagePluginsReaction.cxx
  pqManagePluginsReaction.h
  pqObjectPickingBehavior.cxx
  pqObjectPickingBehavior.h
  pqParaViewBehaviors.cxx
  pqParaViewBehaviors.h
  pqParaViewMenuBuilders.cxx
  pqParaViewMenuBuilders.h
  pqPersistentMainWindowStateBehavior.cxx
  pqPersistentMainWindowStateBehavior.h
  pqPluginActionGroupBehavior.cxx
  pqPluginActionGroupBehavior.h
  pqPluginDockWidgetsBehavior.cxx
  pqPluginDockWidgetsBehavior.h
  pqProxyGroupMenuManager.cxx
  pqProxyGroupMenuManager.h
  pqPVAnimationWidget.cxx
  pqPVAnimationWidget.h
  pqPVApplicationCore.cxx
  pqPVApplicationCore.h
  pqPVNewSourceBehavior.cxx
  pqPVNewSourceBehavior.h
  pqPythonShellReaction.cxx
  pqPythonShellReaction.h
  pqQtMessageHandlerBehavior.cxx
  pqQtMessageHandlerBehavior.h
  pqReaction.cxx
  pqReaction.h
  pqRepresentationToolbar.cxx
  pqRepresentationToolbar.h
  pqResetScalarRangeReaction.cxx
  pqResetScalarRangeReaction.h
  pqSaveAnimationGeometryReaction.cxx
  pqSaveAnimationGeometryReaction.h
  pqSaveAnimationReaction.cxx
  pqSaveAnimationReaction.h
  pqSaveDataReaction.cxx
  pqSaveDataReaction.h
  pqSaveScreenshotReaction.cxx
  pqSaveScreenshotReaction.h
  pqSaveStateReaction.cxx
  pqSaveStateReaction.h
  pqScalarBarVisibilityReaction.cxx
  pqScalarBarVisibilityReaction.h
  pqSelectionInspectorWidget.cxx
  pqSelectionInspectorWidget.h
  pqSelectionToolbar.cxx
  pqSelectionToolbar.h
  pqServerConnectReaction.cxx
  pqServerConnectReaction.h
  pqServerDisconnectReaction.cxx
  pqServerDisconnectReaction.h
  pqSourcesMenuReaction.cxx
  pqSourcesMenuReaction.h
  pqSpreadSheetVisibilityBehavior.cxx
  pqSpreadSheetVisibilityBehavior.h
  pqStandardViewFrameActionGroup.cxx
  pqStandardViewFrameActionGroup.h
  pqStatusBar.cxx
  pqStatusBar.h
  pqTestingReaction.cxx
  pqTestingReaction.h
  pqTimerLogReaction.cxx
  pqTimerLogReaction.h
  pqUndoRedoBehavior.cxx
  pqUndoRedoBehavior.h
  pqUndoRedoReaction.cxx
  pqUndoRedoReaction.h
  pqVCRToolbar.cxx
  pqVCRToolbar.h
  pqViewFrameActionsBehavior.cxx
  pqViewFrameActionsBehavior.h
  pqViewMenuManager.cxx
  pqViewMenuManager.h
  pqViewSettingsManager.cxx
  pqViewSettingsManager.h
  pqViewSettingsReaction.cxx
  pqViewSettingsReaction.h
)
#---------------------------------------------
IF(PARAVIEW_ENABLE_PYTHON)
  SET (MOC_HEADERS
    ${MOC_HEADERS}
    pqEditTraceReaction.h
    pqMacroReaction.h
    pqSaveTraceReaction.h
    pqTraceReaction.h
    )
  SET (SOURCE_FILES
    ${SOURCE_FILES}
    pqEditTraceReaction.cxx
    pqEditTraceReaction.h
    pqMacroReaction.cxx
    pqMacroReaction.h
    pqSaveTraceReaction.cxx
    pqSaveTraceReaction.h
    pqTraceReaction.cxx
    pqTraceReaction.h
    )
ENDIF(PARAVIEW_ENABLE_PYTHON)
#---------------------------------------------
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pqApplicationComponentsExport.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/pqApplicationComponentsExport.h")

SET(dynamicHeaders 
    "${dynamicHeaders};${CMAKE_CURRENT_BINARY_DIR}/pqApplicationComponentsExport.h")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  ${MOC_HEADERS}
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FILES}
)

QT4_ADD_RESOURCES(RCS_SOURCES
  ${UI_RESOURCES}
)

SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${UI_FILES}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
  ${CMAKE_CURRENT_BINARY_DIR}/pqApplicationComponentsExport.h
)

ADD_LIBRARY(pqApplicationComponents
  ${SOURCE_FILES}
  ${UI_BUILT_SOURCES}
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  )

TARGET_LINK_LIBRARIES(pqApplicationComponents
  pqComponents
  )

INSTALL(TARGETS pqApplicationComponents
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
  
IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  # Headers
  GLOB_INSTALL_DEVELOPMENT(
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${PV_INSTALL_INCLUDE_DIR} 
      "*.h;*.hxx;*.txx")
  INSTALL(
      FILES ${dynamicHeaders}
      DESTINATION ${PV_INSTALL_INCLUDE_DIR}
      COMPONENT Development)
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)


