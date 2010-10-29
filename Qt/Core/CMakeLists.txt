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
  pqContextView.h
  pqColorButtonEventPlayer.h
  pqColorButtonEventTranslator.h
  pqCommandServerStartup.h
  pqComparativeContextView.h
  pqComparativeRenderView.h
  pqComparativeXYBarChartView.h
  pqComparativeXYChartView.h
  pqCoreTestUtility.h
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
  pqLinksModel.h
  pqLinkViewWidget.h
  pqLookupTableManager.h
  pqManualServerStartup.h
  pqObjectBuilder.h
  pqOutputWindow.h
  pqOutputWindowAdapter.h
  pqOutputPort.h
  pqParallelCoordinatesChartView.h
  pqParallelCoordinatesSettingsModel.h
  pqPickHelper.h
  pqPipelineSource.h
  pqPipelineFilter.h
  pqPipelineRepresentation.h
  pqPlotSettingsModel.h
  pqPluginManager.h
  pqProgressManager.h
  pqPropertyLinks.h
  pqPropertyManager.h
  pqProxy.h
  pqQVTKWidget.h
  pqQVTKWidgetEventTranslator.h
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
  pqServerManagerModel.h
  pqServerManagerModelItem.h
  pqServerManagerObserver.h
  pqServerManagerSelectionModel.h
  pqServerResources.h
  pqServerStartup.h
  pqServerStartups.h
  pqSettings.h
  pqSpreadSheetView.h
  pqSpreadSheetViewModel.h
  pqSpreadSheetViewSelectionModel.h
  pqSpreadSheetViewWidget.h
  pqStandardServerManagerModelInterface.h
  pqStandardViewModules.h
  pqTableView.h
  pqTextRepresentation.h
  pqTimeKeeper.h
  pqTwoDRenderView.h
#  pqScatterPlotView.h
  pqUndoStack.h
  pqView.h
  pqViewExporterManager.h
  pqWriterFactory.h
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

SET(PV_TEST_SERVER_ROOT "localhost" CACHE STRING
  "Node which serves as the root node for the cluster when testing in parallel")
MARK_AS_ADVANCED(PV_TEST_SERVER_ROOT)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/Resources/pqTestingServers.pvsc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Resources/pqTestingServers.pvsc")

GENERATE_QT_RESOURCE_FROM_FILES(
  "${CMAKE_CURRENT_BINARY_DIR}/Resources/pqCoreTesting.qrc"
  "/pqCoreTesting"
  "${CMAKE_CURRENT_BINARY_DIR}/Resources/pqTestingServers.pvsc")

SET(UI_RESOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/Resources/pqCore.qrc"
  "${CMAKE_CURRENT_BINARY_DIR}/Resources/pqCoreTesting.qrc"
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

ADD_LIBRARY(pqCore
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
  pqContextView.cxx
  pqContextView.h
  pqColorButtonEventPlayer.cxx
  pqColorButtonEventPlayer.h
  pqColorButtonEventTranslator.cxx
  pqColorButtonEventTranslator.h
  pqCommandServerStartup.cxx
  pqCommandServerStartup.h
  pqComparativeContextView.cxx
  pqComparativeContextView.h
  pqComparativeRenderView.cxx
  pqComparativeRenderView.h
  pqComparativeXYBarChartView.cxx
  pqComparativeXYBarChartView.h
  pqComparativeXYChartView.cxx
  pqComparativeXYChartView.h
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
  pqFileDialog.h
  pqFileDialogEventPlayer.cxx
  pqFileDialogEventPlayer.h
  pqFileDialogEventTranslator.cxx
  pqFileDialogEventTranslator.h
  pqFileDialogFavoriteModel.cxx
  pqFileDialogFavoriteModel.h
  pqFileDialogFilter.cxx
  pqFileDialogFilter.h
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
  pqLinksModel.cxx
  pqLinksModel.h
  pqLinkViewWidget.cxx
  pqLinkViewWidget.h
  pqLookupTableManager.cxx
  pqLookupTableManager.h
  pqManualServerStartup.cxx
  pqManualServerStartup.h
  pqNameCount.cxx
  pqNameCount.h
  pqOptions.cxx
  pqOptions.h
  pqObjectBuilder.cxx
  pqObjectBuilder.h
  pqOutputWindow.cxx
  pqOutputWindow.h
  pqOutputWindowAdapter.cxx
  pqOutputWindowAdapter.h
  pqOutputPort.cxx
  pqOutputPort.h
  pqParallelCoordinatesSettingsModel.cxx
  pqParallelCoordinatesSettingsModel.h
  pqParallelCoordinatesChartView.cxx
  pqParallelCoordinatesChartView.h
  pqPickHelper.cxx
  pqPickHelper.h
  pqPipelineFilter.cxx
  pqPipelineFilter.h
  pqPipelineSource.cxx
  pqPipelineSource.h
  pqPipelineRepresentation.cxx
  pqPipelineRepresentation.h
  pqPlotSettingsModel.cxx
  pqPlotSettingsModel.h
  pqPluginManager.cxx
  pqPluginManager.h
  pqProgressManager.cxx
  pqProgressManager.h
  pqPropertyLinks.cxx
  pqPropertyLinks.h
  pqPropertyManager.cxx
  pqPropertyManager.h
  pqProxy.cxx
  pqProxy.h
  pqProxyModifiedStateUndoElement.cxx
  pqProxyModifiedStateUndoElement.h
  pqProxyUnRegisterUndoElement.cxx
  pqProxyUnRegisterUndoElement.h
  pqQVTKWidget.cxx
  pqQVTKWidget.h
  pqQVTKWidgetEventPlayer.cxx
  pqQVTKWidgetEventPlayer.h
  pqQVTKWidgetEventTranslator.cxx
  pqQVTKWidgetEventTranslator.h
  pqRenderView.cxx
  pqRenderView.h
  pqRenderViewBase.cxx
  pqRenderViewBase.h
  pqRepresentation.cxx
  pqRepresentation.h
  pqRubberBandHelper.cxx
  pqRubberBandHelper.h
  pqSMAdaptor.cxx
  pqSMAdaptor.h
  pqSMProxy.cxx
  pqSMProxy.h
  pqScalarBarVisibilityAdaptor.cxx
  pqScalarBarVisibilityAdaptor.h
  pqScalarOpacityFunction.cxx
  pqScalarOpacityFunction.h
  pqScalarsToColors.cxx
  pqScalarsToColors.h
#  pqScatterPlotRepresentation.cxx
#  pqScatterPlotRepresentation.h
  pqServer.cxx
  pqServer.h
  pqServerManagerModel.cxx
  pqServerManagerModel.h
  pqServerManagerModelInterface.cxx
  pqServerManagerModelInterface.h
  pqServerManagerModelItem.cxx
  pqServerManagerModelItem.h
  pqServerManagerObserver.cxx
  pqServerManagerObserver.h
  pqServerManagerSelectionModel.cxx
  pqServerManagerSelectionModel.h
  pqServerResource.cxx
  pqServerResource.h
  pqServerResources.cxx
  pqServerResources.h
  pqServerStartup.h
  pqServerStartups.cxx
  pqServerStartups.h
  pqSettings.cxx
  pqSettings.h
  pqScalarBarRepresentation.cxx
  pqScalarBarRepresentation.h
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
  pqTableView.cxx
  pqTableView.h
  pqTextRepresentation.cxx
  pqTextRepresentation.h
  pqTimeKeeper.cxx
  pqTimeKeeper.h
  pqTreeLayoutStrategyInterface.cxx
  pqTreeLayoutStrategyInterface.h
  pqTwoDRenderView.cxx
  pqTwoDRenderView.h
#  pqScatterPlotView.cxx
#  pqScatterPlotView.h
  pqUndoStack.cxx
  pqUndoStack.h
  pqView.cxx
  pqView.h
  pqViewExporterManager.cxx
  pqViewExporterManager.h
  pqViewModuleInterface.cxx
  pqViewModuleInterface.h
  pqWriterFactory.cxx
  pqWriterFactory.h
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

TARGET_LINK_LIBRARIES(pqCore
  QtTesting
  pqWidgets
  QVTK
  vtkQtChart
  vtkCharts
  vtkPVServerManager
)

IF(VTK_USE_INFOVIS)
  TARGET_LINK_LIBRARIES(pqCore
    vtkInfovisCS
  )
ENDIF(VTK_USE_INFOVIS)

IF(QT_TESTING_WITH_PYTHON)
  TARGET_LINK_LIBRARIES(pqCore
    ${PYTHON_LIBRARIES}
  )
ENDIF(QT_TESTING_WITH_PYTHON)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pqCoreExport.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/pqCoreExport.h")
SET(dynamicHeaders 
    "${dynamicHeaders};${CMAKE_CURRENT_BINARY_DIR}/pqCoreExport.h")

INSTALL(TARGETS pqCore
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

