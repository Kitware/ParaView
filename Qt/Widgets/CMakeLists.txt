
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

PROJECT(QtWidgets)

FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqFileDialog.h
  pqFileDialogModel.h
  pqLocalFileDialogModel.h
  pqObjectInspector.h
  pqObjectInspectorItem.h
  pqServerBrowser.h
  pqServerFileDialogModel.h
  pqSMAdaptor.h
  pqCommandDispatcher.h
  pqRefreshToolbar.h
  pqCommandDispatcherManager.h
  pqTimeoutCommandDispatcher.h
  pqExplicitCommandDispatcher.h
)

ADD_LIBRARY(pqWidgets
  pqFileDialog.cxx
  pqFileDialog.h
  pqFileDialogModel.cxx
  pqFileDialogModel.h
  pqLocalFileDialogModel.cxx
  pqLocalFileDialogModel.h
  pqObjectInspector.cxx
  pqObjectInspector.h
  pqObjectInspectorItem.cxx
  pqObjectInspectorItem.h
  pqRenderViewProxy.cxx
  pqRenderViewProxy.h
  pqServer.cxx
  pqServer.h
  pqOptions.cxx
  pqOptions.h
  pqServerBrowser.cxx
  pqServerBrowser.h
  pqServerFileDialogModel.cxx
  pqServerFileDialogModel.h
  pqSMAdaptor.cxx
  pqSMAdaptor.h
  pqParts.cxx
  pqParts.h
  pqCommandDispatcher.cxx
  pqCommandDispatcher.h
  pqImmediateCommandDispatcher.cxx
  pqImmediateCommandDispatcher.h
  pqTimeoutCommandDispatcher.cxx
  pqTimeoutCommandDispatcher.h
  pqExplicitCommandDispatcher.cxx
  pqExplicitCommandDispatcher.h
  pqRefreshToolbar.cxx
  pqRefreshToolbar.h
  pqCommandDispatcherManager.cxx
  pqCommandDispatcherManager.h
  pqCommand.h  
  ${MOC_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(pqWidgets
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


