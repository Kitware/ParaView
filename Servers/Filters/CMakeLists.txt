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

  vtkCompleteArrays.cxx
  vtkPVInformation.cxx
  vtkClientCompositeManager.cxx
  vtkPVDReader.cxx
  vtkPVCompositeUtilities.cxx
  vtkPVNumberOfOutputsInformation.cxx
  vtkPVLODPartDisplayInformation.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataInformation.cxx
  vtkPVCompositeBuffer.cxx
  vtkPVClassNameInformation.cxx
  vtkPVArrayInformation.cxx
  vtkPVTreeComposite.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXMLCollectionReader.cxx
  vtkSelectInputs.cxx
  vtkExtractCTHPart.cxx
  vtkStringList.cxx # Should go out
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkStringList.cxx
  WRAP_EXCLUDE)

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

IF (PARAVIEW_USE_ICE_T)
  SET(Filters_SRCS 
    ${Filters_SRCS}
    #vtkIceTComposite.cxx
    vtkIceTFactory.cxx
    vtkPVDeskTopRenderModuleUI.cxx
    vtkPVIceTDisplayRenderModuleUI.cxx
    vtkPVIceTRenderModuleUI.cxx
    vtkDesktopDeliveryServer.cxx
    vtkIceTClientCompositeManager.cxx
    vtkIceTRenderer.cxx
    vtkDesktopDeliveryClient.cxx
    )
  IF(VTK_USE_MPI)
    SET(Filters_SRCS 
      ${Filters_SRCS}
      vtkIceTRenderManager.cxx
      )
  ENDIF(VTK_USE_MPI)
  SET_SOURCE_FILES_PROPERTIES(
    vtkIceTFactory.cxx
    ABSTRACT
    )
ENDIF (PARAVIEW_USE_ICE_T)

SET_SOURCE_FILES_PROPERTIES(
  vtkPVInformation
  ABSTRACT
  )

IF (VTK_USE_PATENTED)
  SET(Filters_SRCS ${Filters_SRCS} vtkPVKitwareContourFilter.cxx)
ENDIF (VTK_USE_PATENTED)

SET(_VTK_BINARY_DIR "${VTK_BINARY_DIR}")
SET(VTK_BINARY_DIR "${PVFilters_BINARY_DIR}")
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkExportKit.cmake)
VTK_EXPORT_KIT("PVFilters" "PVFILTERS" "${Filters_SRCS}")
SET(VTK_BINARY_DIR "${_VTK_BINARY_DIR}")

INCLUDE_DIRECTORIES(
  ${PVFilters_BINARY_DIR}
  ${VTK_SOURCE_DIR}
  ${VTK_BINARY_DIR}
  ${ParaView_SOURCE_DIR}/Utilities/hdf5
  ${ParaView_BINARY_DIR}/Utilities/hdf5
  )

SET(kits Common Filtering IO Imaging Rendering Parallel Graphics Hybrid)
IF(VTK_USE_PATENTED)
  SET(kits ${kits} Patented)
ENDIF(VTK_USE_PATENTED)
FOREACH(kit ${kits})
  INCLUDE_DIRECTORIES(
    ${VTK_SOURCE_DIR}/${kit}
    ${VTK_BINARY_DIR}/${kit}
    )
ENDFOREACH(kit)

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
TARGET_LINK_LIBRARIES(vtkPVFilters vtkClientServer)

ADD_EXECUTABLE(vtkTestFilters test.cxx)
TARGET_LINK_LIBRARIES(vtkTestFilters vtkPVFiltersCS)

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
SET(KIT_WRAP_DEPS Parallel Hybrid Rendering)
IF(VTK_USE_PATENTED)
  SET(KIT_WRAP_DEPS Patented)
ENDIF(VTK_USE_PATENTED)
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/Servers/Filters/Utilities")
PV_WRAP_VTK_CS(PVFilters PVFILTERS "${KIT_WRAP_DEPS}")

