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
set_source_files_properties(
  vtkPiece
  WRAP_EXCLUDE)

IF(NOT WIN32)
  SET(vtkStreaming_SOURCES ${vtkStreaming_SOURCES} vtkRawStridedReader2.cxx)
ENDIF(NOT WIN32)

set (${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${vtkStreaming_SOURCES})
