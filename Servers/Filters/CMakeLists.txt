PROJECT(PVFilters)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")


SET(Filters_SRCS
  vtkAttributeDataReductionFilter.cxx
  vtkAttributeEditor.cxx
  vtkCameraManipulator.cxx
  vtkCameraManipulatorGUIHelper.cxx
  vtkCaveRenderManager.cxx
  vtkCellSelect.cxx
  vtkCleanUnstructuredGrid.cxx
  vtkClientCompositeManager.cxx
  vtkClientServerMoveData.cxx
#  vtkColorByPart.cxx
  vtkCompleteArrays.cxx
  vtkCVGeometryCache.cxx
  vtkDataAnalysisFilter.cxx
  vtkDataSetSubdivisionAlgorithm.cxx
  vtkDesktopDeliveryClient.cxx
  vtkDesktopDeliveryServer.cxx
  vtkExtractBlockFromSelection.cxx
  vtkExtractHistogram.cxx
  vtkExtractHistogramExtentTranslator.cxx
  vtkExtractScatterPlot.cxx
#  vtkGroup.cxx
  vtkHDF5RawImageReader.cxx
  vtkHierarchicalFractal.cxx
  vtkImageCompressor.cxx
  vtkIntegrateAttributes.cxx
  vtkIntegrateFlowThroughSurface.cxx
  vtkMergeArrays.cxx
  vtkMPICompositeManager.cxx
  vtkMPIDuplicatePolyData.cxx
  vtkMPIDuplicateUnstructuredGrid.cxx
  vtkMPIMoveData.cxx
  vtkMultiActorHelper.cxx
  vtkMultiBlockExtractSelection.cxx
  vtkMultiDisplayManager.cxx
  vtkNetworkImageSource.cxx
  vtkOrderedCompositeDistributor.cxx
  vtkPhastaReader.cxx
  vtkPickFilter.cxx
  vtkPolyLineToRectilinearGridFilter.cxx
  vtkPPhastaReader.cxx
  vtkPPickFilter.cxx
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
  vtkPVInteractorStyle.cxx
  vtkPVJoystickFly.cxx
  vtkPVJoystickFlyIn.cxx
  vtkPVJoystickFlyOut.cxx
  vtkPVLinearExtrusionFilter.cxx
  vtkPVLODActor.cxx
  vtkPVLODVolume.cxx
  vtkPVLookupTable.cxx
  vtkPVMain.cxx
  vtkPVRenderModuleHelper.cxx
  vtkPVRenderViewProxy.cxx
  vtkPVServerArrayHelper.cxx
  vtkPVServerArraySelection.cxx
  vtkPVServerFileListing.cxx
  vtkPVServerObject.cxx
  vtkPVServerSelectTimeSet.cxx
  vtkPVServerTimeSteps.cxx
  vtkPVServerXDMFParameters.cxx
  vtkPVSummaryHelper.cxx
  vtkPVTrackballMoveActor.cxx
  vtkPVTrackballPan.cxx
  vtkPVTrackballRoll.cxx
  vtkPVTrackballRotate.cxx
  vtkPVTrackballZoom.cxx
  vtkPVTreeComposite.cxx
  vtkPVUpdateSuppressor.cxx
  vtkPVVisibleCellSelector.cxx
  vtkReductionFilter.cxx
#  vtkSelectInputs.cxx
  vtkSpyPlotReader.cxx
  vtkSquirtCompressor.cxx
  vtkStreamingTessellator.cxx
  vtkSubdivisionAlgorithm.cxx
  vtkSurfaceVectors.cxx
  vtkTemporalCacheFilter.cxx
  vtkTemporalPickFilter.cxx
  vtkTemporalProbeFilter.cxx
  vtkTessellatorFilter.cxx
  vtkTiledDisplaySchedule.cxx
  vtkTrackballPan.cxx
  vtkUpdateSuppressorPipeline.cxx
  vtkVolumeSelector.cxx
  vtkVRMLSource.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXMLPVDWriter.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkPVMain.cxx
  WRAP_EXCLUDE)

SET_SOURCE_FILES_PROPERTIES( 
  vtkCameraManipulatorGUIHelper
  vtkImageCompressor.cxx
  vtkPVRenderViewProxy.cxx
  vtkPVJoystickFly.cxx
  vtkSubdivisionAlgorithm.cxx 
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
  IF(VTK_USE_MPI)
    SET(Filters_SRCS 
      ${Filters_SRCS}
      vtkIceTContext.cxx
      vtkIceTRenderManager.cxx
      vtkIceTRenderer.cxx
      )
  ENDIF(VTK_USE_MPI)
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
  IF(PV_INSTALL_HAS_CMAKE_24)
    INSTALL(TARGETS vtkPVFiltersCS vtkPVFilters
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ELSE(PV_INSTALL_HAS_CMAKE_24)
    INSTALL_TARGETS(${PV_INSTALL_LIB_DIR} vtkPVFiltersCS vtkPVFilters)
  ENDIF(PV_INSTALL_HAS_CMAKE_24)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
