cmake_minimum_required(VERSION 2.8)
IF( COMMAND CMAKE_POLICY )
  CMAKE_POLICY( SET CMP0003 NEW )
ENDIF( COMMAND CMAKE_POLICY )

PROJECT(vtkAdaptive)
INCLUDE ( ${CMAKE_ROOT}/Modules/FindVTK.cmake )
FIND_PACKAGE ( VTK REQUIRED )
INCLUDE ( ${VTK_USE_FILE} )

SET(vtkAdaptive_SOURCES
  vtkACosmoReader.cxx
  vtkAdaptiveOptions.cxx
  vtkGridSampler1.cxx
  vtkGridSampler2.cxx
  vtkIterativeStreamer.cxx
  vtkMetaInfoDatabase.cxx
  vtkMultiResolutionStreamer.cxx
  vtkPiece.cxx
  vtkPieceCacheExecutive.cxx
  vtkPieceCacheFilter.cxx
  vtkPieceList.cxx
  vtkPrioritizedStreamer.cxx
  vtkRawStridedReader1.cxx
  vtkStreamedMandelbrot.cxx
  vtkStreamingDriver.cxx
  vtkStreamingHarness.cxx
  vtkVisibilityPrioritizer.cxx
)

IF(NOT WIN32)
  SET(vtkAdaptive_SOURCES ${vtkAdaptive_SOURCES} vtkRawStridedReader2.cxx)
ENDIF(NOT WIN32)

ADD_LIBRARY(vtkAdaptive ${vtkAdaptive_SOURCES})

IF (BUILD_AGAINST_PARAVIEW)

  INCLUDE ( ${VTKCS_SOURCE_DIR}/vtkClientServer.cmake )
  VTK_WRAP_ClientServer ( ${PROJECT_NAME}CS vtkAdaptiveCS_SRCS "${HDRS}" )
  ADD_LIBRARY ( ${PROJECT_NAME}CS SHARED ${vtkAdaptiveCS_SRCS})
  IF (PV_INSTALL_PLUGIN_DIR)
    INSTALL(TARGETS ${PROJECT_NAME}CS
    DESTINATION "${PV_INSTALL_PLUGIN_DIR}"
    COMPONENT Runtime)
  ENDIF (PV_INSTALL_PLUGIN_DIR)

  TARGET_LINK_LIBRARIES( ${PROJECT_NAME}CS vtkAdaptive
    vtkClientServer vtkPVServerManager )

ENDIF (BUILD_AGAINST_PARAVIEW)

ADD_SUBDIRECTORY(Testing)
