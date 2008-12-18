INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
)

SET(STREAMING_SS_SOURCES
  vtkPiece.cxx 
  vtkPieceCacheFilter.cxx
  vtkPieceCacheExecutive.cxx
  vtkPieceList.cxx
  vtkPVSGeometryInformation.cxx
  vtkRawStridedReader.cxx
  vtkSMSImageDataParallelStrategy.cxx
  vtkSMStreamingOptionsProxy.cxx
  vtkSMStreamingOutputPort.cxx
  vtkSMStreamingRepresentation.cxx 
  vtkSMStreamingSerialStrategy.cxx 
  vtkSMStreamingViewHelper.cxx
  vtkSMStreamingViewProxy.cxx
  vtkSMSUniformGridParallelStrategy.cxx
  vtkSMSUnstructuredDataParallelStrategy.cxx
  vtkSMSUnstructuredGridParallelStrategy.cxx
  vtkStreamingFactory.cxx
  vtkStreamingOptions.cxx
  vtkStreamingUpdateSuppressor.cxx
  vtkVisibilityPrioritizer.cxx
)

ADD_PARAVIEW_PLUGIN(StreamingSSPlugin "1.0"
  SERVER_MANAGER_XML StreamingServerManagerXML.xml 
  SERVER_MANAGER_SOURCES ${STREAMING_SS_SOURCES}
)

ADD_SUBDIRECTORY(Testing)
