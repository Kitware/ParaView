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
#=========================================================================
set(Module_SRCS
  pqAnimationCue.cxx
  pqAnimationCue.h
  pqAnimationScene.cxx
  pqAnimationScene.h
  pqApplicationCore.cxx
  pqApplicationCore.h
  pqAutoStartInterface.cxx
  pqAutoStartInterface.h
  pqBoxChartView.cxx
  pqBoxChartView.h
  pqCheckBoxDelegate.h
  pqCheckBoxDelegate.cxx
  pqCollaborationEventPlayer.cxx
  pqCollaborationEventPlayer.h
  pqCollaborationManager.cxx
  pqCollaborationManager.h
  pqComparativeContextView.cxx
  pqComparativeContextView.h
  pqComparativeRenderView.cxx
  pqComparativeRenderView.h
  pqComparativeXYBarChartView.cxx
  pqComparativeXYBarChartView.h
  pqComparativeXYChartView.cxx
  pqComparativeXYChartView.h
  pqContextView.cxx
  pqContextView.h
  pqCoreInit.cxx
  pqCoreInit.h
  pqCoreTestUtility.cxx
  pqCoreTestUtility.h
  pqCoreUtilities.cxx
  pqCoreUtilities.h
  pqDataRepresentation.cxx
  pqDataRepresentation.h
  pqDebug.cxx
  pqDebug.h
  pqDisplayPolicy.cxx
  pqDisplayPolicy.h
  pqFileDialog.cxx
  pqFileDialogEventPlayer.cxx
  pqFileDialogEventPlayer.h
  pqFileDialogEventTranslator.cxx
  pqFileDialogEventTranslator.h
  pqFileDialogFavoriteModel.cxx
  pqFileDialogFavoriteModel.h
  pqFileDialogFilter.cxx
  pqFileDialogFilter.h
  pqFileDialog.h
  pqFileDialogModel.cxx
  pqFileDialogModel.h
  pqFileDialogRecentDirsModel.cxx
  pqFileDialogRecentDirsModel.h
  pqFlatTreeViewEventPlayer.cxx
  pqFlatTreeViewEventPlayer.h
  pqFlatTreeViewEventTranslator.cxx
  pqFlatTreeViewEventTranslator.h
  pqGraphLayoutStrategyInterface.cxx
  pqGraphLayoutStrategyInterface.h
  pqImageUtil.cxx
  pqImageUtil.h
  pqInteractiveViewLink.cxx
  pqInteractiveViewLink.h
  pqInterfaceTracker.cxx
  pqInterfaceTracker.h
  pqLinksModel.cxx
  pqLinksModel.h
  pqLinkViewWidget.cxx
  pqLinkViewWidget.h
  pqMultiSliceAxisWidget.cxx
  pqMultiSliceAxisWidget.h
  pqMultiSliceView.cxx
  pqMultiSliceView.h
  pqNonEditableStyledItemDelegate.cxx
  pqNonEditableStyledItemDelegate.h
  pqObjectBuilder.cxx
  pqObjectBuilder.h
  pqOptions.cxx
  pqOptions.h
  pqOutputPort.cxx
  pqOutputPort.h
  pqOutputWidget.cxx
  pqOutputWidget.h
  pqOutputWindowAdapter.cxx
  pqOutputWindowAdapter.h
  pqOutputWindow.cxx
  pqOutputWindow.h
  pqOutputWindowModel.cxx
  pqOutputWindowModel.h
  pqParallelCoordinatesChartView.cxx
  pqParallelCoordinatesChartView.h
  pqPipelineFilter.cxx
  pqPipelineFilter.h
  pqPipelineRepresentation.cxx
  pqPipelineRepresentation.h
  pqPipelineSource.cxx
  pqPipelineSource.h
  pqPlotMatrixView.cxx
  pqPlotMatrixView.h
  pqPluginManager.cxx
  pqPluginManager.h
  pqProgressManager.cxx
  pqProgressManager.h
  pqPropertyLinks.cxx
  pqPropertyLinks.h
  pqPropertyLinksConnection.cxx
  pqPropertyLinksConnection.h
  pqPropertyManager.cxx
  pqPropertyManager.h
  pqProxy.cxx
  pqProxy.h
  pqProxyModifiedStateUndoElement.cxx
  pqProxyModifiedStateUndoElement.h
  pqProxySelection.cxx
  pqProxySelection.h
  pqQVTKWidgetBase.h
  pqQVTKWidget.cxx
  pqQVTKWidgetEventPlayer.cxx
  pqQVTKWidgetEventPlayer.h
  pqQVTKWidgetEventTranslator.cxx
  pqQVTKWidgetEventTranslator.h
  pqQVTKWidget.h
  pqRecentlyUsedResourceLoaderInterface.cxx
  pqRecentlyUsedResourceLoaderInterface.h
  pqRecentlyUsedResourcesList.cxx
  pqRecentlyUsedResourcesList.h
  pqRenderViewBase.cxx
  pqRenderViewBase.h
  pqRenderView.cxx
  pqRenderView.h
  pqRepresentation.cxx
  pqRepresentation.h
  pqScalarBarRepresentation.cxx
  pqScalarBarRepresentation.h
  pqScalarsToColors.cxx
  pqScalarsToColors.h
  pqServer.cxx
  pqServer.h
  pqServerConfiguration.cxx
  pqServerConfiguration.h
  pqServerConfigurationCollection.cxx
  pqServerConfigurationCollection.h
  pqServerManagerModel.cxx
  pqServerManagerModel.h
  pqServerManagerModelInterface.cxx
  pqServerManagerModelInterface.h
  pqServerManagerModelItem.cxx
  pqServerManagerModelItem.h
  pqServerManagerObserver.cxx
  pqServerManagerObserver.h
  pqServerResource.cxx
  pqServerResource.h
  pqSettings.cxx
  pqSettings.h
  pqSMAdaptor.cxx
  pqSMAdaptor.h
  pqSMProxy.cxx
  pqSMProxy.h
  pqSpreadSheetView.cxx
  pqSpreadSheetView.h
  pqSpreadSheetViewModel.cxx
  pqSpreadSheetViewModel.h
  pqSpreadSheetViewSelectionModel.cxx
  pqSpreadSheetViewSelectionModel.h
  pqSpreadSheetViewWidget.cxx
  pqSpreadSheetViewWidget.h
  pqStandardServerManagerModelInterface.cxx
  pqStandardServerManagerModelInterface.h
  pqTimeKeeper.cxx
  pqTimeKeeper.h
  pqTreeLayoutStrategyInterface.cxx
  pqTreeLayoutStrategyInterface.h
  pqUndoStack.cxx
  pqUndoStack.h
  pqView.cxx
  pqView.h
  pqXMLEventObserver.cxx
  pqXMLEventObserver.h
  pqXMLEventSource.cxx
  pqXMLEventSource.h
  pqXMLUtil.cxx
  pqXMLUtil.h
  pqXYBagChartView.cxx
  pqXYBagChartView.h
  pqXYBarChartView.cxx
  pqXYBarChartView.h
  pqXYFunctionalBagChartView.cxx
  pqXYFunctionalBagChartView.h
  pqXYHistogramChartView.cxx
  pqXYHistogramChartView.h
  pqXYChartView.cxx
  pqXYChartView.h
  vtkPVGUIPluginInterface.cxx
  vtkPVGUIPluginInterface.h
)

set (Module_MOC_HDRS
  pqAnimationCue.h
  pqAnimationScene.h
  pqApplicationCore.h
  pqBoxChartView.h
  pqCheckBoxDelegate.h
  pqCollaborationEventPlayer.h
  pqCollaborationManager.h
  pqComparativeContextView.h
  pqComparativeRenderView.h
  pqComparativeXYBarChartView.h
  pqComparativeXYChartView.h
  pqContextView.h
  pqCoreTestUtility.h
  pqCoreUtilities.h
  pqDataRepresentation.h
  pqDisplayPolicy.h
  pqFileDialogEventPlayer.h
  pqFileDialogEventTranslator.h
  pqFileDialogFavoriteModel.h
  pqFileDialogFilter.h
  pqFileDialog.h
  pqFileDialogModel.h
  pqFileDialogRecentDirsModel.h
  pqFlatTreeViewEventPlayer.h
  pqFlatTreeViewEventTranslator.h
  pqInteractiveViewLink.h
  pqInterfaceTracker.h
  pqLinksModel.h
  pqLinkViewWidget.h
  pqMultiSliceAxisWidget.h
  pqMultiSliceView.h
  pqNonEditableStyledItemDelegate.h
  pqObjectBuilder.h
  pqOutputPort.h
  pqOutputWidget.h
  pqOutputWindowAdapter.h
  pqOutputWindow.h
  pqOutputWindowModel.h
  pqParallelCoordinatesChartView.h
  pqPipelineFilter.h
  pqPipelineRepresentation.h
  pqPipelineSource.h
  pqPlotMatrixView.h
  pqPluginManager.h
  pqProgressManager.h
  pqPropertyLinksConnection.h
  pqPropertyLinks.h
  pqPropertyManager.h
  pqProxy.h
  pqQVTKWidgetEventPlayer.h
  pqQVTKWidgetEventTranslator.h
  pqQVTKWidget.h
  pqRecentlyUsedResourcesList.h
  pqRenderViewBase.h
  pqRenderView.h
  pqRepresentation.h
  pqScalarBarRepresentation.h
  pqScalarsToColors.h
#  pqScatterPlotRepresentation.h
  pqServerConfigurationCollection.h
  pqServer.h
  pqServerManagerModel.h
  pqServerManagerModelItem.h
  pqServerManagerObserver.h
  pqSettings.h
  pqSpreadSheetView.h
  pqSpreadSheetViewModel.h
  pqSpreadSheetViewSelectionModel.h
  pqSpreadSheetViewWidget.h
  pqStandardServerManagerModelInterface.h
  pqTimeKeeper.h
  pqUndoStack.h
  pqView.h
  pqXMLEventObserver.h
  pqXMLEventSource.h
  pqXYBagChartView.h
  pqXYBarChartView.h
  pqXYFunctionalBagChartView.h
  pqXYHistogramChartView.h
  pqXYChartView.h
)

set (Module_UI_FILES
  Resources/UI/pqOutputWidget.ui
  Resources/UI/pqOutputWindow.ui
  Resources/UI/pqFileDialog.ui
)

set (Module_RESOURCE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/Resources/pqCore.qrc"
)

if (PARAVIEW_ENABLE_PYTHON)
  list (APPEND Module_SRCS
    pqPythonView.cxx
    pqPythonView.h
    )
  list (APPEND Module_MOC_HDRS
    pqPythonView.h
    )
endif()

#-------------------------------------------------------------------------
include(ParaViewQt)
if (PARAVIEW_USE_QTHELP)
  pv_find_package_qt(qt_private_targets REQUIRED QUIET
    QT4_COMPONENTS QtHelp
    QT5_COMPONENTS Help)

  set(${vtk-module}_EXPORT_CODE_INSTALL
    "include(\${ParaView_CMAKE_DIR}/ParaViewQt.cmake)
    pv_find_package_qt(__${vtk-module}_qt_targets REQUIRED QUIET
      QT4_COMPONENTS QtHelp
      QT5_COMPONENTS Help)")
else()
  pv_find_package_qt(qt_private_targets REQUIRED QUIET
    QT4_COMPONENTS QtGui
    QT5_COMPONENTS Widgets)

  set(${vtk-module}_EXPORT_CODE_INSTALL
    "include(\${ParaView_CMAKE_DIR}/ParaViewQt.cmake)
    pv_find_package_qt(__${vtk-module}_qt_targets REQUIRED QUIET
      QT4_COMPONENTS QtGui
      QT5_COMPONENTS Widgets)")
endif()
set(${vtk-module}_EXPORT_CODE_BUILD "${${vtk-module}_EXPORT_CODE_INSTALL}")

pv_qt_wrap_cpp(__Module_MOC_SRCS ${Module_MOC_HDRS})
pv_qt_wrap_ui(__Module_UI_SRCS ${Module_UI_FILES})
pv_qt_add_resources(__Module_RESOURCE_SRCS ${Module_RESOURCE_FILES})

#-------------------------------------------------------------------------
set(${vtk-module}_NO_HeaderTest 1)


SOURCE_GROUP("Resources" FILES
  ${Module_UI_FILES}
  ${Module_RESOURCE_FILES}
)

SOURCE_GROUP("Generated" FILES
  ${__Module_RESOURCE_SRCS}
  ${__Module_MOC_SRCS}
  ${__Module_UI_SRCS}
)

vtk_module_library(${vtk-module}
  ${Module_SRCS}
  ${__Module_RESOURCE_SRCS}
  ${__Module_MOC_SRCS}
  ${__Module_UI_SRCS})
target_link_libraries(${vtk-module}
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
#  # QT resources
#  SET(resPath "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")
#  GLOB_INSTALL_DEVELOPMENT(
#    "${CMAKE_CURRENT_SOURCE_DIR}"
#    "${PV_INSTALL_INCLUDE_DIR}/Qt/Core/Resources/UI"
#    "${resPath}/*.ui")
#  SET(resPath "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons")
#  GLOB_INSTALL_DEVELOPMENT(
#    "${CMAKE_CURRENT_SOURCE_DIR}"
#    "${PV_INSTALL_INCLUDE_DIR}/Qt/Core/Resources/Icons"
#    "${resPath}/*.ico;${resPath}/*.png;${resPath}/*.svg;${resPath}/*.xpm;${resPath}/*.rc")
#ENDIF ()
#
#IF(BUILD_TESTING)
#  ADD_SUBDIRECTORY(Testing)
#ENDIF()

