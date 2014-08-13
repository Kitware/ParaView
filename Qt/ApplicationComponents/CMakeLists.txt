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
  pqCatalystSetBreakpointReaction.cxx
  pqCatalystSetBreakpointReaction.h
  pqCatalystRemoveBreakpointReaction.cxx
  pqCatalystRemoveBreakpointReaction.h
  pqCategoryToolbarsBehavior.cxx
  pqCategoryToolbarsBehavior.h
  pqChangePipelineInputReaction.cxx
  pqChangePipelineInputReaction.h
  pqChartSelectionReaction.cxx
  pqChartSelectionReaction.h
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
  pqCTHArraySelectionDecorator.cxx
  pqCTHArraySelectionDecorator.h
  pqCubeAxesPropertyWidget.cxx
  pqCubeAxesPropertyWidget.h
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
  pqDoubleRangeSliderPropertyWidget.cxx
  pqDoubleRangeSliderPropertyWidget.h
  pqEditCameraReaction.cxx
  pqEditCameraReaction.h
  pqEditColorMapReaction.cxx
  pqEditColorMapReaction.h
  pqBoolPropertyWidgetDecorator.cxx
  pqBoolPropertyWidgetDecorator.h
  pqEnableWidgetDecorator.cxx
  pqEnableWidgetDecorator.h
  pqShowWidgetDecorator.cxx
  pqShowWidgetDecorator.h
  pqExportReaction.cxx
  pqExportReaction.h
  pqFiltersMenuReaction.cxx
  pqFiltersMenuReaction.h
  pqFixPathsInStateFilesBehavior.cxx
  pqFixPathsInStateFilesBehavior.h
  pqFontPropertyWidget.cxx
  pqFontPropertyWidget.h
  pqGenericPropertyWidgetDecorator.cxx
  pqGenericPropertyWidgetDecorator.h
  pqGlyphScaleFactorPropertyWidget.cxx
  pqGlyphScaleFactorPropertyWidget.h
  pqHelpReaction.cxx
  pqHelpReaction.h
  pqIgnoreSourceTimeReaction.cxx
  pqIgnoreSourceTimeReaction.h
  pqImageCompressorWidget.cxx
  pqImageCompressorWidget.h
  pqInputDataTypeDecorator.cxx
  pqInputDataTypeDecorator.h
  pqLightsEditor.cxx
  pqLightsEditor.h
  pqListPropertyWidget.cxx
  pqListPropertyWidget.h
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
  pqMasterOnlyReaction.cxx
  pqMasterOnlyReaction.h
  pqObjectPickingBehavior.cxx
  pqObjectPickingBehavior.h
  pqOpacityTableModel.cxx
  pqOpacityTableModel.h
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
  pqPluginDocumentationBehavior.cxx
  pqPluginDocumentationBehavior.h
  pqPluginSettingsBehavior.cxx
  pqPluginSettingsBehavior.h
  pqPropertyGroupButton.cxx
  pqPropertyGroupButton.h
  pqProxyGroupMenuManager.cxx
  pqProxyGroupMenuManager.h
  pqPVAnimationWidget.cxx
  pqPVAnimationWidget.h
  pqPVApplicationCore.cxx
  pqPVApplicationCore.h
  pqPythonShellReaction.cxx
  pqPythonShellReaction.h
  pqQtMessageHandlerBehavior.cxx
  pqQtMessageHandlerBehavior.h
  pqReaction.cxx
  pqReaction.h
  pqRenderViewSelectionReaction.cxx
  pqRenderViewSelectionReaction.h
  pqRepresentationToolbar.cxx
  pqRepresentationToolbar.h
  pqRescaleCustomScalarRangeReaction.cxx
  pqRescaleCustomScalarRangeReaction.h
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
  pqSeriesEditorPropertyWidget.cxx
  pqSeriesEditorPropertyWidget.h
  pqServerConnectReaction.cxx
  pqServerConnectReaction.h
  pqServerDisconnectReaction.cxx
  pqServerDisconnectReaction.h
  pqSourcesMenuReaction.cxx
  pqSourcesMenuReaction.h
  pqSpreadSheetVisibilityBehavior.cxx
  pqSpreadSheetVisibilityBehavior.h
  pqStandardPropertyWidgetInterface.cxx
  pqStandardPropertyWidgetInterface.h
  pqStandardViewFrameActionsImplementation.cxx
  pqStandardViewFrameActionsImplementation.h
  pqStatusBar.cxx
  pqStatusBar.h
  pqTestingReaction.cxx
  pqTestingReaction.h
  pqTextLocationWidget.cxx
  pqTextLocationWidget.h
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
  pqViewStreamingBehavior.cxx
  pqViewStreamingBehavior.h
  pqViewTypePropertyWidget.cxx
  pqViewTypePropertyWidget.h
)

set (Module_MOC_HDRS
  pqAboutDialogReaction.h
  pqAlwaysConnectedBehavior.h
  pqAnimationTimeToolbar.h
  pqApplicationSettingsReaction.h
  pqApplyBehavior.h
  pqAutoApplyReaction.h
  pqAutoLoadPluginXMLBehavior.h
  pqAxesToolbar.h
  pqBackgroundEditorWidget.h
  pqCameraLinkReaction.h
  pqCameraManipulatorWidget.h
  pqCameraReaction.h
  pqCameraToolbar.h
  pqCameraUndoRedoReaction.h
  pqCatalystConnectReaction.h
  pqCatalystContinueReaction.h
  pqCatalystPauseSimulationReaction.h
  pqCatalystSetBreakpointReaction.h
  pqCatalystRemoveBreakpointReaction.h
  pqCategoryToolbarsBehavior.h
  pqChangePipelineInputReaction.h
  pqChartSelectionReaction.h
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
  pqCTHArraySelectionDecorator.h
  pqCubeAxesPropertyWidget.h
  pqDataQueryReaction.h
  pqDataTimeStepBehavior.h
  pqDefaultMainWindow.h
  pqDefaultViewBehavior.h
  pqDeleteReaction.h
  pqDoubleRangeSliderPropertyWidget.h
  pqEditCameraReaction.h
  pqEditColorMapReaction.h
  pqBoolPropertyWidgetDecorator.h
  pqEnableWidgetDecorator.h
  pqShowWidgetDecorator.h
  pqExportReaction.h
  pqFiltersMenuReaction.h
  pqFixPathsInStateFilesBehavior.h
  pqFontPropertyWidget.h
  pqGenericPropertyWidgetDecorator.h
  pqGlyphScaleFactorPropertyWidget.h
  pqHelpReaction.h
  pqIgnoreSourceTimeReaction.h
  pqImageCompressorWidget.h
  pqInputDataTypeDecorator.h
  pqLightsEditor.h
  pqListPropertyWidget.h
  pqLoadDataReaction.h
  pqLoadStateReaction.h
  pqMainControlsToolbar.h
  pqManageCustomFiltersReaction.h
  pqManageLinksReaction.h
  pqManagePluginsReaction.h
  pqMasterOnlyReaction.h
  pqObjectPickingBehavior.h
  pqOpacityTableModel.h
  pqParaViewBehaviors.h
  pqPersistentMainWindowStateBehavior.h
  pqPipelineContextMenuBehavior.h
  pqPluginActionGroupBehavior.h
  pqPluginDockWidgetsBehavior.h
  pqPluginDocumentationBehavior.h
  pqPluginSettingsBehavior.h
  pqPropertyGroupButton.h
  pqProxyGroupMenuManager.h
  pqPVAnimationWidget.h
  pqPVApplicationCore.h
  pqPythonShellReaction.h
  pqQtMessageHandlerBehavior.h
  pqReaction.h
  pqRenderViewSelectionReaction.h
  pqRepresentationToolbar.h
  pqRescaleCustomScalarRangeReaction.h
  pqResetScalarRangeReaction.h
  pqSaveAnimationGeometryReaction.h
  pqSaveAnimationReaction.h
  pqSaveDataReaction.h
  pqSaveScreenshotReaction.h
  pqSaveStateReaction.h
  pqScalarBarVisibilityReaction.h
  pqSeriesEditorPropertyWidget.h
  pqServerConnectReaction.h
  pqServerDisconnectReaction.h
  pqSourcesMenuReaction.h
  pqSpreadSheetVisibilityBehavior.h
  pqStandardPropertyWidgetInterface.h
  pqStandardViewFrameActionsImplementation.h
  pqStatusBar.h
  pqTestingReaction.h
  pqTextLocationWidget.h
  pqTimerLogReaction.h
  pqToggleInteractionViewMode.h
  pqTransferFunctionWidgetPropertyWidget.h
  pqUndoRedoBehavior.h
  pqUndoRedoReaction.h
  pqVCRToolbar.h
  pqVerifyRequiredPluginBehavior.h
  pqViewMenuManager.h
  pqViewStreamingBehavior.h
  pqViewTypePropertyWidget.h
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
  Resources/UI/pqAxesToolbar.ui
  Resources/UI/pqBackgroundEditorWidget.ui
  Resources/UI/pqCameraManipulatorWidget.ui
  Resources/UI/pqCameraToolbar.ui
  Resources/UI/pqColorAnnotationsPropertyWidget.ui
  Resources/UI/pqColorEditorPropertyWidget.ui
  Resources/UI/pqColorMapEditor.ui
  Resources/UI/pqColorOpacityEditorWidget.ui
  Resources/UI/pqColorToolbar.ui
  Resources/UI/pqDefaultMainWindow.ui
  Resources/UI/pqDoubleRangeSliderPropertyWidget.ui
  Resources/UI/pqEditMenuBuilder.ui
  Resources/UI/pqFileMenuBuilder.ui
  Resources/UI/pqFontPropertyWidget.ui
  Resources/UI/pqImageCompressorWidget.ui
  Resources/UI/pqLightsEditor.ui
  Resources/UI/pqMainControlsToolbar.ui
  Resources/UI/pqPipelineBrowserContextMenu.ui
  Resources/UI/pqSeriesEditorPropertyWidget.ui
  Resources/UI/pqTextLocationWidget.ui
  Resources/UI/pqTransferFunctionWidgetPropertyWidgetDialog.ui
  Resources/UI/pqVCRToolbar.ui
)

set (Module_RESOURCE_FILES
)

# not sure this is the best way, but "moc" is complaining about
# interfaces defined under Components and this seems like the only way to fix
# that issue.
if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  qt5_wrap_cpp(__Module_MOC_SRCS ${Module_MOC_HDRS}
    OPTIONS -I ${CMAKE_CURRENT_SOURCE_DIR}/../Components)
else ()
  qt4_wrap_cpp(__Module_MOC_SRCS ${Module_MOC_HDRS}
    OPTIONS -I ${CMAKE_CURRENT_SOURCE_DIR}/../Components)
endif()


if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  qt5_wrap_ui(__Module_UI_SRCS ${Module_UI_FILES})
  qt5_add_resources(__Module_RESOURCE_SRCS ${Module_RESOURCE_FILES})
else ()
  qt4_wrap_ui(__Module_UI_SRCS ${Module_UI_FILES})
  qt4_add_resources(__Module_RESOURCE_SRCS ${Module_RESOURCE_FILES})
endif()

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
vtk_module_library(pqApplicationComponents
  ${Module_SRCS}
  ${__Module_RESOURCE_SRCS}
  ${__Module_MOC_SRCS}
  ${__Module_UI_SRCS})

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
#ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)
