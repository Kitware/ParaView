PROJECT(pqCore)

IF(PARAVIEW_EMBED_PYTHON)
  FIND_PACKAGE(PythonLibs)

  SET(PYTHON_LINK_LIBRARIES
    vtkPVPythonInterpretor
    QtPython
    vtkPVServerCommonPythonD
    vtkPVServerManagerPythonD
    ${PYTHON_LIBRARIES}
  )
ENDIF(PARAVIEW_EMBED_PYTHON)

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
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pq3DWidgetFactory.h
  pqApplicationCore.h
  pqCommandServerStartup.h
  pqFileDialogEventTranslator.h
  pqOutputWindow.h
  pqOutputWindowAdapter.h
  pqPendingDisplayManager.h
  pqPipelineBuilder.h
  pqPipelineDisplay.h
  pqPipelineModel.h
  pqPipelineSource.h
  pqPropertyLinks.h
  pqPropertyManager.h
  pqProxy.h
  pqRenderModule.h
  pqServer.h
  pqServerManagerModel.h
  pqServerManagerModelItem.h
  pqServerManagerObserver.h
  pqServerManagerSelectionModel.h
  pqServerResources.h
  pqServerStartupContext.h
  pqServerStartups.h
  pqSettings.h
  pqTestUtility.h
  pqUndoStack.h
  )
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SET(UI_FORMS
  Resources/UI/pqOutputWindow.ui
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
  pqApplicationCore.cxx
  pqApplicationCore.h
  pqCommandServerStartup.cxx
  pqCommandServerStartup.h
  pqCoreExport.h
  pqFileDialogEventPlayer.cxx
  pqFileDialogEventPlayer.h
  pqFileDialogEventTranslator.cxx
  pqFileDialogEventTranslator.h
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
  pqPipelineModel.cxx
  pqPipelineModel.h
  pqPipelineSource.cxx
  pqPipelineSource.h
  pqProcessModuleGUIHelper.cxx
  pqProcessModuleGUIHelper.h
  pqPropertyLinks.cxx
  pqPropertyLinks.h
  pqPropertyManager.cxx
  pqPropertyManager.h
  pqProxy.cxx
  pqProxy.h
  pqReaderFactory.cxx
  pqReaderFactory.h
  pqRenderModule.cxx
  pqRenderModule.h
  pqRenderViewProxy.cxx
  pqRenderViewProxy.h
  pqSMAdaptor.cxx
  pqSMAdaptor.h
  pqSMProxy.cxx
  pqSMProxy.h
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
  pqTestUtility.cxx
  pqTestUtility.h
  pqUndoRedoStateLoader.cxx
  pqUndoRedoStateLoader.h
  pqUndoStack.cxx
  pqUndoStack.h
  pqWriterFactory.cxx
  pqWriterFactory.h
  pqXMLUtil.cxx
  pqXMLUtil.h
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}

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
  ${PYTHON_LINK_LIBRARIES}
)


INSTALL(TARGETS pqCore
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

