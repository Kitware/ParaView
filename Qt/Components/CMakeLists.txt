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
  pqAboutDialog
  pqAbstractItemSelectionModel
  pqActionGroupInterface
  pqActiveObjects
  pqAnimatablePropertiesComboBox
  pqAnimatableProxyComboBox
  pqAnimationManager
  pqAnimationTimeWidget
  pqAnimationViewWidget
  pqArrayListDomain
  pqArraySelectionWidget
  pqArraySelectorPropertyWidget
  pqArrayStatusPropertyWidget
  pqCalculatorWidget
  pqCameraDialog
  pqCameraKeyFrameWidget
  pqChangeInputDialog
  pqCollaborationPanel
  pqColorChooserButtonWithPalettes
  pqComboBoxDomain
  pqCommandPropertyWidget
  pqComparativeCueWidget
  pqComparativeVisPanel
  pqComponentsInit
  pqComponentsTestUtility
  pqCompositeDataInformationTreeModel
  pqCompositePropertyWidgetDecorator
  pqCompositeTreePropertyWidget
  pqCustomFilterDefinitionModel
  pqCustomFilterDefinitionWizard
  pqCustomFilterManager
  pqCustomFilterManagerModel
  pqCustomViewpointButtonDialog
  pqDataInformationModel
  pqDataInformationWidget
  pqDialog
  pqDisplayColorWidget
  pqDisplayPanel
  pqDisplayPanelInterface
  pqDisplayPanelPropertyWidget
  pqDisplayRepresentationWidget
  pqDockWindowInterface
  pqDoubleVectorPropertyWidget
  pqFavoritesDialog
  pqFavoritesTreeWidget
  pqFileChooserWidget
  pqFindDataCreateSelectionFrame
  pqFindDataCurrentSelectionFrame
  pqFindDataDialog
  pqFindDataSelectionDisplayFrame
  pqHighlightItemDelegate
  pqImageTip
  pqIndexSelectionWidget
  pqInputSelectorWidget
  pqIntMaskPropertyWidget
  pqIntRangeWidget
  pqIntVectorPropertyWidget
  pqItemViewSearchWidget
  pqItemViewSearchWidgetEventPlayer
  pqKeyFrameEditor
  pqKeyFrameTimeValidator
  pqKeyFrameTypeWidget
  pqLabel
  pqLightsEditor
  pqLightsInspector
  pqLinksEditor
  pqLinksManager
  pqLiveInsituManager
  pqLiveInsituVisualizationManager
  pqLockViewSizeCustomDialog
  pqMemoryInspectorPanel
  pqMultiBlockInspectorWidget
  pqMultiViewWidget
  pqOrbitCreatorDialog
  pqOutputPortComboBox
  pqPipelineAnnotationFilterModel
  pqPipelineBrowserWidget
  pqPipelineInputComboBox
  pqPipelineModel
  pqPipelineModelSelectionAdaptor
  pqPipelineTimeKeyFrameEditor
  pqPluginDialog
  pqPluginTreeWidgetEventPlayer
  pqPluginTreeWidgetEventTranslator
  pqPopOutWidget
  pqPresetDialog
  pqPresetGroupsManager
  pqPresetToPixmap
  pqPropertiesPanel
  pqPropertyGroupWidget
  pqPropertyWidget
  pqPropertyWidgetDecorator
  pqPropertyWidgetInterface
  pqProxiesWidget
  pqProxyEditorPropertyWidget
  pqProxyInformationWidget
  pqProxyPropertyWidget
  pqProxySILModel
  pqProxySelectionWidget
  pqProxyWidget
  pqProxyWidgetDialog
  pqQueryClauseWidget
  pqRecentFilesMenu
  pqRemoteCommandDialog
  pqRemoteCommandTemplateDialog
  pqRescaleRange
  pqSILModel
  pqSILWidget
  pqSMSignalAdaptors
  pqSampleScalarAddRangeDialog
  pqSampleScalarWidget
  pqScalarSetModel
  pqScalarValueListPropertyWidget
  pqSearchBox
  pqSelectReaderDialog
  pqSelectionAdaptor
  pqSelectionInputWidget
  pqSelectionLinkDialog
  pqSelectionManager
  pqServerConfigurationImporter
  pqServerConnectDialog
  pqServerLauncher
  pqSetBreakpointDialog
  pqSettingsDialog
  pqShaderReplacementsComboBox
  pqShaderReplacementsSelectorPropertyWidget
  pqSignalAdaptorKeyFrameType
  pqSignalAdaptorSelectionTreeWidget
  pqSignalAdaptorTreeWidget
  pqStringVectorPropertyWidget
  pqSubsetInclusionLatticeTreeModel
  pqSubsetInclusionLatticeWidget
  pqTabbedMultiViewWidget
  pqTextureComboBox
  pqTextureSelectorPropertyWidget
  pqTimerLogDisplay
  pqToolBarInterface
  pqToolTipTrapper
  pqTransferFunctionWidget
  pqTriggerOnIdleHelper
  pqUndoStackBuilder
  pqVCRController
  pqViewFrame
  pqViewFrameActionsInterface
  pqViewManager
  pqWidgetRangeDomain)

set(headers
  pqPluginTreeWidget.h)

set(ui_files
  Resources/UI/pqAbortAnimation.ui
  Resources/UI/pqAboutDialog.ui
  Resources/UI/pqAnimationTimeWidget.ui
  Resources/UI/pqFavoritesDialog.ui
  Resources/UI/pqCalculatorWidget.ui
  Resources/UI/pqCameraDialog.ui
  Resources/UI/pqCameraKeyFrameWidget.ui
  Resources/UI/pqChangeInputDialog.ui
  Resources/UI/pqCollaborationPanel.ui
  Resources/UI/pqComparativeParameterRangeDialog.ui
  Resources/UI/pqComparativeVisPanel.ui
  Resources/UI/pqConnectIdDialog.ui
  Resources/UI/pqContourControls.ui
  Resources/UI/pqCustomFilterDefinitionWizard.ui
  Resources/UI/pqCustomFilterManager.ui
  Resources/UI/pqCustomViewpointButtonDialog.ui
  Resources/UI/pqDisplayRepresentationWidget.ui
  Resources/UI/pqFindDataCreateSelectionFrame.ui
  Resources/UI/pqFindDataCurrentSelectionFrame.ui
  Resources/UI/pqFindDataDialog.ui
  Resources/UI/pqFindDataSelectionDisplayFrame.ui
  Resources/UI/pqItemViewSearchWidget.ui
  Resources/UI/pqKeyFrameEditor.ui
  Resources/UI/pqKeyFrameTypeWidget.ui
  Resources/UI/pqLightsEditor.ui
  Resources/UI/pqLightsInspector.ui
  Resources/UI/pqLinksEditor.ui
  Resources/UI/pqLinksManager.ui
  Resources/UI/pqLockViewSizeCustomDialog.ui
  Resources/UI/pqMemoryInspectorPanelForm.ui
  Resources/UI/pqMultiBlockInspectorWidget.ui
  Resources/UI/pqOrbitCreatorDialog.ui
  Resources/UI/pqPipelineTimeKeyFrameEditor.ui
  Resources/UI/pqPluginDialog.ui
  Resources/UI/pqPopoutPlaceholder.ui
  Resources/UI/pqPresetDialog.ui
  Resources/UI/pqPropertiesPanel.ui
  Resources/UI/pqProxyInformationWidget.ui
  Resources/UI/pqProxySelectionWidget.ui
  Resources/UI/pqProxyWidgetDialog.ui
  Resources/UI/pqPythonAnimationCue.ui
  Resources/UI/pqQueryClauseWidget.ui
  Resources/UI/pqQueryCompositeTreeDialog.ui
  Resources/UI/pqRemoteCommandDialogForm.ui
  Resources/UI/pqRemoteCommandTemplateDialogForm.ui
  Resources/UI/pqRescaleRangeDialog.ui
  Resources/UI/pqSampleScalarAddRangeDialog.ui
  Resources/UI/pqSampleScalarWidget.ui
  Resources/UI/pqScalarValueListPropertyWidget.ui
  Resources/UI/pqSearchBox.ui
  Resources/UI/pqSelectReaderDialog.ui
  Resources/UI/pqSelectionInputWidget.ui
  Resources/UI/pqSelectionLinkDialog.ui
  Resources/UI/pqServerConnectDialog.ui
  Resources/UI/pqServerLauncherDialog.ui
  Resources/UI/pqSetBreakpointDialog.ui
  Resources/UI/pqSettingsDialog.ui
  Resources/UI/pqTimerLogDisplay.ui
  Resources/UI/pqViewFrame.ui)

set(sources)
set(private_headers
  pqSGExportStateWizardInternals.h)

if (TARGET pqPython)
  # code for generating specialized Python scripts
  # from the GUI using trace information
  list(APPEND classes
    pqCatalystExportStateWizard
    pqCinemaConfiguration
    pqCinemaTrack
    pqImageOutputInfo
    pqSGExportStateWizard
    pqSGPluginManager
    pqSGWritersMenuManager)
  list(APPEND sources
    pqCinemaTrackSelection.cxx
    pqExportViewSelection.cxx)
  list(APPEND private_headers
    pqCinemaTrackSelection.h
    pqExportViewSelection.h)

  list(APPEND ui_files
    Resources/UI/pqCinemaConfiguration.ui
    Resources/UI/pqCinemaTrack.ui
    Resources/UI/pqCinemaTrackSelection.ui
    Resources/UI/pqExportStateWizard.ui
    Resources/UI/pqExportViewSelection.ui
    Resources/UI/pqImageOutputInfo.ui)
endif ()

set(resource_files
  Resources/pqComponents.qrc)

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  Network Widgets)

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

vtk_module_add_module(ParaView::pqComponents
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${sources}
          ${ui_files}
          ${resource_files}
  PRIVATE_HEADERS ${private_headers})
vtk_module_link(ParaView::pqComponents
  PRIVATE
    Qt5::Network
    Qt5::Widgets)

if (PARAVIEW_ENABLE_PYTHON)
  # The pqSGExportStateWizard has subclasses that directly access the UI file,
  # and currently we don't have a clean way to break this hard dependency, so
  # for now we manually install this ui file.
  vtk_module_install_headers(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/pqComponents_autogen/include/ui_pqExportStateWizard.h")
endif ()
