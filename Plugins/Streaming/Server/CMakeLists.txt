INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
)

SET(STREAMING_SS_SOURCES
  vtkPiece.cxx 
  vtkPieceList.cxx
  vtkPieceCacheFilter.cxx
  vtkPieceCacheExecutive.cxx
  vtkPVSGeometryInformation.cxx
  vtkRawStridedReader.cxx
  vtkSMSImageDataParallelStrategy.cxx
  vtkSMStreamingHelperProxy.cxx
  vtkSMStreamingOutputPort.cxx
  vtkSMStreamingRepresentation.cxx 
  vtkSMStreamingSerialStrategy.cxx 
  vtkSMStreamingViewProxy.cxx 
  vtkSMStreamingViewHelper.cxx
  vtkSMSUniformGridParallelStrategy.cxx
  vtkSMSUnstructuredDataParallelStrategy.cxx
  vtkSMSUnstructuredGridParallelStrategy.cxx
  vtkStreamingFactory.cxx
  vtkStreamingUpdateSuppressor.cxx
  vtkVisibilityPrioritizer.cxx
)

ADD_PARAVIEW_PLUGIN(StreamingSSPlugin "1.0"
  SERVER_MANAGER_XML StreamingServerManagerXML.xml 
  SERVER_MANAGER_SOURCES ${STREAMING_SS_SOURCES}
)


ADD_SUBDIRECTORY(Testing)
