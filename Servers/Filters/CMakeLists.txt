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
