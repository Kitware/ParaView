PROJECT(Filters)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(Filters_SRCS
  vtkBoxClipDataSet.cxx
  vtkCleanUnstructuredGrid.cxx
  vtkColorByPart.cxx
  vtkGroup.cxx
  vtkKWExtractGeometryByScalar.cxx
  vtkMergeArrays.cxx
  vtkMultiOut2.cxx
  vtkMultiOut3.cxx
  vtkMultiOut.cxx
  vtkPVArrowSource.cxx
  vtkPVClipDataSet.cxx
  vtkPVConnectivityFilter.cxx
  vtkPVContourFilter.cxx
  vtkPVEnSightMasterServerReader.cxx
  vtkPVEnSightMasterServerTranslator.cxx
  vtkPVGlyphFilter.cxx
  vtkPVImageContinuousDilate3D.cxx
  vtkPVImageContinuousErode3D.cxx
  vtkPVImageGradient.cxx
  vtkPVImageGradientMagnitude.cxx
  vtkPVImageMedian3D.cxx
  vtkPVRibbonFilter.cxx
  vtkPVThresholdFilter.cxx
  vtkPVUpdateSuppressor.cxx
  vtkPVWarpScalar.cxx
  vtkPVWarpVector.cxx
  vtkStructuredCacheFilter.cxx
  vtkVRMLSource.cxx
  vtkXMLPVDWriter.cxx
  )

IF(VTK_USE_MPI)
  SET(Filters_SRCS
    ${Filters_SRCS}
    vtkAllToNRedistributePolyData.cxx
    #vtkBalancedRedistributePolyData.cxx
    vtkDistributedDataFilter.cxx
    vtkExtractCells.cxx
    vtkKdTree.cxx
    vtkMergeCells.cxx
    vtkPKdTree.cxx
    vtkPlanesIntersection.cxx
    vtkPointsProjectedHull.cxx
    vtkRedistributePolyData.cxx
    vtkWeightedRedistributePolyData.cxx
    )
ENDIF(VTK_USE_MPI)

IF (VTK_USE_PATENTED)
  SET(Filters_SRCS 
    ${Filters_SRCS}
    vtkPVKitwareContourFilter.cxx)
ENDIF (VTK_USE_PATENTED)

IF(NOT PARAVIEW_NO_HDF5)
  SET(Filters_SRCS ${Filters_SRCS} vtkHDF5RawImageReader.cxx)
ENDIF(NOT PARAVIEW_NO_HDF5)

VTK_WRAP_TCL2(vtkPVFiltersTCL FiltersTCL_SRCS ${Filters_SRCS})
ADD_LIBRARY(vtkPVFiltersTCL ${FiltersTCL_SRCS} ${Filters_SRCS})

TARGET_LINK_LIBRARIES(vtkPVFiltersTCL
  vtkParallelTCL
  vtkGraphicsTCL
  vtkRenderingTCL
  vtkHybridTCL
  )

IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtkPVFiltersTCL
    vtkPatentedTCL
    )
ENDIF (VTK_USE_PATENTED)

IF(NOT PARAVIEW_NO_HDF5)
  TARGET_LINK_LIBRARIES(vtkPVFiltersTCL ${PARAVIEW_HDF5_LIBRARIES})
ENDIF(NOT PARAVIEW_NO_HDF5)

ADD_EXECUTABLE(vtkTestFilters test.cxx)
TARGET_LINK_LIBRARIES(vtkTestFilters vtkPVFiltersTCL)

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkPVFiltersTCL)
ENDIF(BUILD_SHARED_LIBS)
