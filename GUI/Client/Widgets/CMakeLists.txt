PROJECT(pqWidgets)

IF(PARAQ_EMBED_PYTHON)
  FIND_PACKAGE(PythonLibs)

  SET(PYTHON_LINK_LIBRARIES
    pqPython
    QtPython
    vtkPVServerCommonPythonD
    vtkPVServerManagerPythonD
    ${PYTHON_LIBRARIES}
  )
ENDIF(PARAQ_EMBED_PYTHON)

INCLUDE_DIRECTORIES(
  ${QtPython_BINARY_DIR}
  ${QtPython_SOURCE_DIR}
  ${pqWidgets_BINARY_DIR}
  ${pqWidgets_SOURCE_DIR}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqCompoundProxyWizard.h
  pqDataSetModel.h
  pqDisplayProxyEditor.h
  pqElementInspectorWidget.h
  pqImageTip.h
  pqMainWindow.h
  pqMultiView.h
  pqMultiViewFrame.h
  pqMultiViewManager.h
  pqObjectEditor.h
  pqObjectInspector.h
  pqObjectInspectorItem.h
  pqObjectInspectorWidget.h
  pqOutputWindow.h
  pqOutputWindowAdapter.h
  pqPicking.h
  pqPipelineData.h
  pqPipelineListModel.h
  pqPipelineListWidget.h
  pqPropertyLinks.h
  pqPropertyManager.h
  pqRenderViewEditor.h
  pqSMAdaptor.h
  pqSMSignalAdaptors.h
  pqServerBrowser.h
  pqServerFileDialogModel.h
  pqVariableSelectorWidget.h
)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SET(UI_FORMS
  pqCompoundProxyWizard.ui
  pqDisplayProxyEditor.ui
  pqMultiViewFrameMenu.ui
  pqOutputWindow.ui
  pqRenderViewEditor.ui
  pqServerBrowser.ui
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
)

SET(UI_RESOURCES
  pqWidgets.qrc
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

ADD_LIBRARY(pqWidgets
  pqCompoundProxyWizard.cxx
  pqCompoundProxyWizard.h
  pqDataSetModel.cxx
  pqDataSetModel.h
  pqDisplayProxyEditor.cxx
  pqDisplayProxyEditor.h
  pqElementInspectorWidget.cxx
  pqElementInspectorWidget.h
  pqImageTip.cxx
  pqImageTip.h
  pqMainWindow.cxx
  pqMainWindow.h
  pqMultiView.cxx
  pqMultiView.h
  pqMultiViewFrame.cxx
  pqMultiViewFrame.h
  pqMultiViewManager.cxx
  pqMultiViewManager.h
  pqNameCount.cxx
  pqNameCount.h
  pqObjectEditor.cxx
  pqObjectEditor.h
  pqObjectInspector.cxx
  pqObjectInspector.h
  pqObjectInspectorItem.cxx
  pqObjectInspectorItem.h
  pqObjectInspectorWidget.cxx
  pqObjectInspectorWidget.h
  pqOptions.cxx
  pqOptions.h
  pqOutputWindow.cxx
  pqOutputWindow.h
  pqOutputWindowAdapter.cxx
  pqOutputWindowAdapter.h
  pqParts.cxx
  pqParts.h
  pqPicking.cxx
  pqPicking.h
  pqPipelineData.cxx
  pqPipelineData.h
  pqPipelineListModel.cxx
  pqPipelineListModel.h
  pqPipelineListWidget.cxx
  pqPipelineListWidget.h
  pqPipelineObject.cxx
  pqPipelineObject.h
  pqPipelineServer.cxx
  pqPipelineServer.h
  pqPipelineWindow.cxx
  pqPipelineWindow.h
  pqProcessModuleGUIHelper.cxx
  pqProcessModuleGUIHelper.h
  pqRenderViewProxy.cxx
  pqRenderViewProxy.h
  pqRenderViewEditor.cxx
  pqRenderViewEditor.h
  pqPropertyLinks.cxx
  pqPropertyLinks.h
  pqPropertyManager.cxx
  pqPropertyManager.h
  pqSMAdaptor.cxx
  pqSMAdaptor.h
  pqSMMultiView.cxx
  pqSMMultiView.h
  pqSMProxy.cxx
  pqSMProxy.h
  pqSMSignalAdaptors.cxx
  pqSMSignalAdaptors.h
  pqServer.cxx
  pqServer.h
  pqServerBrowser.cxx
  pqServerBrowser.h
  pqServerFileDialogModel.cxx
  pqServerFileDialogModel.h
  pqSourceProxyInfo.cxx
  pqSourceProxyInfo.h
  pqVariableSelectorWidget.cxx
  pqVariableSelectorWidget.h
  pqVariableType.h
  pqWidgetsExport.h
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

TARGET_LINK_LIBRARIES(pqWidgets
  QtChart
  QtTesting
  QtWidgets
  vtkPVServerManager
  QVTK
  ${PYTHON_LINK_LIBRARIES}
)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

