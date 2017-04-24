#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
set (Module_SRCS
  pqAboutDialogReaction.cxx
  pqAboutDialogReaction.h
  pqAlwaysConnectedBehavior.cxx
  pqAlwaysConnectedBehavior.h
  pqAnimationTimeToolbar.cxx
  pqAnimationTimeToolbar.h
  pqAnimationTimeWidget.cxx
  pqAnimationTimeWidget.h
  pqApplicationComponentsInit.cxx
  pqApplicationComponentsInit.h
  pqApplicationSettingsReaction.cxx
  pqApplicationSettingsReaction.h
  pqApplyBehavior.cxx
  pqApplyBehavior.h
  pqAutoApplyReaction.cxx
  pqAutoApplyReaction.h
  pqAutoLoadPluginXMLBehavior.cxx
  pqAutoLoadPluginXMLBehavior.h
  pqAxesToolbar.cxx
  pqAxesToolbar.h
  pqBackgroundEditorWidget.cxx
  pqBackgroundEditorWidget.h
  pqBoolPropertyWidgetDecorator.cxx
  pqBoolPropertyWidgetDecorator.h
  pqBoxPropertyWidget.cxx
  pqBoxPropertyWidget.h
  pqCTHArraySelectionDecorator.cxx
  pqCTHArraySelectionDecorator.h
  pqCameraLinkReaction.cxx
  pqCameraLinkReaction.h
  pqCameraManipulatorWidget.cxx
  pqCameraManipulatorWidget.h
  pqCameraReaction.cxx
  pqCameraReaction.h
  pqCameraToolbar.cxx
  pqCameraToolbar.h
  pqCameraUndoRedoReaction.cxx
  pqCameraUndoRedoReaction.h
  pqCatalystConnectReaction.cxx
  pqCatalystConnectReaction.h
  pqCatalystContinueReaction.h
  pqCatalystPauseSimulationReaction.cxx
  pqCatalystPauseSimulationReaction.h
  pqCatalystRemoveBreakpointReaction.cxx
  pqCatalystRemoveBreakpointReaction.h
  pqCatalystSetBreakpointReaction.cxx
  pqCatalystSetBreakpointReaction.h
  pqCategoryToolbarsBehavior.cxx
  pqCategoryToolbarsBehavior.h
  pqChangePipelineInputReaction.cxx
  pqChangePipelineInputReaction.h
  pqChartSelectionReaction.cxx
  pqChartSelectionReaction.h
  pqChooseColorPresetReaction.cxx
  pqChooseColorPresetReaction.h
  pqCollaborationBehavior.cxx
  pqCollaborationBehavior.h
  pqColorAnnotationsPropertyWidget.cxx
  pqColorAnnotationsPropertyWidget.h
  pqColorEditorPropertyWidget.cxx
  pqColorEditorPropertyWidget.h
  pqColorMapEditor.cxx
  pqColorMapEditor.h
  pqColorOpacityEditorWidget.cxx
  pqColorOpacityEditorWidget.h
  pqColorPaletteSelectorWidget.cxx
  pqColorPaletteSelectorWidget.h
  pqColorSelectorPropertyWidget.cxx
  pqColorSelectorPropertyWidget.h
  pqColorTableModel.cxx
  pqColorTableModel.h
  pqColorToolbar.cxx
  pqColorToolbar.h
  pqCommandButtonPropertyWidget.cxx
  pqCommandButtonPropertyWidget.h
  pqCommandLineOptionsBehavior.cxx
  pqCommandLineOptionsBehavior.h
  pqCopyReaction.cxx
  pqCopyReaction.h
  pqCrashRecoveryBehavior.cxx
  pqCrashRecoveryBehavior.h
  pqCreateCustomFilterReaction.cxx
  pqCreateCustomFilterReaction.h
  pqCylinderPropertyWidget.cxx
  pqCylinderPropertyWidget.h
  pqDataQueryReaction.cxx
  pqDataQueryReaction.h
  pqDataTimeStepBehavior.cxx
  pqDataTimeStepBehavior.h
  pqDefaultMainWindow.cxx
  pqDefaultMainWindow.h
  pqDefaultViewBehavior.cxx
  pqDefaultViewBehavior.h
  pqDeleteReaction.cxx
  pqDeleteReaction.h
  pqDesktopServicesReaction.cxx
  pqDesktopServicesReaction.h
  pqDoubleRangeSliderPropertyWidget.cxx
  pqDoubleRangeSliderPropertyWidget.h
  pqEditCameraReaction.cxx
  pqEditCameraReaction.h
  pqEditColorMapReaction.cxx
  pqEditColorMapReaction.h
  pqEditScalarBarReaction.cxx
  pqEditScalarBarReaction.h
  pqEnableWidgetDecorator.cxx
  pqEnableWidgetDecorator.h
  pqExampleVisualizationsDialog.cxx
  pqExampleVisualizationsDialog.h
  pqExampleVisualizationsDialogReaction.cxx
  pqExampleVisualizationsDialogReaction.h
  pqExportReaction.cxx
  pqExportReaction.h
  pqFileNamePropertyWidget.cxx
  pqFileNamePropertyWidget.h
  pqFiltersMenuReaction.cxx
  pqFiltersMenuReaction.h
  pqFontPropertyWidget.cxx
  pqFontPropertyWidget.h
  pqGenericPropertyWidgetDecorator.cxx
  pqGenericPropertyWidgetDecorator.h
  pqGlyphScaleFactorPropertyWidget.cxx
  pqGlyphScaleFactorPropertyWidget.h
  pqHandlePropertyWidget.cxx
  pqHandlePropertyWidget.h
  pqHideAllReaction.cxx
  pqHideAllReaction.h
  pqIgnoreSourceTimeReaction.cxx
  pqIgnoreSourceTimeReaction.h
  pqImageCompressorWidget.cxx
  pqImageCompressorWidget.h
  pqImplicitPlanePropertyWidget.cxx
  pqImplicitPlanePropertyWidget.h
  pqImportCinemaReaction.cxx
  pqImportCinemaReaction.h
  pqInputDataTypeDecorator.cxx
  pqInputDataTypeDecorator.h
  pqInteractivePropertyWidget.cxx
  pqInteractivePropertyWidget.h
  pqLightsEditor.cxx
  pqLightsEditor.h
  pqLinePropertyWidget.cxx
  pqLinePropertyWidget.h
  pqLinkSelectionReaction.cxx
  pqLinkSelectionReaction.h
  pqListPropertyWidget.cxx
  pqListPropertyWidget.h
  pqLoadDataReaction.cxx
  pqLoadDataReaction.h
  pqLoadPaletteReaction.cxx
  pqLoadPaletteReaction.h
  pqLoadRestoreWindowLayoutReaction.cxx
  pqLoadRestoreWindowLayoutReaction.h
  pqLoadStateReaction.cxx
  pqLoadStateReaction.h
  pqLockPanelsBehavior.cxx
  pqLockPanelsBehavior.h
  pqLockPanelsReaction.cxx
  pqLockPanelsReaction.h
  pqMainControlsToolbar.cxx
  pqMainControlsToolbar.h
  pqManageCustomFiltersReaction.cxx
  pqManageCustomFiltersReaction.h
  pqManageLinksReaction.cxx
  pqManageLinksReaction.h
  pqManagePluginsReaction.cxx
  pqManagePluginsReaction.h
  pqMasterOnlyReaction.cxx
  pqMasterOnlyReaction.h
  pqModelTransformSupportBehavior.cxx
  pqModelTransformSupportBehavior.h
  pqObjectPickingBehavior.cxx
  pqObjectPickingBehavior.h
  pqOpacityTableModel.cxx
  pqOpacityTableModel.h
  pqOpenVRHidingDecorator.cxx
  pqOpenVRHidingDecorator.h
  pqOSPRayHidingDecorator.cxx
  pqOSPRayHidingDecorator.h
  pqPVAnimationWidget.cxx
  pqPVAnimationWidget.h
  pqPVApplicationCore.cxx
  pqPVApplicationCore.h
  pqParaViewBehaviors.cxx
  pqParaViewBehaviors.h
  pqParaViewMenuBuilders.cxx
  pqParaViewMenuBuilders.h
  pqPersistentMainWindowStateBehavior.cxx
  pqPersistentMainWindowStateBehavior.h
  pqPipelineContextMenuBehavior.cxx
  pqPipelineContextMenuBehavior.h
  pqPluginActionGroupBehavior.cxx
  pqPluginActionGroupBehavior.h
  pqPluginDockWidgetsBehavior.cxx
  pqPluginDockWidgetsBehavior.h
  pqPluginSettingsBehavior.cxx
  pqPluginSettingsBehavior.h
  pqPointPickingHelper.cxx
  pqPointPickingHelper.h
  pqPropertyGroupButton.cxx
  pqPropertyGroupButton.h
  pqProxyGroupMenuManager.cxx
  pqProxyGroupMenuManager.h
  pqPythonShellReaction.cxx
  pqPythonShellReaction.h
  pqQtMessageHandlerBehavior.cxx
  pqQtMessageHandlerBehavior.h
  pqReaction.cxx
  pqReaction.h
  pqReloadFilesReaction.cxx
  pqReloadFilesReaction.h
  pqRenameProxyReaction.cxx
  pqRenameProxyReaction.h
  pqRenderViewSelectionReaction.cxx
  pqRenderViewSelectionReaction.h
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
  pqSelectionReaction.cxx
  pqSelectionReaction.h
  pqSeriesEditorPropertyWidget.cxx
  pqSeriesEditorPropertyWidget.h
  pqServerConnectReaction.cxx
  pqServerConnectReaction.h
  pqServerDisconnectReaction.cxx
  pqServerDisconnectReaction.h
  pqShowWidgetDecorator.cxx
  pqShowWidgetDecorator.h
  pqSourcesMenuReaction.cxx
  pqSourcesMenuReaction.h
  pqSpherePropertyWidget.cxx
  pqSpherePropertyWidget.h
  pqSplinePropertyWidget.cxx
  pqSplinePropertyWidget.h
  pqSpreadSheetViewDecorator.cxx
  pqSpreadSheetViewDecorator.h
  pqSpreadSheetVisibilityBehavior.cxx
  pqSpreadSheetVisibilityBehavior.h
  pqStandardPropertyWidgetInterface.cxx
  pqStandardPropertyWidgetInterface.h
  pqStandardRecentlyUsedResourceLoaderImplementation.cxx
  pqStandardRecentlyUsedResourceLoaderImplementation.h
  pqStandardViewFrameActionsImplementation.cxx
  pqStandardViewFrameActionsImplementation.h
  pqStatusBar.cxx
  pqStatusBar.h
  pqStreamingTestingEventPlayer.cxx
  pqStreamingTestingEventPlayer.h
  pqTestingReaction.cxx
  pqTestingReaction.h
  pqTextLocationWidget.cxx
  pqTextLocationWidget.h
  pqTimeInspectorWidget.cxx
  pqTimeInspectorWidget.h
  pqTimerLogReaction.cxx
  pqTimerLogReaction.h
  pqToggleInteractionViewMode.cxx
  pqToggleInteractionViewMode.h
  pqTransferFunctionWidgetPropertyWidget.cxx
  pqTransferFunctionWidgetPropertyWidget.h
  pqUndoRedoBehavior.cxx
  pqUndoRedoBehavior.h
  pqUndoRedoReaction.cxx
  pqUndoRedoReaction.h
  pqVCRToolbar.cxx
  pqVCRToolbar.h
  pqVerifyRequiredPluginBehavior.cxx
  pqVerifyRequiredPluginBehavior.h
  pqViewMenuManager.cxx
  pqViewMenuManager.h
  pqViewResolutionPropertyWidget.cxx
  pqViewResolutionPropertyWidget.h
  pqViewStreamingBehavior.cxx
  pqViewStreamingBehavior.h
  pqViewTypePropertyWidget.cxx
  pqViewTypePropertyWidget.h
  pqWelcomeDialog.cxx
  pqWelcomeDialog.h
  pqYoungsMaterialPropertyWidget.cxx
  pqYoungsMaterialPropertyWidget.h
)

set (Module_MOC_HDRS
  pqAboutDialogReaction.h
  pqAlwaysConnectedBehavior.h
  pqAnimationTimeToolbar.h
  pqAnimationTimeWidget.h
  pqApplicationSettingsReaction.h
  pqApplyBehavior.h
  pqAutoApplyReaction.h
  pqAutoLoadPluginXMLBehavior.h
  pqAxesToolbar.h
  pqBackgroundEditorWidget.h
  pqBoolPropertyWidgetDecorator.h
  pqBoxPropertyWidget.h
  pqCTHArraySelectionDecorator.h
  pqCameraLinkReaction.h
  pqCameraManipulatorWidget.h
  pqCameraReaction.h
  pqCameraToolbar.h
  pqCameraUndoRedoReaction.h
  pqCatalystConnectReaction.h
  pqCatalystContinueReaction.h
  pqCatalystPauseSimulationReaction.h
  pqCatalystRemoveBreakpointReaction.h
  pqCatalystSetBreakpointReaction.h
  pqCategoryToolbarsBehavior.h
  pqChangePipelineInputReaction.h
  pqChartSelectionReaction.h
  pqChooseColorPresetReaction.h
  pqCollaborationBehavior.h
  pqColorAnnotationsPropertyWidget.h
  pqColorEditorPropertyWidget.h
  pqColorMapEditor.h
  pqColorOpacityEditorWidget.h
  pqColorPaletteSelectorWidget.h
  pqColorSelectorPropertyWidget.h
  pqColorTableModel.h
  pqColorToolbar.h
  pqCommandButtonPropertyWidget.h
  pqCommandLineOptionsBehavior.h
  pqCopyReaction.h
  pqCrashRecoveryBehavior.h
  pqCreateCustomFilterReaction.h
  pqCylinderPropertyWidget.h
  pqDataQueryReaction.h
  pqDataTimeStepBehavior.h
  pqDefaultMainWindow.h
  pqDefaultViewBehavior.h
  pqDeleteReaction.h
  pqDesktopServicesReaction.h
  pqDoubleRangeSliderPropertyWidget.h
  pqEditCameraReaction.h
  pqEditColorMapReaction.h
  pqEditScalarBarReaction.h
  pqEnableWidgetDecorator.h
  pqExampleVisualizationsDialog.h
  pqExampleVisualizationsDialogReaction.h
  pqExportReaction.h
  pqFileNamePropertyWidget.h
  pqFiltersMenuReaction.h
  pqFontPropertyWidget.h
  pqGenericPropertyWidgetDecorator.h
  pqGlyphScaleFactorPropertyWidget.h
  pqHandlePropertyWidget.h
  pqHideAllReaction.h
  pqIgnoreSourceTimeReaction.h
  pqImageCompressorWidget.h
  pqImplicitPlanePropertyWidget.h
  pqImportCinemaReaction.h
  pqInputDataTypeDecorator.h
  pqInteractivePropertyWidget.h
  pqLightsEditor.h
  pqLinePropertyWidget.h
  pqLinkSelectionReaction.h
  pqListPropertyWidget.h
  pqLoadDataReaction.h
  pqLoadPaletteReaction.h
  pqLoadRestoreWindowLayoutReaction.h
  pqLoadStateReaction.h
  pqLockPanelsBehavior.h
  pqLockPanelsReaction.h
  pqMainControlsToolbar.h
  pqManageCustomFiltersReaction.h
  pqManageLinksReaction.h
  pqManagePluginsReaction.h
  pqMasterOnlyReaction.h
  pqModelTransformSupportBehavior.h
  pqObjectPickingBehavior.h
  pqOpacityTableModel.h
  pqOpenVRHidingDecorator.h
  pqOSPRayHidingDecorator.h
  pqPVAnimationWidget.h
  pqPVApplicationCore.h
  pqParaViewBehaviors.h
  pqPersistentMainWindowStateBehavior.h
  pqPipelineContextMenuBehavior.h
  pqPluginActionGroupBehavior.h
  pqPluginDockWidgetsBehavior.h
  pqPluginSettingsBehavior.h
  pqPointPickingHelper.h
  pqPropertyGroupButton.h
  pqProxyGroupMenuManager.h
  pqPythonShellReaction.h
  pqQtMessageHandlerBehavior.h
  pqReaction.h
  pqReloadFilesReaction.h
  pqRenameProxyReaction.h
  pqRenderViewSelectionReaction.h
  pqRepresentationToolbar.h
  pqResetScalarRangeReaction.h
  pqSaveAnimationGeometryReaction.h
  pqSaveAnimationReaction.h
  pqSaveDataReaction.h
  pqSaveScreenshotReaction.h
  pqSaveStateReaction.h
  pqScalarBarVisibilityReaction.h
  pqSelectionReaction.h
  pqSeriesEditorPropertyWidget.h
  pqServerConnectReaction.h
  pqServerDisconnectReaction.h
  pqShowWidgetDecorator.h
  pqSourcesMenuReaction.h
  pqSpherePropertyWidget.h
  pqSplinePropertyWidget.h
  pqSpreadSheetViewDecorator.h
  pqSpreadSheetVisibilityBehavior.h
  pqStandardPropertyWidgetInterface.h
  pqStandardRecentlyUsedResourceLoaderImplementation.h
  pqStandardViewFrameActionsImplementation.h
  pqStatusBar.h
  pqStreamingTestingEventPlayer.h
  pqTestingReaction.h
  pqTextLocationWidget.h
  pqTimeInspectorWidget.h
  pqTimerLogReaction.h
  pqToggleInteractionViewMode.h
  pqTransferFunctionWidgetPropertyWidget.h
  pqUndoRedoBehavior.h
  pqUndoRedoReaction.h
  pqVCRToolbar.h
  pqVerifyRequiredPluginBehavior.h
  pqViewMenuManager.h
  pqViewResolutionPropertyWidget.h
  pqViewStreamingBehavior.h
  pqViewTypePropertyWidget.h
  pqWelcomeDialog.h
  pqYoungsMaterialPropertyWidget.h
)

if (PARAVIEW_ENABLE_PYTHON)
  list (APPEND Module_MOC_HDRS
    pqMacroReaction.h
    pqTraceReaction.h
  )

  list (APPEND Module_SRCS
    pqMacroReaction.cxx
    pqMacroReaction.h
    pqTraceReaction.cxx
    pqTraceReaction.h
  )
endif()

set (Module_UI_FILES
  Resources/UI/pqAnimationTimeWidget.ui
  Resources/UI/pqAxesToolbar.ui
  Resources/UI/pqBackgroundEditorWidget.ui
  Resources/UI/pqBoxPropertyWidget.ui
  Resources/UI/pqCameraManipulatorWidget.ui
  Resources/UI/pqCameraToolbar.ui
  Resources/UI/pqColorAnnotationsPropertyWidget.ui
  Resources/UI/pqColorEditorPropertyWidget.ui
  Resources/UI/pqColorMapEditor.ui
  Resources/UI/pqColorOpacityEditorWidget.ui
  Resources/UI/pqColorToolbar.ui
  Resources/UI/pqCylinderPropertyWidget.ui
  Resources/UI/pqDefaultMainWindow.ui
  Resources/UI/pqDoubleRangeSliderPropertyWidget.ui
  Resources/UI/pqEditMenuBuilder.ui
  Resources/UI/pqExampleVisualizationsDialog.ui
  Resources/UI/pqFileMenuBuilder.ui
  Resources/UI/pqFontPropertyWidget.ui
  Resources/UI/pqHandlePropertyWidget.ui
  Resources/UI/pqImageCompressorWidget.ui
  Resources/UI/pqImplicitPlanePropertyWidget.ui
  Resources/UI/pqLightsEditor.ui
  Resources/UI/pqLinePropertyWidget.ui
  Resources/UI/pqMainControlsToolbar.ui
  Resources/UI/pqPipelineBrowserContextMenu.ui
  Resources/UI/pqResetScalarRangeToDataOverTime.ui
  Resources/UI/pqSavePresetOptions.ui
  Resources/UI/pqSeriesEditorPropertyWidget.ui
  Resources/UI/pqSpherePropertyWidget.ui
  Resources/UI/pqSplinePropertyWidget.ui
  Resources/UI/pqSpreadSheetViewDecorator.ui
  Resources/UI/pqTextLocationWidget.ui
  Resources/UI/pqTimeInspectorWidget.ui
  Resources/UI/pqTransferFunctionWidgetPropertyWidgetDialog.ui
  Resources/UI/pqViewResolutionPropertyWidget.ui
  Resources/UI/pqVCRToolbar.ui
  Resources/UI/pqWelcomeDialog.ui
  Resources/UI/pqYoungsMaterialPropertyWidget.ui
)

set (Module_RESOURCE_FILES
  Resources/pqApplicationComponents.qrc
)

include(ParaViewQt)
if (PARAVIEW_USE_QTHELP)
  pv_find_package_qt(qt_private_targets REQUIRED QUIET
    QT4_COMPONENTS QtHelp
    QT5_COMPONENTS Widgets Help)
  list(APPEND Module_SRCS
    pqHelpReaction.cxx
    pqHelpReaction.h
    pqPluginDocumentationBehavior.cxx
    pqPluginDocumentationBehavior.h)
  list(APPEND Module_MOC_HDRS
    pqHelpReaction.h
    pqPluginDocumentationBehavior.h)
  set(${vtk-module}_EXPORT_CODE_INSTALL
    "include(\${ParaView_CMAKE_DIR}/ParaViewQt.cmake)
    pv_find_package_qt(__${vtk-module}_qt_targets REQUIRED QUIET
      QT4_COMPONENTS QtHelp
      QT5_COMPONENTS Widgets Help)")
else()
  pv_find_package_qt(qt_private_targets REQUIRED QUIET
    QT5_COMPONENTS Widgets)
  set(${vtk-module}_EXPORT_CODE_INSTALL
    "include(\${ParaView_CMAKE_DIR}/ParaViewQt.cmake)
    pv_find_package_qt(__${vtk-module}_qt_targets REQUIRED QUIET
      QT5_COMPONENTS Widgets)")
endif()
set(${vtk-module}_EXPORT_CODE_BUILD "${${vtk-module}_EXPORT_CODE_INSTALL}")

# not sure this is the best way, but "moc" is complaining about
# interfaces defined under Components and this seems like the only way to fix
# that issue.
pv_qt_wrap_cpp(__Module_MOC_SRCS ${Module_MOC_HDRS}
  OPTIONS -I ${CMAKE_CURRENT_SOURCE_DIR}/../Components -I ${CMAKE_CURRENT_SOURCE_DIR}/../Core)
pv_qt_wrap_ui(__Module_UI_SRCS ${Module_UI_FILES})
pv_qt_add_resources(__Module_RESOURCE_SRCS ${Module_RESOURCE_FILES})

SOURCE_GROUP("Resources" FILES
  ${Module_UI_FILES}
  ${Module_RESOURCE_FILES}
)

SOURCE_GROUP("Generated" FILES
  ${__Module_RESOURCE_SRCS}
  ${__Module_MOC_SRCS}
  ${__Module_UI_SRCS}
)

set(${vtk-module}_NO_HeaderTest 1)
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  add_definitions(-DVTKGL2)
endif()
vtk_module_library(pqApplicationComponents
  ${Module_SRCS}
  ${__Module_RESOURCE_SRCS}
  ${__Module_MOC_SRCS}
  ${__Module_UI_SRCS})
target_link_libraries(pqApplicationComponents
  LINK_PRIVATE ${qt_private_targets})

#IF (NOT PV_INSTALL_NO_DEVELOPMENT)
#  # Headers
#  GLOB_INSTALL_DEVELOPMENT(
#      ${CMAKE_CURRENT_SOURCE_DIR}
#      ${PV_INSTALL_INCLUDE_DIR}
#      "*.h;*.hxx;*.txx")
#  INSTALL(
#      FILES ${dynamicHeaders}
#      DESTINATION ${PV_INSTALL_INCLUDE_DIR}
#      COMPONENT Development)
#ENDIF ()
