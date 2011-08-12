PROJECT(vtkStreaming)

cmake_minimum_required(VERSION 2.8)
IF( COMMAND CMAKE_POLICY )
  CMAKE_POLICY( SET CMP0003 NEW )
ENDIF( COMMAND CMAKE_POLICY )

INCLUDE ( ${CMAKE_ROOT}/Modules/FindVTK.cmake )
FIND_PACKAGE ( VTK REQUIRED )
INCLUDE ( ${VTK_USE_FILE} )

SET ( VTK_LINK_LIBRARIES
  vtkRendering vtkGraphics vtkFiltering vtkCommon vtkIO vtkImaging vtkNetCDF)

INCLUDE_DIRECTORIES(${VTKNETCDF_BINARY_DIR})

SET(vtkStreaming_SOURCES
  vtkACosmoReader.cxx
  vtkAdaptiveOptions.cxx
  vtkGridSampler1.cxx
  vtkGridSampler2.cxx
  vtkImageNetCDFPOPReader.cxx
  vtkIterativeStreamer.cxx
  vtkMetaInfoDatabase.cxx
  vtkMultiResolutionStreamer.cxx
  vtkParallelStreamHelper.cxx
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
  vtkWarpScalarsAndMetaInfo.cxx
  vtkWorldWarp.cxx
)

IF(NOT WIN32)
  SET(vtkStreaming_SOURCES ${vtkStreaming_SOURCES} vtkRawStridedReader2.cxx)
ENDIF(NOT WIN32)

ADD_LIBRARY(vtkStreaming ${vtkStreaming_SOURCES})

TARGET_LINK_LIBRARIES( vtkStreaming ${VTK_LINK_LIBRARIES} )

IF (BUILD_AGAINST_PARAVIEW)

  INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    )

  SET(HDRS)
  SET(REALSRCS)
  SET(INST_SRCS)
  FOREACH(SRC
      vtkACosmoReader
      vtkImageNetCDFPOPReader
      vtkIterativeStreamer
      vtkMultiResolutionStreamer
      vtkParallelStreamHelper
      vtkPrioritizedStreamer
      vtkPieceCacheExecutive
      vtkPieceCacheFilter
      vtkPieceList
      vtkRawStridedReader1
      vtkStreamedMandelbrot
      vtkStreamingDriver
      vtkStreamingHarness
      vtkWarpScalarsAndMetaInfo
      vtkWorldWarp
      )
    GET_FILENAME_COMPONENT(src_name "${SRC}" NAME_WE)
    GET_FILENAME_COMPONENT(src_path "${SRC}" ABSOLUTE)
    GET_FILENAME_COMPONENT(src_path "${src_path}" PATH)
    SET(HDR)
    IF(EXISTS "${src_path}/${src_name}.h")
      SET(HDR "${src_path}/${src_name}.h")
    ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${src_name}.h")
      SET(HDR "${CMAKE_CURRENT_BINARY_DIR}/${src_name}.h")
    ENDIF()
    LIST(APPEND HDRS ${HDR})
    IF(NOT HDR MATCHES ${SRC})
      SET(REALSRCS ${REALSRCS} ${SRC})
    ENDIF(NOT HDR MATCHES ${SRC})
  ENDFOREACH(SRC ${ARGN})

  SET ( VTK_USE_INSTANTIATOR_NEW 1 )

  VTK_MAKE_INSTANTIATOR3 (${PROJECT_NAME}Instantiator
    vtkStreamingInstantiator_SRCS
    "${HDRS}"
    "VTK_${PROJECT_NAME}_EXPORT"
    ${PROJECT_BINARY_DIR}
    "${PROJECT_NAME}Configure.h"
    )

  SET ( VTK_LINK_LIBRARIES ${VTK_LINK_LIBRARIES} vtkPVFilters)

  INCLUDE ( ${VTKCS_SOURCE_DIR}/vtkClientServer.cmake )
  VTK_WRAP_ClientServer ( ${PROJECT_NAME}CS vtkStreamingCS_SRCS "${HDRS}" )
  ADD_LIBRARY ( ${PROJECT_NAME}CS ${vtkStreamingCS_SRCS})
  
  INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
   
  IF (PV_INSTALL_PLUGIN_DIR)
    INSTALL(TARGETS ${PROJECT_NAME}CS
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
  ENDIF (PV_INSTALL_PLUGIN_DIR)

  TARGET_LINK_LIBRARIES( ${PROJECT_NAME}CS vtkStreaming
    vtkClientServer vtkPVServerManager )

ENDIF (BUILD_AGAINST_PARAVIEW)

IF (BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
ENDIF (BUILD_TESTING)
