PROJECT(PVFilters)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(Filters_SRCS
  vtkMultiDisplayManager.cxx
  vtkCaveRenderManager.cxx
  vtkPVServerArraySelection.cxx
  vtkPVServerFileListing.cxx
  vtkPVServerObject.cxx
  vtkPVServerProbe.cxx
  vtkPVServerSelectTimeSet.cxx
  vtkPVServerXDMFParameters.cxx
  vtkPVExtentTranslator.cxx
  vtkPickPointWidget.cxx
  vtkPVSummaryHelper.cxx

  vtkPVLODActor.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVRenderViewProxy.cxx
  vtkPVGeometryFilter.cxx
  vtkPVXMLElement.cxx
  vtkPVXMLParser.cxx
  vtkCleanUnstructuredGrid.cxx
  vtkColorByPart.cxx
  vtkCTHData.cxx
  vtkCTHDataToPolyDataFilter.cxx
  vtkCTHExtractAMRPart.cxx
  vtkCTHFractal.cxx
  vtkCTHOutlineFilter.cxx
  vtkCTHSource.cxx
  vtkGroup.cxx
  vtkHDF5RawImageReader.cxx
  vtkMPIDuplicatePolyData.cxx
  vtkMPIDuplicateUnstructuredGrid.cxx
  vtkMazeSource.cxx
  vtkMergeArrays.cxx
  vtkPickFilter.cxx
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
  vtkTiledDisplaySchedule.cxx
  vtkVRMLSource.cxx
  vtkXMLPVDWriter.cxx
  vtkDataSetSubdivisionAlgorithm.cxx
  vtkStreamingTessellator.cxx
  vtkSubdivisionAlgorithm.cxx
  vtkMPIMoveData.cxx

  vtkCompleteArrays.cxx
  vtkClientCompositeManager.cxx
  vtkPVDReader.cxx
  vtkPVCompositeUtilities.cxx
  vtkPVCompositeBuffer.cxx
  vtkPVTreeComposite.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXMLCollectionReader.cxx
  vtkSelectInputs.cxx
	vtkTempTessellatorFilter.cxx # Remove after ParaView 1.4
  )

SET_SOURCE_FILES_PROPERTIES( 
  vtkSubdivisionAlgorithm.cxx 
  vtkPVRenderViewProxy.cxx
  ABSTRACT)

IF(VTK_USE_MPI)
  SET(Filters_SRCS ${Filters_SRCS}
    vtkBalancedRedistributePolyData.cxx
    vtkAllToNRedistributePolyData.cxx
    vtkPVDuplicatePolyData.cxx
    vtkRedistributePolyData.cxx
    vtkWeightedRedistributePolyData.cxx
    )
ENDIF(VTK_USE_MPI)

IF (PARAVIEW_USE_ICE_T)
  SET(Filters_SRCS 
    ${Filters_SRCS}
    vtkIceTFactory.cxx
    vtkDesktopDeliveryServer.cxx
    vtkDesktopDeliveryClient.cxx
    )
  IF(VTK_USE_MPI)
    SET(Filters_SRCS 
      ${Filters_SRCS}
      vtkIceTClientCompositeManager.cxx
      vtkIceTRenderManager.cxx
      vtkIceTRenderer.cxx
      )
  ENDIF(VTK_USE_MPI)
  SET_SOURCE_FILES_PROPERTIES(
    vtkIceTFactory.cxx
    ABSTRACT
    )
ENDIF (PARAVIEW_USE_ICE_T)


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
  ${PVFilters_SOURCE_DIR}
  ${PVCommon_SOURCE_DIR}
  ${KWCOMMON_INCLUDE_DIR}
  ${HDF5_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  ${XDMF_INCLUDE_DIRS}
  )

IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)


CONFIGURE_FILE(${PVFilters_SOURCE_DIR}/vtkPVFiltersConfig.h.in
  ${PVFilters_BINARY_DIR}/vtkPVFiltersConfig.h
  ESCAPE_QUOTES)

# Setup vtkInstantiator registration for this library's classes.
INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)
VTK_MAKE_INSTANTIATOR3(vtkPVFiltersInstantiator FiltersInst_SRCS 
  "${Filters_SRCS}"
  VTK_EXPORT
  "${PVFilters_BINARY_DIR}" "")

# Create filters library.
ADD_LIBRARY(vtkPVFilters ${Filters_SRCS} ${FiltersInst_SRCS})
TARGET_LINK_LIBRARIES(vtkPVFilters vtkKWCommon vtkPVCommon vtkPVCommonCS vtkXdmfCS vtkHybridCS vtkParallelCS vtkKWCommon)
IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtkPVFilters vtkPatented)
ENDIF (VTK_USE_PATENTED)
TARGET_LINK_LIBRARIES(vtkPVFilters ${PARAVIEW_HDF5_LIBRARIES})
TARGET_LINK_LIBRARIES(vtkPVFilters vtkClientServer)

IF(PARAVIEW_USE_ICE_T)
  TARGET_LINK_LIBRARIES(vtkPVFilters icet icet_mpi icet_strategies)
  INCLUDE_DIRECTORIES(${ICE_T_INCLUDE_DIR})
ENDIF(PARAVIEW_USE_ICE_T)

# load the ClientServer command
INCLUDE(${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/vtkClientServer.cmake)
CS_INITIALIZE_WRAP()
INCLUDE_DIRECTORIES(${VTKCLIENTSERVER_INCLUDE_DIR})

# Wrap PVFilters
SET(KIT_WRAP_DEPS Parallel Hybrid Rendering)
IF(VTK_USE_PATENTED)
  SET(KIT_WRAP_DEPS Patented)
ENDIF(VTK_USE_PATENTED)
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/Servers/Filters/Utilities")
PV_WRAP_VTK_CS(PVFilters PVFILTERS "${KIT_WRAP_DEPS}")

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(/lib vtkPVFiltersCS vtkPVFilters)
ENDIF(BUILD_SHARED_LIBS)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
