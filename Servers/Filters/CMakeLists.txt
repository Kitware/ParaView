PROJECT(PVFilters)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(Filters_SRCS
  vtkCleanUnstructuredGrid.cxx
  vtkColorByPart.cxx
  vtkGroup.cxx
  vtkHDF5RawImageReader.cxx
  vtkMPIDuplicatePolyData.cxx
  vtkMazeSource.cxx
  vtkMergeArrays.cxx
  vtkPVArrowSource.cxx
  vtkPVClipDataSet.cxx
  vtkPVConnectivityFilter.cxx
  vtkPVContourFilter.cxx
  vtkPVEnSightMasterServerReader.cxx
  vtkPVEnSightMasterServerTranslator.cxx
  vtkPVExtractVOI.cxx
  vtkPVGlyphFilter.cxx
  vtkPVImageContinuousDilate3D.cxx
  vtkPVImageContinuousErode3D.cxx
  vtkPVImageGradient.cxx
  vtkPVImageGradientMagnitude.cxx
  vtkPVImageMedian3D.cxx
  vtkPVLinearExtrusionFilter.cxx
  vtkPVRibbonFilter.cxx
  vtkPVThresholdFilter.cxx
  vtkPVUpdateSuppressor.cxx
  vtkPVWarpScalar.cxx
  vtkPVWarpVector.cxx
  vtkStructuredCacheFilter.cxx
  vtkTiledDisplaySchedule.cxx
  vtkVRMLSource.cxx
  vtkXMLPVDWriter.cxx
  )

IF(VTK_USE_MPI)
  SET(Filters_SRCS ${Filters_SRCS}
      #vtkBalancedRedistributePolyData.cxx
      vtkAllToNRedistributePolyData.cxx
      vtkBoxClipDataSet.cxx
      vtkDistributedDataFilter.cxx
      vtkExtractCells.cxx
      vtkExtractUserDefinedPiece.cxx
      vtkKdTree.cxx
      vtkMergeCells.cxx
      vtkPKdTree.cxx
      vtkPlanesIntersection.cxx
      vtkPointsProjectedHull.cxx
      vtkPVDuplicatePolyData.cxx
      vtkRedistributePolyData.cxx
      vtkWeightedRedistributePolyData.cxx
    )
ENDIF(VTK_USE_MPI)

IF (VTK_USE_PATENTED)
  SET(Filters_SRCS ${Filters_SRCS} vtkPVKitwareContourFilter.cxx)
ENDIF (VTK_USE_PATENTED)

SET(VTK_BINARY_DIR "${PVFilters_BINARY_DIR}")
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkExportKit.cmake)
VTK_EXPORT_KIT("PVFilters" "PVFILTERS" "${Filters_SRCS}")

INCLUDE_DIRECTORIES(${PVFilters_BINARY_DIR})
CONFIGURE_FILE(${PVFilters_SOURCE_DIR}/vtkPVFiltersConfig.h.in
  ${PVFilters_BINARY_DIR}/vtkPVFiltersConfig.h
  ESCAPE_QUOTES)

# Create filters library.
ADD_LIBRARY(vtkPVFilters ${Filters_SRCS})
TARGET_LINK_LIBRARIES(vtkPVFilters vtkKWCommon vtkHybrid vtkParallel)
IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtkPVFilters vtkPatented)
ENDIF (VTK_USE_PATENTED)
TARGET_LINK_LIBRARIES(vtkPVFilters ${PARAVIEW_HDF5_LIBRARIES})

ADD_EXECUTABLE(vtkTestFilters test.cxx)
TARGET_LINK_LIBRARIES(vtkTestFilters vtkPVFilters)

# load the ClientServer command
INCLUDE(${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/vtkClientServer.cmake)
CS_INITIALIZE_WRAP()
INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  )

# Wrap PVFilters
INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/Servers/Filters
  )
SET(KIT_WRAP_DEPS Parallel)
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/Servers/Filters/Utilities")
PV_WRAP_VTK_CS(PVFilters PVFILTERS "${KIT_WRAP_DEPS}")

