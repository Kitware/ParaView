PROJECT(PVFilters)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")


SET(Filters_SRCS
  vtkAttributeEditor.cxx
  vtkCVGeometryCache.cxx
  vtkCaveRenderManager.cxx
  vtkCleanUnstructuredGrid.cxx
  vtkClientCompositeManager.cxx
  vtkColorByPart.cxx
  vtkCompleteArrays.cxx
  vtkDataAnalysisFilter.cxx
  vtkDataSetSubdivisionAlgorithm.cxx
  vtkDesktopDeliveryClient.cxx
  vtkDesktopDeliveryServer.cxx
  vtkExtractHistogram.cxx
  vtkExtractScatterPlot.cxx
  vtkGroup.cxx
  vtkHDF5RawImageReader.cxx
  vtkHierarchicalFractal.cxx
  vtkImageCompressor.cxx
  vtkIntegrateAttributes.cxx
  vtkIntegrateFlowThroughSurface.cxx
  vtkMPICompositeManager.cxx
  vtkMPIDuplicatePolyData.cxx
  vtkMPIDuplicateUnstructuredGrid.cxx
  vtkMPIMoveData.cxx
  vtkMergeArrays.cxx
  vtkMultiActorHelper.cxx
  vtkMultiDisplayManager.cxx
  vtkOrderedCompositeDistributor.cxx
  vtkPPhastaReader.cxx
  vtkPVArrowSource.cxx
  vtkPVClipDataSet.cxx
  vtkPVCompositeBuffer.cxx
  vtkPVCompositeUtilities.cxx
  vtkPVConnectivityFilter.cxx
  vtkPVDesktopDeliveryClient.cxx
  vtkPVDesktopDeliveryServer.cxx
  vtkPVDReader.cxx
  vtkPVEnSightMasterServerReader.cxx
  vtkPVEnSightMasterServerTranslator.cxx
  vtkPVExtentTranslator.cxx
  vtkPVExtractVOI.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVGeometryFilter.cxx
  vtkPVGeometryInformation.cxx
  vtkPVGlyphFilter.cxx
  vtkPVLODActor.cxx
  vtkPVLODVolume.cxx
  vtkPVLinearExtrusionFilter.cxx
  vtkPVMain.cxx
  vtkPPickFilter.cxx
  vtkPVRenderViewProxy.cxx
  vtkPVRenderModuleHelper.cxx
  vtkPVServerArraySelection.cxx
  vtkPVServerFileListing.cxx
  vtkPVServerObject.cxx
  vtkPVServerSelectTimeSet.cxx
  vtkPVServerTimeSteps.cxx
  vtkPVServerXDMFParameters.cxx
  vtkPVSummaryHelper.cxx
  vtkPVTreeComposite.cxx
  vtkPVUpdateSuppressor.cxx
  vtkPhastaReader.cxx
  vtkPickFilter.cxx
  vtkSelectInputs.cxx
  vtkSpyPlotReader.cxx
  vtkStreamingTessellator.cxx
  vtkSubdivisionAlgorithm.cxx
  vtkSurfaceVectors.cxx
  vtkSquirtCompressor.cxx
  vtkTemporalCacheFilter.cxx
  vtkTemporalProbeFilter.cxx
  vtkTemporalPickFilter.cxx
  vtkTessellatorFilter.cxx
  vtkTiledDisplaySchedule.cxx
  vtkUpdateSuppressorPipeline.cxx
  vtkVRMLSource.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXMLPVDWriter.cxx
  vtkPropAndCellSelect.cxx
  vtkSelectDataSets.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkPVMain.cxx
  WRAP_EXCLUDE)

SET_SOURCE_FILES_PROPERTIES( 
  vtkSubdivisionAlgorithm.cxx 
  vtkPVRenderViewProxy.cxx
  vtkImageCompressor.cxx
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
      vtkIceTContext.cxx
      vtkIceTRenderManager.cxx
      vtkIceTRenderer.cxx
      )
  ENDIF(VTK_USE_MPI)
  SET_SOURCE_FILES_PROPERTIES(
    vtkIceTFactory.cxx
    ABSTRACT
    )
ENDIF (PARAVIEW_USE_ICE_T)


SET(_VTK_BINARY_DIR "${VTK_BINARY_DIR}")
SET(VTK_BINARY_DIR "${PVFilters_BINARY_DIR}")
INCLUDE(${VTK_CMAKE_DIR}/vtkExportKit.cmake)
VTK_EXPORT_KIT("PVFilters" "PVFILTERS" "${Filters_SRCS};${ExtraParaViewServer_SRCS}")
SET(VTK_BINARY_DIR "${_VTK_BINARY_DIR}")

INCLUDE_DIRECTORIES(
  ${PVFilters_BINARY_DIR}
  ${PVFilters_SOURCE_DIR}
  ${PVServerCommon_SOURCE_DIR}
  ${HDF5_INCLUDE_DIR}
  ${SPCTH_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  ${XDMF_INCLUDE_DIRS}
  ${ExtraParaViewServerFiltersIncludes}
  )

# The following was copy/paste from vtkIncludeDirectories.cmake
# OpenGL include directories.
IF(APPLE)
  IF(VTK_USE_X)
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
  ENDIF(VTK_USE_X)
ELSE(APPLE)
  INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
ENDIF(APPLE)

# MPI
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
TARGET_LINK_LIBRARIES(vtkPVFilters
  vtkPVServerCommon
  vtkPVServerCommonCS vtkXdmfCS
  vtkHybridCS
  vtkParallelCS
  vtkVolumeRenderingCS
  vtkWidgetsCS
  KWCommon
  vtksys
  ${PARAVIEW_ADDITIONAL_LIBRARIES}
  )
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
SET(KIT_WRAP_DEPS Parallel Hybrid Rendering VolumeRendering)
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/Servers/Filters/Utilities")
PV_WRAP_VTK_CS(PVFilters PVFILTERS "${KIT_WRAP_DEPS}")

IF(NOT PV_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${PV_INSTALL_LIB_DIR} vtkPVFiltersCS vtkPVFilters)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
