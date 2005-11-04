
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

PROJECT(QtWidgets)

FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})

INCLUDE_DIRECTORIES(
  ${QtWidgets_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqCommandDispatcher.h
  pqCommandDispatcherManager.h
  pqExplicitCommandDispatcher.h
  pqFileDialog.h
  pqFileDialogModel.h
  pqLocalFileDialogModel.h
  pqObjectInspector.h
  pqObjectInspectorItem.h
  pqRefreshToolbar.h
  pqServerBrowser.h
  pqServerFileDialogModel.h
  pqSMAdaptor.h
  pqTimeoutCommandDispatcher.h
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  pqFileDialog.ui
  pqServerBrowser.ui
)

ADD_LIBRARY(QtWidgets
  pqCommandDispatcher.cxx
  pqCommandDispatcher.h
  pqCommandDispatcherManager.cxx
  pqCommandDispatcherManager.h
  pqCommand.h  
  pqExplicitCommandDispatcher.cxx
  pqExplicitCommandDispatcher.h
  pqFileDialog.cxx
  pqFileDialog.h
  pqFileDialogModel.cxx
  pqFileDialogModel.h
  pqImmediateCommandDispatcher.cxx
  pqImmediateCommandDispatcher.h
  pqLocalFileDialogModel.cxx
  pqLocalFileDialogModel.h
  pqObjectInspector.cxx
  pqObjectInspector.h
  pqObjectInspectorItem.cxx
  pqObjectInspectorItem.h
  pqOptions.cxx
  pqOptions.h
  pqParts.cxx
  pqParts.h
  pqRefreshToolbar.cxx
  pqRefreshToolbar.h
  pqRenderViewProxy.cxx
  pqRenderViewProxy.h
  pqServer.cxx
  pqServer.h
  pqServerBrowser.cxx
  pqServerBrowser.h
  pqServerFileDialogModel.cxx
  pqServerFileDialogModel.h
  pqSMAdaptor.cxx
  pqSMAdaptor.h
  pqTimeoutCommandDispatcher.cxx
  pqTimeoutCommandDispatcher.h
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(QtWidgets
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

  ${JPEG_LIBRARY}
  ${PNG_LIBRARY}
  ${FREETYPE_LIBRARY}
)


