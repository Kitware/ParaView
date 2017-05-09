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
  pqAboutDialog.cxx
  pqAboutDialog.h
  pqActionGroupInterface.cxx
  pqActionGroupInterface.h
  pqActiveObjects.cxx
  pqActiveObjects.h
  pqAnimatablePropertiesComboBox.cxx
  pqAnimatablePropertiesComboBox.h
  pqAnimatableProxyComboBox.cxx
  pqAnimatableProxyComboBox.h
  pqAnimationManager.cxx
  pqAnimationManager.h
  pqAnimationViewWidget.cxx
  pqAnimationViewWidget.h
  pqArrayListDomain.cxx
  pqArrayListDomain.h
  pqArrayStatusPropertyWidget.cxx
  pqArrayStatusPropertyWidget.h
  pqCalculatorWidget.cxx
  pqCalculatorWidget.h
  pqCameraDialog.cxx
  pqCameraDialog.h
  pqCameraKeyFrameWidget.cxx
  pqCameraKeyFrameWidget.h
  pqChangeInputDialog.cxx
  pqChangeInputDialog.h
  pqCollaborationPanel.cxx
  pqCollaborationPanel.h
  pqColorChooserButtonWithPalettes.cxx
  pqColorChooserButtonWithPalettes.h
  pqComboBoxDomain.cxx
  pqComboBoxDomain.h
  pqCommandPropertyWidget.cxx
  pqCommandPropertyWidget.h
  pqComparativeCueWidget.cxx
  pqComparativeCueWidget.h
  pqComparativeVisPanel.cxx
  pqComparativeVisPanel.h
  pqComponentsInit.cxx
  pqComponentsInit.h
  pqComponentsTestUtility.cxx
  pqComponentsTestUtility.h
  pqCompositeDataInformationTreeModel.cxx
  pqCompositeDataInformationTreeModel.h
  pqCompositeTreePropertyWidget.cxx
  pqCompositeTreePropertyWidget.h
  pqCustomFilterDefinitionModel.cxx
  pqCustomFilterDefinitionModel.h
  pqCustomFilterDefinitionWizard.cxx
  pqCustomFilterDefinitionWizard.h
  pqCustomFilterManager.cxx
  pqCustomFilterManager.h
  pqCustomFilterManagerModel.cxx
  pqCustomFilterManagerModel.h
  pqCustomViewButtonDialog.cxx
  pqCustomViewButtonDialog.h
  pqDataInformationModel.cxx
  pqDataInformationModel.h
  pqDataInformationWidget.cxx
  pqDataInformationWidget.h
  pqDialog.cxx
  pqDialog.h
  pqDisplayColorWidget.cxx
  pqDisplayColorWidget.h
  pqDisplayPanel.cxx
  pqDisplayPanel.h
  pqDisplayPanelInterface.cxx
  pqDisplayPanelInterface.h
  pqDisplayPanelPropertyWidget.cxx
  pqDisplayPanelPropertyWidget.h
  pqDisplayRepresentationWidget.cxx
  pqDisplayRepresentationWidget.h
  pqDockWindowInterface.cxx
  pqDockWindowInterface.h
  pqDoubleVectorPropertyWidget.cxx
  pqDoubleVectorPropertyWidget.h
  pqExodusIIVariableSelectionWidget.cxx
  pqExodusIIVariableSelectionWidget.h
  pqFieldSelectionAdaptor.cxx
  pqFieldSelectionAdaptor.h
  pqFileChooserWidget.cxx
  pqFileChooserWidget.h
  pqFindDataCreateSelectionFrame.cxx
  pqFindDataCreateSelectionFrame.h
  pqFindDataCurrentSelectionFrame.cxx
  pqFindDataCurrentSelectionFrame.h
  pqFindDataSelectionDisplayFrame.cxx
  pqFindDataSelectionDisplayFrame.h
  pqFindDataDialog.cxx
  pqFindDataDialog.h
  pqHighlightItemDelegate.cxx
  pqHighlightItemDelegate.h
  pqImageTip.cxx
  pqImageTip.h
  pqIndexSelectionWidget.cxx
  pqIndexSelectionWidget.h
  pqInputSelectorWidget.cxx
  pqInputSelectorWidget.h
  pqIntMaskPropertyWidget.cxx
  pqIntMaskPropertyWidget.h
  pqIntRangeWidget.cxx
  pqIntRangeWidget.h
  pqIntVectorPropertyWidget.cxx
  pqIntVectorPropertyWidget.h
  pqItemViewSearchWidget.cxx
  pqItemViewSearchWidget.h
  pqItemViewSearchWidgetEventPlayer.cxx
  pqItemViewSearchWidgetEventPlayer.h
  pqKeyFrameEditor.cxx
  pqKeyFrameEditor.h
  pqKeyFrameTimeValidator.cxx
  pqKeyFrameTimeValidator.h
  pqKeyFrameTypeWidget.cxx
  pqKeyFrameTypeWidget.h
  pqLabel.cxx
  pqLabel.h
  pqLinksEditor.cxx
  pqLinksEditor.h
  pqLinksManager.cxx
  pqLinksManager.h
  pqLiveInsituManager.h
  pqLiveInsituManager.cxx
  pqLiveInsituVisualizationManager.cxx
  pqLiveInsituVisualizationManager.h
  pqLockViewSizeCustomDialog.cxx
  pqLockViewSizeCustomDialog.h
  pqMemoryInspectorPanel.cxx
  pqMemoryInspectorPanel.h
  pqMultiBlockInspectorWidget.cxx
  pqMultiBlockInspectorWidget.h
  pqMultiViewWidget.cxx
  pqMultiViewWidget.h
  pqOrbitCreatorDialog.cxx
  pqOrbitCreatorDialog.h
  pqOutputPortComboBox.cxx
  pqOutputPortComboBox.h
  pqPipelineAnnotationFilterModel.cxx
  pqPipelineAnnotationFilterModel.h
  pqPipelineBrowserWidget.cxx
  pqPipelineBrowserWidget.h
  pqPipelineInputComboBox.cxx
  pqPipelineInputComboBox.h
  pqPipelineModel.cxx
  pqPipelineModel.h
  pqPipelineModelSelectionAdaptor.cxx
  pqPipelineModelSelectionAdaptor.h
  pqPipelineTimeKeyFrameEditor.cxx
  pqPipelineTimeKeyFrameEditor.h
  pqPluginDialog.cxx
  pqPluginDialog.h
  pqPluginTreeWidgetEventPlayer.cxx
  pqPluginTreeWidgetEventPlayer.h
  pqPluginTreeWidgetEventTranslator.cxx
  pqPluginTreeWidgetEventTranslator.h
  pqPluginTreeWidget.h
  pqPopOutWidget.cxx
  pqPopOutWidget.h
  pqPresetDialog.cxx
  pqPresetDialog.h
  pqPresetToPixmap.cxx
  pqPresetToPixmap.h
  pqPropertiesPanel.cxx
  pqPropertiesPanel.h
  pqPropertyGroupWidget.cxx
  pqPropertyGroupWidget.h
  pqPropertyWidget.cxx
  pqPropertyWidget.h
  pqPropertyWidgetDecorator.h
  pqPropertyWidgetDecorator.cxx
  pqPropertyWidgetInterface.cxx
  pqPropertyWidgetInterface.h
  pqProxiesWidget.cxx
  pqProxiesWidget.h
  pqProxyEditorPropertyWidget.cxx
  pqProxyEditorPropertyWidget.h
  pqProxyInformationWidget.cxx
  pqProxyInformationWidget.h
  pqProxyPropertyWidget.cxx
  pqProxyPropertyWidget.h
  pqProxySelectionWidget.cxx
  pqProxySelectionWidget.h
  pqProxySILModel.cxx
  pqProxySILModel.h
  pqProxyWidget.cxx
  pqProxyWidgetDialog.cxx
  pqProxyWidgetDialog.h
  pqProxyWidget.h
  pqQueryClauseWidget.cxx
  pqQueryClauseWidget.h
  pqRecentFilesMenu.cxx
  pqRecentFilesMenu.h
  pqRemoteCommandDialog.cxx
  pqRemoteCommandDialog.h
  pqRemoteCommandTemplateDialog.cxx
  pqRemoteCommandTemplateDialog.h
  pqRescaleRange.cxx
  pqRescaleRange.h
  pqSampleScalarAddRangeDialog.cxx
  pqSampleScalarAddRangeDialog.h
  pqSampleScalarWidget.cxx
  pqSampleScalarWidget.h
  pqScalarSetModel.cxx
  pqScalarSetModel.h
  pqScalarValueListPropertyWidget.cxx
  pqScalarValueListPropertyWidget.h
  pqSearchBox.cxx
  pqSearchBox.h
  pqSelectionAdaptor.cxx
  pqSelectionAdaptor.h
  pqSelectionInputWidget.cxx
  pqSelectionInputWidget.h
  pqSelectionLinkDialog.cxx
  pqSelectionLinkDialog.h
  pqSelectionManager.cxx
  pqSelectionManager.h
  pqSelectReaderDialog.cxx
  pqSelectReaderDialog.h
  pqServerConfigurationImporter.cxx
  pqServerConfigurationImporter.h
  pqServerConnectDialog.cxx
  pqServerConnectDialog.h
  pqServerLauncher.cxx
  pqServerLauncher.h
  pqSetBreakpointDialog.cxx
  pqSetBreakpointDialog.h
  pqSettingsDialog.cxx
  pqSettingsDialog.h
  pqSignalAdaptorKeyFrameType.cxx
  pqSignalAdaptorKeyFrameType.h
  pqSignalAdaptorSelectionTreeWidget.cxx
  pqSignalAdaptorSelectionTreeWidget.h
  pqSignalAdaptorTreeWidget.cxx
  pqSignalAdaptorTreeWidget.h
  pqSILModel.cxx
  pqSILModel.h
  pqSILWidget.cxx
  pqSILWidget.h
  pqSMSignalAdaptors.cxx
  pqSMSignalAdaptors.h
  pqStringVectorPropertyWidget.cxx
  pqStringVectorPropertyWidget.h
  pqTabbedMultiViewWidget.cxx
  pqTabbedMultiViewWidget.h
  pqTextureComboBox.cxx
  pqTextureComboBox.h
  pqTextureSelectorPropertyWidget.cxx
  pqTextureSelectorPropertyWidget.h
  pqTimerLogDisplay.cxx
  pqTimerLogDisplay.h
  pqToolTipTrapper.cxx
  pqToolTipTrapper.h
  pqTransferFunctionWidget.cxx
  pqTransferFunctionWidget.h
  pqTriggerOnIdleHelper.cxx
  pqTriggerOnIdleHelper.h
  pqUndoStackBuilder.cxx
  pqUndoStackBuilder.h
  pqVariableType.h
  pqVCRController.cxx
  pqVCRController.h
  pqViewContextMenuHandler.cxx
  pqViewContextMenuHandler.h
  pqViewContextMenuManager.cxx
  pqViewContextMenuManager.h
  pqViewFrameActionsInterface.cxx
  pqViewFrameActionsInterface.h
  pqViewFrame.cxx
  pqViewFrame.h
  pqViewManager.cxx
  pqViewManager.h
  pqWidgetRangeDomain.cxx
  pqWidgetRangeDomain.h
  pqAbstractItemSelectionModel.cxx
)

set (Module_MOC_HDRS
  pqAboutDialog.h
  pqActiveObjects.h
  pqAnimatablePropertiesComboBox.h
  pqAnimatableProxyComboBox.h
  pqAnimationManager.h
  pqAnimationViewWidget.h
  pqArrayListDomain.h
  pqArrayStatusPropertyWidget.h
  pqCalculatorWidget.h
  pqCameraDialog.h
  pqCameraKeyFrameWidget.h
  pqChangeInputDialog.h
  pqColorChooserButtonWithPalettes.h
  pqCollaborationPanel.h
  pqComboBoxDomain.h
  pqCommandPropertyWidget.h
  pqComparativeCueWidget.h
  pqComparativeVisPanel.h
  pqComponentsTestUtility.h
  pqCompositeDataInformationTreeModel.h
  pqCompositeTreePropertyWidget.h
  pqCustomFilterDefinitionWizard.h
  pqCustomFilterManager.h
  pqCustomFilterManagerModel.h
  pqCustomViewButtonDialog.h
  pqDataInformationModel.h
  pqDataInformationWidget.h
  pqDialog.h
  pqDisplayColorWidget.h
  pqDisplayPanel.h
  pqDisplayPanelPropertyWidget.h
  pqDisplayRepresentationWidget.h
  pqDoubleVectorPropertyWidget.h
  pqExodusIIVariableSelectionWidget.h
  pqFieldSelectionAdaptor.h
  pqFileChooserWidget.h
  pqFindDataCreateSelectionFrame.h
  pqFindDataCurrentSelectionFrame.h
  pqFindDataDialog.h
  pqFindDataSelectionDisplayFrame.h
  pqHighlightItemDelegate.h
  pqImageTip.h
  pqIndexSelectionWidget.h
  pqInputSelectorWidget.h
  pqIntMaskPropertyWidget.h
  pqIntRangeWidget.h
  pqIntVectorPropertyWidget.h
  pqItemViewSearchWidgetEventPlayer.h
  pqItemViewSearchWidget.h
  pqKeyFrameEditor.h
  pqKeyFrameTimeValidator.h
  pqKeyFrameTypeWidget.h
  pqLabel.h
  pqLinksEditor.h
  pqLinksManager.h
  pqLiveInsituManager.h
  pqLiveInsituVisualizationManager.h
  pqLockViewSizeCustomDialog.h
  pqMemoryInspectorPanel.h
  pqMultiBlockInspectorWidget.h
  pqMultiViewWidget.h
  pqOrbitCreatorDialog.h
  pqOutputPortComboBox.h
  pqPipelineAnnotationFilterModel.h
  pqPipelineBrowserWidget.h
  pqPipelineInputComboBox.h
  pqPipelineModel.h
  pqPipelineModelSelectionAdaptor.h
  pqPipelineTimeKeyFrameEditor.h
  pqPluginDialog.h
  pqPluginTreeWidgetEventPlayer.h
  pqPluginTreeWidgetEventTranslator.h
  pqPluginTreeWidget.h
  pqPopOutWidget.h
  pqPresetDialog.h
  pqPresetToPixmap.h
  pqPropertiesPanel.h
  pqPropertyGroupWidget.h
  pqPropertyWidgetDecorator.h
  pqPropertyWidget.h
  pqProxiesWidget.h
  pqProxyEditorPropertyWidget.h
  pqProxyInformationWidget.h
  pqProxyPropertyWidget.h
  pqProxySelectionWidget.h
  pqProxySILModel.h
  pqProxyWidgetDialog.h
  pqProxyWidget.h
  pqQueryClauseWidget.h
  pqRecentFilesMenu.h
  pqRemoteCommandDialog.h
  pqRemoteCommandTemplateDialog.h
  pqRescaleRange.h
  pqSampleScalarAddRangeDialog.h
  pqSampleScalarWidget.h
  pqScalarSetModel.h
  pqScalarValueListPropertyWidget.h
  pqSearchBox.h
  pqSelectionAdaptor.h
  pqSelectionInputWidget.h
  pqSelectionLinkDialog.h
  pqSelectionManager.h
  pqSelectReaderDialog.h
  pqServerConfigurationImporter.h
  pqServerConnectDialog.h
  pqServerLauncher.h
  pqSetBreakpointDialog.h
  pqSettingsDialog.h
  pqSignalAdaptorKeyFrameType.h
  pqSignalAdaptorSelectionTreeWidget.h
  pqSignalAdaptorTreeWidget.h
  pqSILModel.h
  pqSILWidget.h
  pqSMSignalAdaptors.h
  pqStringVectorPropertyWidget.h
  pqTabbedMultiViewWidget.h
  pqTextureComboBox.h
  pqTextureSelectorPropertyWidget.h
  pqTimerLogDisplay.h
  pqToolTipTrapper.h
  pqTransferFunctionWidget.h
  pqTriggerOnIdleHelper.h
  pqVCRController.h
  pqViewContextMenuHandler.h
  pqViewContextMenuManager.h
  pqViewFrame.h
  pqViewManager.h
  pqWidgetRangeDomain.h
  pqAbstractItemSelectionModel.h
)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND Module_SRCS
    pqMultiBlockInspectorPanel.cxx
    pqMultiBlockInspectorPanel.h)
  list(APPEND Module_MOC_HDRS
    pqMultiBlockInspectorPanel.h)
endif()

set (Module_UI_FILES
  Resources/UI/pqAbortAnimation.ui
  Resources/UI/pqAboutDialog.ui
  Resources/UI/pqCalculatorWidget.ui
  Resources/UI/pqCameraDialog.ui
  Resources/UI/pqCameraKeyFrameWidget.ui
  Resources/UI/pqChangeInputDialog.ui
  Resources/UI/pqCollaborationPanel.ui
  Resources/UI/pqComparativeParameterRangeDialog.ui
  Resources/UI/pqComparativeVisPanel.ui
  Resources/UI/pqContourControls.ui
  Resources/UI/pqCustomFilterDefinitionWizard.ui
  Resources/UI/pqCustomFilterManager.ui
  Resources/UI/pqCustomViewButtonDialog.ui
  Resources/UI/pqDisplayRepresentationWidget.ui
  Resources/UI/pqEmptyView.ui
  Resources/UI/pqFindDataCreateSelectionFrame.ui
  Resources/UI/pqFindDataCurrentSelectionFrame.ui
  Resources/UI/pqFindDataDialog.ui
  Resources/UI/pqFindDataSelectionDisplayFrame.ui
  Resources/UI/pqItemViewSearchWidget.ui
  Resources/UI/pqKeyFrameEditor.ui
  Resources/UI/pqKeyFrameTypeWidget.ui
  Resources/UI/pqLinksEditor.ui
  Resources/UI/pqLinksManager.ui
  Resources/UI/pqLockViewSizeCustomDialog.ui
  Resources/UI/pqMemoryInspectorPanelForm.ui
  Resources/UI/pqMultiBlockInspectorWidget.ui
  Resources/UI/pqOrbitCreatorDialog.ui
  Resources/UI/pqPipelineTimeKeyFrameEditor.ui
  Resources/UI/pqPluginDialog.ui
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
  Resources/UI/pqSelectionInputWidget.ui
  Resources/UI/pqSelectionLinkDialog.ui
  Resources/UI/pqSelectReaderDialog.ui
  Resources/UI/pqServerConnectDialog.ui
  Resources/UI/pqServerLauncherDialog.ui
  Resources/UI/pqSetBreakpointDialog.ui
  Resources/UI/pqSettingsDialog.ui
  Resources/UI/pqTimerLogDisplay.ui
  Resources/UI/pqViewFrame.ui
)

if(PARAVIEW_ENABLE_PYTHON)
  # code for generating specialized Python scripts
  # from the GUI using trace information
  LIST(APPEND Module_SRCS
    pqImageOutputInfo.cxx
    pqImageOutputInfo.h
    pqCinemaTrack.cxx
    pqCinemaTrack.h
    pqCinemaConfiguration.cxx
    pqExportViewSelection.cxx
    pqCinemaTrackSelection.cxx
    pqSGExportStateWizard.cxx
    pqSGExportStateWizard.h
    pqSGPluginManager.cxx
    pqSGPluginManager.h
    pqSGWritersMenuManager.cxx
    pqSGWritersMenuManager.h
    )
  LIST(APPEND Module_MOC_HDRS
    pqImageOutputInfo.h
    pqCinemaTrack.h
    pqCinemaConfiguration.h
    pqExportViewSelection.h
    pqCinemaTrackSelection.h
    pqSGExportStateWizard.h
    pqSGPluginManager.h
    pqSGWritersMenuManager.h
    )
  LIST(APPEND Module_UI_FILES
    Resources/UI/pqImageOutputInfo.ui
    Resources/UI/pqCinemaTrack.ui
    Resources/UI/pqExportStateWizard.ui
    Resources/UI/pqCinemaConfiguration.ui
    Resources/UI/pqExportViewSelection.ui
    Resources/UI/pqCinemaTrackSelection.ui
    )
endif()

set (Module_RESOURCE_FILES
  Resources/pqComponents.qrc
)

include(ParaViewQt)
pv_find_package_qt(qt_private_targets REQUIRED QUIET
  QT4_COMPONENTS QtNetwork
  QT5_COMPONENTS Widgets Network)
pv_qt_wrap_cpp(__Module_MOC_SRCS ${Module_MOC_HDRS})
pv_qt_wrap_ui(__Module_UI_SRCS ${Module_UI_FILES})
pv_qt_add_resources(__Module_RESOURCE_SRCS ${Module_RESOURCE_FILES})

set(${vtk-module}_EXPORT_CODE_INSTALL
  "include(\${ParaView_CMAKE_DIR}/ParaViewQt.cmake)
  pv_find_package_qt(__${vtk-module}_qt_targets REQUIRED QUIET
    QT4_COMPONENTS QtNetwork
    QT5_COMPONENTS Widgets Network)")
set(${vtk-module}_EXPORT_CODE_BUILD "${${vtk-module}_EXPORT_CODE_INSTALL}")

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
vtk_module_library(${vtk-module}
  ${Module_SRCS}
  ${__Module_RESOURCE_SRCS}
  ${__Module_MOC_SRCS}
  ${__Module_UI_SRCS})
target_link_libraries(${vtk-module} LINK_PRIVATE ${qt_private_targets})

#the pqSGExportStateWizard has subclasses that directly access
#the UI file, and currently we don't have a clean way to break this hard
#dependency, so for now we manually install this ui file.
if(PARAVIEW_INSTALL_DEVELOPMENT_FILES AND PARAVIEW_ENABLE_PYTHON)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ui_pqExportStateWizard.h"
          DESTINATION "${VTK_INSTALL_INCLUDE_DIR}")
endif()

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
#  # QT resources
#  SET(resPath "${CMAKE_CURRENT_SOURCE_DIR}/Resources/XML")
#  GLOB_INSTALL_DEVELOPMENT(
#    "${CMAKE_CURRENT_SOURCE_DIR}"
#    "${PV_INSTALL_INCLUDE_DIR}/Qt/Components/Resources/XML"
#    "${resPath}/*.xml")
#  SET(resPath "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")
#  GLOB_INSTALL_DEVELOPMENT(
#    "${CMAKE_CURRENT_SOURCE_DIR}"
#    "${PV_INSTALL_INCLUDE_DIR}/Qt/Components/Resources/UI"
#    "${resPath}/*.ui")
#  SET(resPath "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons")
#  GLOB_INSTALL_DEVELOPMENT(
#    "${CMAKE_CURRENT_SOURCE_DIR}"
#    "${PV_INSTALL_INCLUDE_DIR}/Qt/Components/Resources/Icons"
#    "${resPath}/*.ico;${resPath}/*.png;${resPath}/*.svg;${resPath}/*.xpm;${resPath}/*.rc")
#ENDIF ()
