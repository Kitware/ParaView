PROJECT(PVFilters)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")


SET(Filters_SRCS
  vtkAMRSpyPlotReader.cxx
  vtkCTHAMRCellToPointData.cxx
  vtkCTHAMRContour.cxx
  vtkCTHAMRExtractPart.cxx
  vtkCTHAMRSurface.cxx
  vtkCTHData.cxx
  vtkCTHDataToCTHDataFilter.cxx
  vtkCTHDataToPolyDataFilter.cxx
  vtkCTHExtractAMRPart.cxx
  vtkCTHFractal.cxx
  vtkCTHOutlineFilter.cxx
  vtkCTHSource.cxx
  vtkCaveRenderManager.cxx
  vtkCellCenterDepthSort.cxx
  vtkCleanUnstructuredGrid.cxx
  vtkClientCompositeManager.cxx
  vtkColorByPart.cxx
  vtkCompleteArrays.cxx
  vtkDataSetSubdivisionAlgorithm.cxx
  vtkDesktopDeliveryClient.cxx
  vtkDesktopDeliveryServer.cxx
  vtkGroup.cxx
  vtkHDF5RawImageReader.cxx
  vtkIntegrateAttributes.cxx
  vtkIntegrateFlowThroughSurface.cxx
  vtkMPIDuplicatePolyData.cxx
  vtkMPIDuplicateUnstructuredGrid.cxx
  vtkMPIMoveData.cxx
  vtkMazeSource.cxx
  vtkMergeArrays.cxx
  vtkMultiDisplayManager.cxx
  vtkPVArrowSource.cxx
  vtkPVClipDataSet.cxx
  vtkPVCompositeBuffer.cxx
  vtkPVCompositeUtilities.cxx
  vtkPVConnectivityFilter.cxx
  vtkPVContourFilter.cxx
  vtkPVDReader.cxx
  vtkPVEnSightMasterServerReader.cxx
  vtkPVEnSightMasterServerTranslator.cxx
  vtkPVExtentTranslator.cxx
  vtkPVExtractVOI.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVGeometryFilter.cxx
  vtkPVGeometryInformation.cxx
  vtkPVGlyphFilter.cxx
  vtkPVImageContinuousDilate3D.cxx
  vtkPVImageContinuousErode3D.cxx
  vtkPVImageGradient.cxx
  vtkPVImageGradientMagnitude.cxx
  vtkPVImageMedian3D.cxx
  vtkPVLODActor.cxx
  vtkPVLODVolume.cxx
  vtkPVLinearExtrusionFilter.cxx
  vtkPVMain.cxx
  vtkPVRenderViewProxy.cxx
  vtkPVRibbonFilter.cxx
  vtkPVServerArraySelection.cxx
  vtkPVServerFileListing.cxx
  vtkPVServerObject.cxx
  vtkPVServerSelectTimeSet.cxx
  vtkPVServerXDMFParameters.cxx
  vtkPVSummaryHelper.cxx
  vtkPVThresholdFilter.cxx
  vtkPVTreeComposite.cxx
  vtkPVUpdateSuppressor.cxx
  vtkPVWarpScalar.cxx
  vtkPVWarpVector.cxx
  vtkPVXMLElement.cxx
  vtkPVXMLParser.cxx
  vtkPickFilter.cxx
  vtkPickLineWidget.cxx
  vtkPickPointWidget.cxx
  vtkProjectedTetrahedraMapper.cxx
  vtkSelectInputs.cxx
  vtkSortDataArray.cxx
  vtkStreamingTessellator.cxx
  vtkSubdivisionAlgorithm.cxx
  vtkSurfaceVectors.cxx
  vtkTempTessellatorFilter.cxx # Remove after ParaView 1.4
  vtkTiledDisplaySchedule.cxx
  vtkVRMLSource.cxx
  vtkVisibilitySort.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXMLPVDWriter.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkPVMain.cxx
  WRAP_EXCLUDE)

SET_SOURCE_FILES_PROPERTIES( 
  vtkCTHDataToCTHDataFilter.cxx
  vtkCTHDataToPolyDataFilter.cxx
  vtkSubdivisionAlgorithm.cxx 
  vtkPVRenderViewProxy.cxx
  vtkVisibilitySort.cxx
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
VTK_EXPORT_KIT("PVFilters" "PVFILTERS" "${Filters_SRCS};${ExtraParaViewServer_SRCS}")
SET(VTK_BINARY_DIR "${_VTK_BINARY_DIR}")

INCLUDE_DIRECTORIES(
  ${PVFilters_BINARY_DIR}
  ${PVFilters_SOURCE_DIR}
  ${PVCommon_SOURCE_DIR}
  ${KWCOMMON_INCLUDE_DIR}
  ${HDF5_INCLUDE_DIR}
  ${SPCTH_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  ${XDMF_INCLUDE_DIRS}
  ${KWSYS_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
  )

IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

#########################################################################
# Configure ParaViewAdaptor
OPTION(PARAVIEW_BUILD_WITH_ADAPTOR "Build ParaView against the PVAdaptor private source library. Requires Kitware License" OFF)
MARK_AS_ADVANCED(PARAVIEW_BUILD_WITH_ADAPTOR)
IF(PARAVIEW_BUILD_WITH_ADAPTOR)
  FIND_LIBRARY(PARAVIEW_ADAPTOR_LIBRARY PVAdaptor
    PATHS /usr/lib /usr/local/lib
    DOC "Private source library. Requires Kitware License"
    )
  IF(PARAVIEW_ADAPTOR_LIBRARY)
    MESSAGE("adaptor lib found.")
    GET_FILENAME_COMPONENT(PARAVIEW_ADAPTOR_LIBRARY_PATH 
      "${PARAVIEW_ADAPTOR_LIBRARY}" PATH)
    SET(PARAVIEW_ADAPTOR_INCLUDE_DIR ${PARAVIEW_ADAPTOR_LIBRARY_PATH}/../../include/vtk)
  ENDIF(PARAVIEW_ADAPTOR_LIBRARY)
  MARK_AS_ADVANCED(PARAVIEW_ADAPTOR_LIBRARY)

  IF (PARAVIEW_ADAPTOR_LIBRARY)
    MESSAGE("adding adaptor include dir")
    MESSAGE("PARAVIEW_ADAPTOR_INCLUDE_DIR=${PARAVIEW_ADAPTOR_INCLUDE_DIR}")
    INCLUDE_DIRECTORIES(${PARAVIEW_ADAPTOR_INCLUDE_DIR})
    MESSAGE("adding adaptor lib")
    MESSAGE("PARAVIEW_ADAPTOR_LIBRARY=${PARAVIEW_ADAPTOR_LIBRARY}")
    #   TARGET_LINK_LIBRARIES(vtkKWParaView ${PARAVIEW_ADAPTOR_LIBRARY})
    #   TARGET_LINK_LIBRARIES(vtkPVFilters ${PARAVIEW_ADAPTOR_LIBRARY})
  ENDIF (PARAVIEW_ADAPTOR_LIBRARY)
ENDIF(PARAVIEW_BUILD_WITH_ADAPTOR)

#########################################################################

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
ADD_LIBRARY(vtkPVFilters ${Filters_SRCS} ${FiltersInst_SRCS}
  ${ExtraParaViewServer_SRCS} ${ExtraParaViewServerNonWrapped_SRCS})
TARGET_LINK_LIBRARIES(vtkPVFilters vtkKWCommon vtkPVCommon vtkPVCommonCS vtkXdmfCS vtkHybridCS vtkParallelCS vtkKWCommon ${PARAVIEW_ADDITIONAL_LIBRARIES})
IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtkPVFilters vtkPatented)
ENDIF (VTK_USE_PATENTED)
TARGET_LINK_LIBRARIES(vtkPVFilters ${PARAVIEW_HDF5_LIBRARIES} ${SPCTH_LIBRARIES})
TARGET_LINK_LIBRARIES(vtkPVFilters vtkClientServer)

IF(PARAVIEW_USE_ICE_T)
  TARGET_LINK_LIBRARIES(vtkPVFilters icet icet_mpi icet_strategies)
  INCLUDE_DIRECTORIES(${ICE_T_INCLUDE_DIR})
ENDIF(PARAVIEW_USE_ICE_T)

IF (PARAVIEW_ADAPTOR_LIBRARY)
  TARGET_LINK_LIBRARIES(vtkPVFilters ${PARAVIEW_ADAPTOR_LIBRARY})
ENDIF (PARAVIEW_ADAPTOR_LIBRARY)

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
