PROJECT(Client)

SET(QT_INCLUDE_DIR "c:\\qt\\4.0.1\\include" CACHE PATH "The top-level path to the Qt4 headers")
SET(QT_LIBRARIES_DIR "c:\\qt\\4.0.1\\lib" CACHE PATH "The path to the Qt4 libraries")
SET(QT_LIBRARIES qtcore4.lib qtgui4.lib qt3support4.lib)
SET(QT_MOC_EXECUTABLE "c:\\qt\\4.0.1\\bin\\moc.exe" CACHE FILEPATH "The Qt MOC binary")

INCLUDE_DIRECTORIES(
  ${ParaQ_BINARY_DIR}/ParaView
  ${ParaQ_BINARY_DIR}/ParaView/VTK
  ${ParaQ_BINARY_DIR}/ParaView/Servers/Common
  ${ParaQ_BINARY_DIR}/ParaView/Servers/Filters
  ${ParaQ_BINARY_DIR}/ParaView/Servers/ServerManager
  ${ParaQ_BINARY_DIR}/ParaView/Utilities/VTKClientServer
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Common
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Filtering
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/GenericFiltering
  ${ParaQ_SOURCE_DIR}/ParaView/VTK/Rendering
  ${ParaQ_SOURCE_DIR}/ParaView/GUI/Client
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Common
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/Filters
  ${ParaQ_SOURCE_DIR}/ParaView/Servers/ServerManager
  ${ParaQ_SOURCE_DIR}/ParaView/Utilities/VTKClientServer
  ${QT_INCLUDE_DIR}
  ${QT_INCLUDE_DIR}/QtCore
  ${QT_INCLUDE_DIR}/QtGui
  ${QT_INCLUDE_DIR}/Qt3Support
)

SET(MOC_SOURCES
  pqMainWindow.h
  pqServerFileBrowser.h
  pqServerFileBrowserList.h
)

QT_WRAP_CPP(pqClient MOC_OUTPUT
  MOC_SOURCES
)

ADD_DEFINITIONS(
  -DQT3_SUPPORT
)

ADD_EXECUTABLE(pqClient
  main.cxx
  pqApplication.cxx
  pqApplication.h
  pqMainWindow.cxx
  pqMainWindow.h
  pqOptions.cxx
  pqOptions.h
  pqProcessModuleGUIHelper.cxx
  pqProcessModuleGUIHelper.h
  pqServer.cxx
  pqServer.h
  pqServerFileBrowser.cxx
  pqServerFileBrowser.h
  pqServerFileBrowserList.cxx
  pqServerFileBrowserList.h
  ${MOC_OUTPUT}
)

LINK_DIRECTORIES(pqClient
  ${QT_LIBRARIES_DIR}
)

TARGET_LINK_LIBRARIES(pqClient
  OpenGL32.lib
  SPCTH.lib
  Vfw32.lib
  Xdmf.lib
  kwcommon.lib
  vtkClientServer.lib
  vtkCommon.lib
  vtkCommonCS.lib
  vtkDICOMParser.lib
  vtkFiltering.lib
  vtkFilteringCS.lib
  vtkGenericFiltering.lib
  vtkGenericFilteringCS.lib
  vtkGraphics.lib
  vtkGraphicsCS.lib
  vtkHybrid.lib
  vtkHybridCS.lib
  vtkIO.lib
  vtkIOCS.lib
  vtkImaging.lib
  vtkImagingCS.lib
  vtkMPEG2Encode.lib
  vtkNetCDF.lib
  vtkPVFilters.lib
  vtkPVFiltersCS.lib
  vtkPVServerCommon.lib
  vtkPVServerCommonCS.lib
  vtkPVServerManager.lib
  vtkParallel.lib
  vtkParallelCS.lib
  vtkRendering.lib
  vtkRenderingCS.lib
  vtkSys.lib
  vtkVolumeRendering.lib
  vtkVolumeRenderingCS.lib
  vtkWidgets.lib
  vtkWidgetsCS.lib
  vtkXdmf.lib
  vtkXdmfCS.lib
  vtkexoIIc.lib
  vtkexpat.lib
  vtkfreetype.lib
  vtkftgl.lib
  vtkhdf5.lib
  vtkimaging.lib
  vtkjpeg.lib
  vtkparallel.lib
  vtkpng.lib
  vtktiff.lib
  vtkzlib.lib
  wsock32.lib
  ${QT_LIBRARIES}
)
