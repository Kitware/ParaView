PROJECT(pqCore)


# Any build configured headers should be added to this 
# variable so that they also get installed.
SET (dynamicHeaders)

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${QtTesting_BINARY_DIR}
  ${pqCore_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  ${pqCore_BINARY_DIR}/Resources/UI
)

IF(QT_TESTING_WITH_PYTHON)
  SET(PYTHON_CORE_SRCS 
    pqPythonEventSourceImage.cxx
    pqPythonEventSourceImage.h
  )
  SET(PYTHON_MOC_SRCS 
    pqPythonEventSourceImage.h
  )
  INCLUDE_DIRECTORIES(
    ${PYTHON_INCLUDE_PATH}
  )
ENDIF(QT_TESTING_WITH_PYTHON)

INCLUDE_DIRECTORIES(
  ${protobuf_SOURCE_DIR}/src
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pq3DWidgetFactory.h
  pqAnimationCue.h
  pqAnimationScene.h
  pqApplicationCore.h
  pqBrandPluginsLoader.h
  pqChartRepresentation.h
  pqChartSeriesSettingsModel.h
  pqContextView.h
  pqCollaborationEventPlayer.h
  pqCollaborationManager.h
  pqColorButtonEventPlayer.h
  pqColorButtonEventTranslator.h
  pqComparativeContextView.h
  pqComparativeRenderView.h
  pqComparativeXYBarChartView.h
  pqComparativeXYChartView.h
  pqCoreTestUtility.h
  pqCoreUtilities.h
  pqDataRepresentation.h
  pqDisplayPolicy.h
  pqFileDialog.h
  pqFileDialogEventTranslator.h
  pqFileDialogFavoriteModel.h
  pqFileDialogFilter.h
  pqFileDialogModel.h
  pqFileDialogRecentDirsModel.h
  pqFlatTreeViewEventTranslator.h
  pqFormBuilder.h
  pqHelperProxyStateLoader.h
  pqHistogramTableModel.h
  pqInterfaceTracker.h
  pqLinksModel.h
  pqLinkViewWidget.h
  pqLookupTableManager.h
  pqNonEditableStyledItemDelegate.h
  pqObjectBuilder.h
  pqOutputWindow.h
  pqOutputWindowAdapter.h
  pqOutputPort.h
  pqParallelCoordinatesChartView.h
  pqPipelineSource.h
  pqPipelineFilter.h
  pqPipelineRepresentation.h
  pqPlotMatrixView.h
  pqPlotSettingsModel.h
  pqPluginManager.h
  pqProgressManager.h
  pqPropertyLinks.h
  pqPropertyLinksConnection.h
  pqPropertyManager.h
  pqProxy.h
  pqQVTKWidget.h
  pqQVTKWidgetEventTranslator.h
  pqRecentlyUsedResourcesList.h
  pqRenderView.h
  pqRenderViewBase.h
  pqRepresentation.h
  pqRubberBandHelper.h
  pqScalarBarRepresentation.h
  pqScalarBarVisibilityAdaptor.h
  pqScalarOpacityFunction.h
  pqScalarsToColors.h
#  pqScatterPlotRepresentation.h
  pqServer.h
  pqServerConfigurationCollection.h
  pqServerManagerModel.h
  pqServerManagerModelItem.h
  pqServerManagerObserver.h
  pqSettings.h
  pqSpreadSheetView.h
  pqSpreadSheetViewModel.h
  pqSpreadSheetViewSelectionModel.h
  pqSpreadSheetViewWidget.h
  pqStandardServerManagerModelInterface.h
  pqStandardViewModules.h
  pqTextRepresentation.h
  pqTimeKeeper.h
  pqUndoStack.h
  pqView.h
  pqViewExporterManager.h
  pqXMLEventObserver.h
  pqXMLEventSource.h
  pqXYBarChartView.h
  pqXYChartView.h
  ${PYTHON_MOC_SRCS}
  )
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SET(UI_FORMS
  Resources/UI/pqOutputWindow.ui
  Resources/UI/pqFileDialog.ui
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
)

SET(UI_RESOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/Resources/pqCore.qrc"
)

QT4_ADD_RESOURCES(RCS_SOURCES
  ${UI_RESOURCES}
)


SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${UI_FORMS}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
)

PVVTK_ADD_LIBRARY(pqCore
  pq3DWidgetFactory.cxx
  pq3DWidgetFactory.h
  pqAnimationCue.cxx
  pqAnimationCue.h
  pqAnimationScene.cxx
  pqAnimationScene.h
  pqAnimationSceneImageWriter.cxx
  pqAnimationSceneImageWriter.h
  pqApplicationCore.cxx
  pqApplicationCore.h
  pqBrandPluginsLoader.cxx
  pqBrandPluginsLoader.h
  pqChartRepresentation.cxx
  pqChartRepresentation.h
  pqChartSeriesSettingsModel.cxx
  pqChartSeriesSettingsModel.h
  pqCollaborationEventPlayer.cxx
  pqCollaborationEventPlayer.h
  pqCollaborationManager.cxx
  pqCollaborationManager.h
  pqColorButtonEventPlayer.cxx
  pqColorButtonEventPlayer.h
  pqColorButtonEventTranslator.cxx
  pqColorButtonEventTranslator.h
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
  pqCoreExport.h
  pqCoreInit.cxx
  pqCoreInit.h
  pqCoreTestUtility.cxx
  pqCoreTestUtility.h
  pqCoreUtilities.cxx
  pqCoreUtilities.h
  pqDataRepresentation.cxx
  pqDataRepresentation.h
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
  pqFormBuilder.cxx
  pqFormBuilder.h
  pqHelperProxyRegisterUndoElement.cxx
  pqHelperProxyRegisterUndoElement.h
  pqHelperProxyStateLoader.cxx
  pqHelperProxyStateLoader.h
  pqHistogramTableModel.cxx
  pqHistogramTableModel.h
  pqImageUtil.cxx
  pqImageUtil.h
  pqInterfaceTracker.cxx
  pqInterfaceTracker.h
  pqLinksModel.cxx
  pqLinksModel.h
  pqLinkViewWidget.cxx
  pqLinkViewWidget.h
  pqLookupTableManager.cxx
  pqLookupTableManager.h
  pqNameCount.cxx
  pqNameCount.h
  pqNonEditableStyledItemDelegate.cxx
  pqNonEditableStyledItemDelegate.h
  pqObjectBuilder.cxx
  pqObjectBuilder.h
  pqOptions.cxx
  pqOptions.h
  pqOutputPort.cxx
  pqOutputPort.h
  pqOutputWindowAdapter.cxx
  pqOutputWindowAdapter.h
  pqOutputWindow.cxx
  pqOutputWindow.h
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
  pqPlotSettingsModel.cxx
  pqPlotSettingsModel.h
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
  pqQVTKWidget.cxx
  pqQVTKWidgetEventPlayer.cxx
  pqQVTKWidgetEventPlayer.h
  pqQVTKWidgetEventTranslator.cxx
  pqQVTKWidgetEventTranslator.h
  pqQVTKWidget.h
  pqRecentlyUsedResourcesList.cxx
  pqRecentlyUsedResourcesList.h
  pqRenderViewBase.cxx
  pqRenderViewBase.h
  pqRenderView.cxx
  pqRenderView.h
  pqRepresentation.cxx
  pqRepresentation.h
  pqRubberBandHelper.cxx
  pqRubberBandHelper.h
  pqScalarBarRepresentation.cxx
  pqScalarBarRepresentation.h
  pqScalarBarVisibilityAdaptor.cxx
  pqScalarBarVisibilityAdaptor.h
  pqScalarOpacityFunction.cxx
  pqScalarOpacityFunction.h
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
  pqStandardViewModules.cxx
  pqStandardViewModules.h
  pqTextRepresentation.cxx
  pqTextRepresentation.h
  pqTimeKeeper.cxx
  pqTimeKeeper.h
  pqTreeLayoutStrategyInterface.cxx
  pqTreeLayoutStrategyInterface.h
  pqUndoStack.cxx
  pqUndoStack.h
  pqView.cxx
  pqViewExporterManager.cxx
  pqViewExporterManager.h
  pqView.h
  pqViewModuleInterface.cxx
  pqViewModuleInterface.h
  pqXMLEventObserver.cxx
  pqXMLEventObserver.h
  pqXMLEventSource.cxx
  pqXMLEventSource.h
  pqXMLUtil.cxx
  pqXMLUtil.h
  pqXYBarChartView.cxx
  pqXYBarChartView.h
  pqXYChartView.cxx
  pqXYChartView.h

  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}

  ${PYTHON_CORE_SRCS}
  vtkPVGUIPluginInterface.cxx
  vtkPVGUIPluginInterface.h
)

#ADD_LIBRARY(pqCore_old
#
##  pqScatterPlotRepresentation.cxx
##  pqScatterPlotRepresentation.h
##  pqScatterPlotView.cxx
##  pqScatterPlotView.h
#
#)

TARGET_LINK_LIBRARIES(pqCore
  pqWidgets
  protobuf
  QtTesting
  vtkChartsCore
  vtkGUISupportQt
  vtkPVServerManager
)

pv_set_link_interface_libs(pqCore
  pqWidgets
  vtkPVServerManager)

IF(QT_TESTING_WITH_PYTHON)
  TARGET_LINK_LIBRARIES(pqCore
    ${PYTHON_LIBRARIES}
  )
ENDIF(QT_TESTING_WITH_PYTHON)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pqCoreExport.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/pqCoreExport.h")
SET(dynamicHeaders 
    "${dynamicHeaders};${CMAKE_CURRENT_BINARY_DIR}/pqCoreExport.h")

IF(NOT PV_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS pqCore
    EXPORT ${PV_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF()

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
  # QT resources
  SET(resPath "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")
  GLOB_INSTALL_DEVELOPMENT( 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PV_INSTALL_INCLUDE_DIR}/Qt/Core/Resources/UI"
    "${resPath}/*.ui")
  SET(resPath "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons")
  GLOB_INSTALL_DEVELOPMENT( 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PV_INSTALL_INCLUDE_DIR}/Qt/Core/Resources/Icons"
    "${resPath}/*.ico;${resPath}/*.png;${resPath}/*.svg;${resPath}/*.xpm;${resPath}/*.rc")
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)

IF(BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
ENDIF(BUILD_TESTING)

