PROJECT(QtWidgets)

CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

FIND_PACKAGE(Qt4)
SET(QT_USE_QTDESIGNER ON)
INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${QtWidgets_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${ParaQ_SOURCE_DIR}/Qt/Chart
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Graphics
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Hybrid
)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqCompoundProxyWizard.h
  pqConsoleWidget.h
  pqDataSetModel.h
  pqElementInspectorWidget.h
  pqFileDialog.h
  pqFileDialogModel.h
  pqImageTip.h
  pqLocalFileDialogModel.h
  pqMultiView.h
  pqMultiViewFrame.h
  pqMultiViewManager.h
  pqObjectHistogramWidget.h
  pqObjectLineChartWidget.h
  pqObjectInspector.h
  pqObjectInspectorItem.h
  pqObjectInspectorWidget.h
  pqPicking.h
  pqPipelineData.h
  pqPipelineListModel.h
  pqPipelineListWidget.h
  pqPlayControlsWidget.h
  pqServerBrowser.h
  pqServerFileDialogModel.h
  pqSMAdaptor.h
  pqVariableSelectorWidget.h
)

SET(UI_FORMS
  pqCompoundProxyWizard.ui
  pqFileDialog.ui
  pqMultiViewFrameMenu.ui
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

ADD_LIBRARY(QtWidgets
  pqCompoundProxyWizard.cxx
  pqCompoundProxyWizard.h
  pqConnect.cxx
  pqConnect.h
  pqConsoleWidget.cxx
  pqConsoleWidget.h
  pqDataSetModel.cxx
  pqDataSetModel.h
  pqElementInspectorWidget.cxx
  pqElementInspectorWidget.h
  pqFileDialog.cxx
  pqFileDialog.h
  pqFileDialogModel.cxx
  pqFileDialogModel.h
  pqImageTip.cxx
  pqImageTip.h
  pqLocalFileDialogModel.cxx
  pqLocalFileDialogModel.h
  pqMultiView.cxx
  pqMultiView.h
  pqMultiViewFrame.cxx
  pqMultiViewFrame.h
  pqMultiViewManager.cxx
  pqMultiViewManager.h
  pqNameCount.cxx
  pqNameCount.h
  pqObjectHistogramWidget.cxx
  pqObjectHistogramWidget.h
  pqObjectInspector.cxx
  pqObjectInspector.h
  pqObjectInspectorDelegate.cxx
  pqObjectInspectorDelegate.h
  pqObjectInspectorItem.cxx
  pqObjectInspectorItem.h
  pqObjectInspectorWidget.cxx
  pqObjectInspectorWidget.h
  pqObjectLineChartWidget.cxx
  pqObjectLineChartWidget.h
  pqOptions.cxx
  pqOptions.h
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
  pqPlayControlsWidget.cxx
  pqPlayControlsWidget.h
  pqRenderViewProxy.cxx
  pqRenderViewProxy.h
  pqServer.cxx
  pqServer.h
  pqServerBrowser.cxx
  pqServerBrowser.h
  pqServerFileDialogModel.cxx
  pqServerFileDialogModel.h
  pqSetData.cxx
  pqSetData.h
  pqSetName.cxx
  pqSetName.h
  pqSMAdaptor.cxx
  pqSMAdaptor.h
  pqSMMultiView.cxx
  pqSMMultiView.h
  pqSourceProxyInfo.cxx
  pqSourceProxyInfo.h
  pqVariableSelectorWidget.cxx
  pqVariableSelectorWidget.h
  pqVariableType.h
  pqXMLUtil.cxx
  pqXMLUtil.h
  QtWidgetsExport.h
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}

  vtkPVAxesWidget.cxx
  vtkPVAxesWidget.h
  vtkPVAxesActor.cxx
  vtkPVAxesActor.h
)

TARGET_LINK_LIBRARIES(QtWidgets
  pqChart
  vtkPVServerManager
  vtkPVFiltersCS
  vtkPVFilters
  vtkPVServerCommonCS
  vtkPVServerCommon
  vtkParallelCS
  vtkWidgetsCS
  vtkHybridCS
  vtkVolumeRenderingCS
  vtkXdmfCS
  vtkRenderingCS
  vtkGenericFilteringCS
  vtkGenericFiltering
  vtkIOCS
  vtkImagingCS
  vtkGraphicsCS
  vtkFilteringCS
  vtkCommonCS
  vtkClientServer
  SPCTH

  QVTK

  vtkParallel
  vtkHybrid
  vtkIO
  vtkVolumeRendering
  vtkWidgets
  vtkImaging
  vtkRendering
  vtkGraphics
  vtkFiltering
  vtkCommon
  vtkXdmf
  Xdmf
  vtkexoIIc
  vtkNetCDF
  vtkexpat
  vtkftgl
  vtkhdf5
  vtktiff
  vtkzlib
  vtkMPEG2Encode
  vtkDICOMParser

  ${QT_LIBRARIES}
)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)


