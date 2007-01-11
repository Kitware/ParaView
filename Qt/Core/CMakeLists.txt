PROJECT(pqCore)

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtChart_SOURCE_DIR}
  ${QtChart_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${QtTesting_BINARY_DIR}
  ${pqCore_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  ${PYTHON_INCLUDE_PATH}
)

IF(QT_TESTING_WITH_PYTHON)
  SET(PYTHON_CORE_SRCS 
    pqPythonEventSourceImage.cxx
    pqPythonEventSourceImage.h
  )
  SET(PYTHON_MOC_SRCS 
    pqPythonEventSourceImage.h
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
  pqCommandServerStartup.h
  pqConsumerDisplay.h
  pqCoreTestUtility.h
  pqDisplay.h
  pqDisplayPolicy.h
  pqFileDialog.h
  pqFileDialogEventTranslator.h
  pqFileDialogFavoriteModel.h
  pqFileDialogFilter.h
  pqFileDialogModel.h
  pqFlatTreeViewEventTranslator.h
  pqGenericViewModule.h
  pqHistogramTableModel.h
  pqLinksModel.h
  pqLookupTableManager.h
  pqOutputWindow.h
  pqOutputWindowAdapter.h
  pqPendingDisplayManager.h
  pqPipelineBuilder.h
  pqPipelineDisplay.h
  pqPipelineSource.h
  pqPipelineFilter.h
  pqPlotViewModule.h
  pqPropertyLinks.h
  pqPropertyManager.h
  pqProxy.h
  pqQVTKWidgetEventTranslator.h
  pqRenderViewModule.h
  pqScalarBarDisplay.h
  pqScalarBarVisibilityAdaptor.h
  pqScalarsToColors.h
  pqServer.h
  pqServerManagerModel.h
  pqServerManagerModelItem.h
  pqServerManagerObserver.h
  pqServerManagerSelectionModel.h
  pqServerResources.h
  pqServerStartupContext.h
  pqServerStartups.h
  pqSettings.h
  pqTableViewModule.h
  pqTextWidgetDisplay.h
  pqUndoStack.h
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
  Resources/pqCore.qrc
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
  pqApplicationCore.cxx
  pqApplicationCore.h
  pqCommandServerStartup.cxx
  pqCommandServerStartup.h
  pqConsumerDisplay.cxx
  pqConsumerDisplay.h
  pqCoreExport.h
  pqCoreInit.cxx
  pqCoreInit.h
  pqCoreTestUtility.cxx
  pqCoreTestUtility.h
  pqDisplay.cxx
  pqDisplay.h
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
  pqFlatTreeViewEventPlayer.cxx
  pqFlatTreeViewEventPlayer.h
  pqFlatTreeViewEventTranslator.cxx
  pqFlatTreeViewEventTranslator.h
  pqGenericViewModule.cxx
  pqGenericViewModule.h
  pqHistogramTableModel.cxx
  pqHistogramTableModel.h
  pqJoystickFly.cxx
  pqJoystickFly.h
  pqLinksModel.cxx
  pqLinksModel.h
  pqLookupTableManager.cxx
  pqLookupTableManager.h
  pqMain.cxx
  pqMain.h
  pqManualServerStartup.cxx
  pqManualServerStartup.h
  pqNameCount.cxx
  pqNameCount.h
  pqOptions.cxx
  pqOptions.h
  pqOutputWindow.cxx
  pqOutputWindow.h
  pqOutputWindowAdapter.cxx
  pqOutputWindowAdapter.h
  pqPendingDisplayManager.cxx
  pqPendingDisplayManager.h
  pqPendingDisplayUndoElement.cxx
  pqPendingDisplayUndoElement.h
  pqPipelineBuilder.cxx
  pqPipelineBuilder.h
  pqPipelineDisplay.cxx
  pqPipelineDisplay.h
  pqPipelineFilter.cxx
  pqPipelineFilter.h
  pqPipelineSource.cxx
  pqPipelineSource.h
  pqPlotViewModule.cxx
  pqPlotViewModule.h
  pqProcessModuleGUIHelper.cxx
  pqProcessModuleGUIHelper.h
  pqPropertyLinks.cxx
  pqPropertyLinks.h
  pqPropertyManager.cxx
  pqPropertyManager.h
  pqProxy.cxx
  pqProxy.h
  pqQVTKWidgetEventPlayer.cxx
  pqQVTKWidgetEventPlayer.h
  pqQVTKWidgetEventTranslator.cxx
  pqQVTKWidgetEventTranslator.h
  pqReaderFactory.cxx
  pqReaderFactory.h
  pqRenderViewModule.cxx
  pqRenderViewModule.h
  pqRenderViewProxy.cxx
  pqRenderViewProxy.h
  pqSMAdaptor.cxx
  pqSMAdaptor.h
  pqSMProxy.cxx
  pqSMProxy.h
  pqScalarBarDisplay.cxx
  pqScalarBarDisplay.h
  pqScalarBarVisibilityAdaptor.cxx
  pqScalarBarVisibilityAdaptor.h
  pqScalarsToColors.cxx
  pqScalarsToColors.h
  pqServer.cxx
  pqServer.h
  pqServerManagerModel.cxx
  pqServerManagerModel.h
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
  pqServerStartupContext.cxx
  pqServerStartupContext.h
  pqServerStartups.cxx
  pqServerStartups.h
  pqSettings.cxx
  pqSettings.h
  pqTableViewModule.cxx
  pqTableViewModule.h
  pqTextWidgetDisplay.cxx
  pqTextWidgetDisplay.h
  pqUndoRedoStateLoader.cxx
  pqUndoRedoStateLoader.h
  pqUndoStack.cxx
  pqUndoStack.h
  pqVTKHistogramModel.cxx
  pqVTKHistogramModel.h
  pqVTKLineChartModel.cxx
  pqVTKLineChartModel.h
  pqVTKLineChartPlot.cxx
  pqVTKLineChartPlot.h
  pqWriterFactory.cxx
  pqWriterFactory.h
  pqXMLUtil.cxx
  pqXMLUtil.h

  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}

  ${PYTHON_CORE_SRCS}

  vtkPVAxesWidget.cxx
  vtkPVAxesWidget.h
  vtkPVAxesActor.cxx
  vtkPVAxesActor.h
)

TARGET_LINK_LIBRARIES(pqCore
  QtChart
  QtTesting
  QtWidgets
  vtkPVServerManager
  QVTK
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


INSTALL(TARGETS pqCore
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

