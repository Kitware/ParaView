cmake_minimum_required(VERSION 2.8)

PROJECT(AdaptiveVTK)
FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

SET(SOURCES
  vtkACosmoReader.cxx
  vtkAdaptiveOptions.cxx
  vtkGridSampler1.cxx
  vtkGridSampler2.cxx
  vtkMetaInfoDatabase.cxx
  vtkPiece.cxx
  vtkPieceCacheExecutive.cxx
  vtkPieceCacheFilter.cxx
  vtkPieceList.cxx
  vtkRawStridedReader1.cxx
  vtkStreamAlgorithm.cxx
  vtkStreamedMandelbrot.cxx
  vtkStreamingDriver.cxx
  vtkVisibilityPrioritizer.cxx
)

IF(NOT WIN32)
  SET(SOURCES ${SOURCES} vtkRawStridedReader2.cxx)
ENDIF(NOT WIN32)

ADD_LIBRARY(AdaptiveVTK ${SOURCES})
ADD_EXECUTABLE(AdaptiveStreaming AdaptiveMain.cxx)
TARGET_LINK_LIBRARIES(AdaptiveStreaming AdaptiveVTK vtkRendering vtkFiltering vtkGraphics vtkCommon vtkImaging)
