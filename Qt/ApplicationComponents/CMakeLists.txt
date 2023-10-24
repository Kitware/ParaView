# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  pqAboutDialogReaction
  pqAddToFavoritesReaction
  pqAlwaysConnectedBehavior
  pqAnglePropertyWidget
  pqAnimationShortcutDecorator
  pqAnimationShortcutWidget
  pqAnimationTimeToolbar
  pqApplicationComponentsInit
  pqApplicationSettingsReaction
  pqApplyBehavior
  pqApplyPropertiesReaction
  pqAutoApplyReaction
  pqAutoLoadPluginXMLBehavior
  pqAxesToolbar
  pqBackgroundEditorWidget
  pqBlockContextMenu
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
  pqChangeFileNameReaction
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
  pqCoordinateFramePropertyWidget
  pqCustomViewpointsController
  pqCustomViewpointsDefaultController
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
  pqDisplaySizedImplicitPlanePropertyWidget
  pqDoubleRangeSliderPropertyWidget
  pqEditCameraReaction
  pqEditColorMapReaction
  pqEditScalarBarReaction
  pqEnableWidgetDecorator
  pqEqualizeLayoutReaction
  pqEqualizerPropertyWidget
  pqExampleVisualizationsDialog
  pqExampleVisualizationsDialogReaction
  pqExportReaction
  pqExtractorsMenuReaction
  pqFileListPropertyWidget
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
  pqInteractiveProperty2DWidget
  pqInteractivePropertyWidget
  pqInteractivePropertyWidgetAbstract
  pqLanguageChooserWidget
  pqLightPropertyWidget
  pqLightToolbar
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
  pqManageExpressionsReaction
  pqManageFavoritesReaction
  pqManageLinksReaction
  pqManagePluginsReaction
  pqMasterOnlyReaction
  pqMenuReactionUtils
  pqMetaDataPropertyWidget
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
  pqRescaleScalarRangeReaction
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
  pqSelectionEditor
  pqSelectionListPropertyWidget
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
  pqTimelineItemDelegate
  pqTimelineModel
  pqTimelinePainter
  pqTimelineView
  pqTimelineWidget
  pqTimeManagerWidget
  pqTimerLogReaction
  pqToggleInteractionViewMode
  pqTransferFunctionWidgetPropertyDialog
  pqTransferFunctionWidgetPropertyWidget
  pqUndoRedoBehavior
  pqUndoRedoReaction
  pqUsageLoggingBehavior
  pqUse2DTransferFunctionReaction
  pqUseSeparateColorMapReaction
  pqUseSeparateOpacityArrayReaction
  pqVCRToolbar
  pqVerifyRequiredPluginBehavior
  pqViewMenuManager
  pqViewResolutionPropertyWidget
  pqViewStreamingBehavior
  pqViewTypePropertyWidget
  pqWelcomeDialog
  pqYoungsMaterialPropertyWidget)

set_property(SOURCE pqApplicationComponentsInit.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "BUILD_SHARED_LIBS=$<BOOL:${BUILD_SHARED_LIBS}>")

set(headers
  pqCatalystContinueReaction.h
  )

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
  Resources/UI/pqFileListPropertyWidget.ui
  Resources/UI/pqFindDataWidget.ui
  Resources/UI/pqFontPropertyWidget.ui
  Resources/UI/pqHandlePropertyWidget.ui
  Resources/UI/pqImageCompressorWidget.ui
  Resources/UI/pqImplicitPlanePropertyWidget.ui
  Resources/UI/pqLightPropertyWidget.ui
  Resources/UI/pqLightToolbar.ui
  Resources/UI/pqLinePropertyWidget.ui
  Resources/UI/pqMainControlsToolbar.ui
  Resources/UI/pqMoleculePropertyWidget.ui
  Resources/UI/pqOMETransferFunctionsPropertyWidget.ui
  Resources/UI/pqOMETransferFunctionsPropertyWidgetPage.ui
  Resources/UI/pqPropertyCollectionWidget.ui
  Resources/UI/pqResetScalarRangeToDataOverTime.ui
  Resources/UI/pqSavePresetOptions.ui
  Resources/UI/pqSelectionEditor.ui
  Resources/UI/pqSeriesEditorPropertyWidget.ui
  Resources/UI/pqSpherePropertyWidget.ui
  Resources/UI/pqSplinePropertyWidget.ui
  Resources/UI/pqSpreadSheetViewDecorator.ui
  Resources/UI/pqTextLocationWidget.ui
  Resources/UI/pqTimeInspectorWidget.ui
  Resources/UI/pqTimeManagerWidget.ui
  Resources/UI/pqTransferFunctionWidgetPropertyDialog.ui
  Resources/UI/pqVCRToolbar.ui
  Resources/UI/pqViewResolutionPropertyWidget.ui
  Resources/UI/pqWelcomeDialog.ui
  Resources/UI/pqSelectionListPropertyWidget.ui
  Resources/UI/pqYoungsMaterialPropertyWidget.ui)

set(resource_files
  Resources/pqApplicationComponents.qrc)

set(qt_components
  Network
  Widgets
  Svg)

if(PARAVIEW_QT_MAJOR_VERSION GREATER "5")
  list(APPEND qt_components
    Core5Compat)
endif ()


if (TARGET ParaView::pqPython)
  list(APPEND classes
    pqEditMacrosDialog
    pqEditMacrosReaction
    pqMacroReaction
    pqTraceReaction
  )
  list(APPEND headers
    pqPythonScriptEditorReaction.h
  )
  list(APPEND ui_files
    Resources/UI/pqEditMacrosDialog.ui)
endif ()

if (paraview_use_materialeditor)
  list(APPEND classes
    pqMaterialAttributesDelegate
    pqMaterialEditor
    pqNewMaterialDialog)

  list(APPEND ui_files
    Resources/UI/pqMaterialEditor.ui
    Resources/UI/pqNewMaterialDialog.ui)
endif ()

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
  PACKAGE     "Qt${PARAVIEW_QT_MAJOR_VERSION}"
  COMPONENTS  ${qt_components})

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

set(translatable_files)
if (PARAVIEW_BUILD_TRANSLATIONS)
  list(APPEND translatable_files ${ui_files})
  foreach (el IN LISTS classes)
    list(APPEND translatable_files "${el}.cxx")
    list(APPEND translatable_files "${el}.h")
  endforeach ()

  paraview_create_translation(
    TARGET "ApplicationComponents.ts"
    FILES ${translatable_files}
    OUTPUT_TS "${PARAVIEW_TRANSLATIONS_DIRECTORY}/Qt_ApplicationComponents.ts")
  add_dependencies(localization "ApplicationComponents.ts")
endif ()
vtk_module_add_module(ParaView::pqApplicationComponents
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${ui_files}
          ${resource_files})
vtk_module_link(ParaView::pqApplicationComponents
  PRIVATE
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets"
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Svg"
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Network")
vtk_module_definitions(ParaView::pqApplicationComponents PRIVATE QT_NO_KEYWORDS)

if (PARAVIEW_QT_MAJOR_VERSION GREATER "5")
  vtk_module_link(ParaView::pqApplicationComponents
    PRIVATE
      "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core5Compat")
endif ()

if (PARAVIEW_USE_QTHELP)
  vtk_module_link(ParaView::pqApplicationComponents
    PRIVATE
      "Qt${PARAVIEW_QT_MAJOR_VERSION}::Help")
endif ()
