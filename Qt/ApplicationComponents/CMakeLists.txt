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
set(classes
  pqAboutDialogReaction
  pqAddToFavoritesReaction
  pqAlwaysConnectedBehavior
  pqAnimationShortcutDecorator
  pqAnimationShortcutWidget
  pqAnimationTimeToolbar
  pqAnnotationsModel
  pqApplicationComponentsInit
  pqApplicationSettingsReaction
  pqApplyBehavior
  pqApplyPropertiesReaction
  pqAutoApplyReaction
  pqAutoLoadPluginXMLBehavior
  pqAxesToolbar
  pqBackgroundEditorWidget
  pqBoolPropertyWidgetDecorator
  pqBoxPropertyWidget
  pqCTHArraySelectionDecorator
  pqCameraLinkReaction
  pqCameraManipulatorWidget
  pqCameraReaction
  pqCameraToolbar
  pqCameraUndoRedoReaction
  pqCatalystConnectReaction
  pqCatalystExportReaction
  pqCatalystPauseSimulationReaction
  pqCatalystRemoveBreakpointReaction
  pqCatalystSetBreakpointReaction
  pqCategoryToolbarsBehavior
  pqChangePipelineInputReaction
  pqChartSelectionReaction
  pqChooseColorPresetReaction
  pqCollaborationBehavior
  pqColorAnnotationsPropertyWidget
  pqColorAnnotationsWidget
  pqColorEditorPropertyWidget
  pqColorMapEditor
  pqColorOpacityEditorWidget
  pqColorPaletteSelectorWidget
  pqColorSelectorPropertyWidget
  pqColorTableModel
  pqColorToolbar
  pqCommandButtonPropertyWidget
  pqCommandLineOptionsBehavior
  pqCopyReaction
  pqCrashRecoveryBehavior
  pqCreateCustomFilterReaction
  pqCustomShortcutBehavior
  pqCustomizeShortcutsReaction
  pqCustomizeShortcutsDialog
  pqCheckableProperty
  pqCustomViewpointsToolbar
  pqCylinderPropertyWidget
  pqDataAssemblyPropertyWidget
  pqDataQueryReaction
  pqDataTimeStepBehavior
  pqDefaultContextMenu
  pqDefaultMainWindow
  pqDefaultViewBehavior
  pqDeleteReaction
  pqDesktopServicesReaction
  pqDoubleRangeSliderPropertyWidget
  pqEditCameraReaction
  pqEditColorMapReaction
  pqEditScalarBarReaction
  pqEnableWidgetDecorator
  pqExampleVisualizationsDialog
  pqExampleVisualizationsDialogReaction
  pqExportReaction
  pqExtractorsMenuReaction
  pqFileNamePropertyWidget
  pqFiltersMenuReaction
  pqFindDataWidget
  pqFontPropertyWidget
  pqGenericPropertyWidgetDecorator
  pqGlyphScaleFactorPropertyWidget
  pqHandlePropertyWidget
  pqIgnoreSourceTimeReaction
  pqImageCompressorWidget
  pqImplicitPlanePropertyWidget
  pqInputDataTypeDecorator
  pqInteractivePropertyWidget
  pqLightPropertyWidget
  pqLinePropertyWidget
  pqLinkSelectionReaction
  pqListPropertyWidget
  pqLiveSourceBehavior
  pqLoadDataReaction
  pqLoadMaterialsReaction
  pqLoadPaletteReaction
  pqLoadRestoreWindowLayoutReaction
  pqLoadStateReaction
  pqLockPanelsBehavior
  pqLockPanelsReaction
  pqLogViewerReaction
  pqMainControlsToolbar
  pqMainWindowEventBehavior
  pqManageCustomFiltersReaction
  pqManageFavoritesReaction
  pqManageLinksReaction
  pqManagePluginsReaction
  pqMasterOnlyReaction
  pqMenuReactionUtils
  pqModelTransformSupportBehavior
  pqMoleculePropertyWidget
  pqMultiComponentsDecorator
  pqObjectPickingBehavior
  pqOMETransferFunctionsPropertyWidget
  pqOpacityTableModel
  pqOSPRayHidingDecorator
  pqPVAnimationWidget
  pqPVApplicationCore
  pqParaViewBehaviors
  pqParaViewMenuBuilders
  pqPauseLiveSourcePropertyWidget
  pqPersistentMainWindowStateBehavior
  pqPipelineContextMenuBehavior
  pqPluginActionGroupBehavior
  pqPluginDockWidgetsBehavior
  pqPluginSettingsBehavior
  pqPluginToolBarBehavior
  pqPointPickingHelper
  pqPreviewMenuManager
  pqPropertyCollectionWidget
  pqProxyGroupMenuManager
  pqReaction
  pqReloadFilesReaction
  pqRenameProxyReaction
  pqRenderViewSelectionReaction
  pqRepresentationToolbar
  pqResetScalarRangeReaction
  pqResetDefaultSettingsReaction
  pqSaveAnimationGeometryReaction
  pqAnimatedExportReaction
  pqSaveAnimationReaction
  pqSaveDataReaction
  pqSaveExtractsReaction
  pqSaveScreenshotReaction
  pqSaveStateReaction
  pqScalarBarVisibilityReaction
  pqSearchItemReaction
  pqSelectionQueryPropertyWidget
  pqSelectionReaction
  pqSeriesEditorPropertyWidget
  pqServerConnectReaction
  pqServerDisconnectReaction
  pqSessionTypeDecorator
  pqSetMainWindowTitleReaction
  pqShowHideAllReaction
  pqShowWidgetDecorator
  pqSourcesMenuReaction
  pqSpherePropertyWidget
  pqSplinePropertyWidget
  pqSpreadSheetViewDecorator
  pqSpreadSheetVisibilityBehavior
  pqStandardPropertyWidgetInterface
  pqStandardRecentlyUsedResourceLoaderImplementation
  pqStandardViewFrameActionsImplementation
  pqStatusBar
  pqStreamingTestingEventPlayer
  pqTemporalExportReaction
  pqTestingReaction
  pqTextLocationWidget
  pqTimeInspectorWidget
  pqTimerLogReaction
  pqToggleInteractionViewMode
  pqTransferFunctionWidgetPropertyDialog
  pqTransferFunctionWidgetPropertyWidget
  pqUndoRedoBehavior
  pqUndoRedoReaction
  pqUsageLoggingBehavior
  pqUseSeparateColorMapReaction
  pqVCRToolbar
  pqVerifyRequiredPluginBehavior
  pqViewMenuManager
  pqViewResolutionPropertyWidget
  pqViewStreamingBehavior
  pqViewTypePropertyWidget
  pqWelcomeDialog
  pqYoungsMaterialPropertyWidget)

set(built_shared 0)
if (BUILD_SHARED_LIBS)
  set(built_shared 1)
endif ()
set_property(SOURCE pqApplicationComponentsInit.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "BUILD_SHARED_LIBS=${built_shared}")

set(headers
  pqCatalystContinueReaction.h
  )

if (TARGET ParaView::pqPython)
  list(APPEND classes
    pqMacroReaction
    pqTraceReaction
  )
  list(APPEND headers
    pqPythonScriptEditorReaction.h
  )
endif ()

set(ui_files
  Resources/UI/pqAxesToolbar.ui
  Resources/UI/pqBackgroundEditorWidget.ui
  Resources/UI/pqBoxPropertyWidget.ui
  Resources/UI/pqCameraManipulatorWidget.ui
  Resources/UI/pqCameraToolbar.ui
  Resources/UI/pqColorAnnotationsWidget.ui
  Resources/UI/pqColorEditorPropertyWidget.ui
  Resources/UI/pqColorMapEditor.ui
  Resources/UI/pqColorOpacityEditorWidget.ui
  Resources/UI/pqColorToolbar.ui
  Resources/UI/pqCustomResolutionDialog.ui
  Resources/UI/pqCylinderPropertyWidget.ui
  Resources/UI/pqCustomizeShortcutsDialog.ui
  Resources/UI/pqDataAssemblyPropertyWidget.ui
  Resources/UI/pqDefaultMainWindow.ui
  Resources/UI/pqDoubleRangeSliderPropertyWidget.ui
  Resources/UI/pqEditMenuBuilder.ui
  Resources/UI/pqEmptyView.ui
  Resources/UI/pqExampleVisualizationsDialog.ui
  Resources/UI/pqFileMenuBuilder.ui
  Resources/UI/pqFindDataWidget.ui
  Resources/UI/pqFontPropertyWidget.ui
  Resources/UI/pqHandlePropertyWidget.ui
  Resources/UI/pqImageCompressorWidget.ui
  Resources/UI/pqImplicitPlanePropertyWidget.ui
  Resources/UI/pqLightPropertyWidget.ui
  Resources/UI/pqLinePropertyWidget.ui
  Resources/UI/pqMainControlsToolbar.ui
  Resources/UI/pqMoleculePropertyWidget.ui
  Resources/UI/pqOMETransferFunctionsPropertyWidget.ui
  Resources/UI/pqOMETransferFunctionsPropertyWidgetPage.ui
  Resources/UI/pqPropertyCollectionWidget.ui
  Resources/UI/pqResetScalarRangeToDataOverTime.ui
  Resources/UI/pqSavePresetOptions.ui
  Resources/UI/pqSeriesEditorPropertyWidget.ui
  Resources/UI/pqSpherePropertyWidget.ui
  Resources/UI/pqSplinePropertyWidget.ui
  Resources/UI/pqSpreadSheetViewDecorator.ui
  Resources/UI/pqTextLocationWidget.ui
  Resources/UI/pqTimeInspectorWidget.ui
  Resources/UI/pqTransferFunctionWidgetPropertyDialog.ui
  Resources/UI/pqVCRToolbar.ui
  Resources/UI/pqViewResolutionPropertyWidget.ui
  Resources/UI/pqWelcomeDialog.ui
  Resources/UI/pqYoungsMaterialPropertyWidget.ui)

set(resource_files
  Resources/pqApplicationComponents.qrc)

set(qt_components
  Network
  Widgets
  Svg)

if (PARAVIEW_USE_QTHELP)
  list(APPEND qt_components
    Help)

  # TODO: Split these into a separate module so that there isn't a public
  # optional dependency.
  list(APPEND classes
    pqHelpReaction
    pqPluginDocumentationBehavior)
endif ()

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  ${qt_components})

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

vtk_module_add_module(ParaView::pqApplicationComponents
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${ui_files}
          ${resource_files})
vtk_module_link(ParaView::pqApplicationComponents
  PRIVATE
    Qt5::Widgets
    Qt5::Svg
    Qt5::Network)
if (PARAVIEW_USE_QTHELP)
  vtk_module_link(ParaView::pqApplicationComponents
    PRIVATE
      Qt5::Help)
endif ()
