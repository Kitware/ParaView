PROJECT(PVFilters)

# needed by vtkExportKit.cmake
SET(VTK_INSTALL_PACKAGE_DIR_CM24 ${PV_INSTALL_LIB_DIR})
SET(VTK_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})

# Any build configured headers should be added to this 
# variable so that they also get installed.
SET (dynamicHeaders)

INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

IF(PARAVIEW_ENABLE_PYTHON)
  INCLUDE_DIRECTORIES(
    ${PYTHON_INCLUDE_PATH}
    ${VTKPythonWrapping_SOURCE_DIR}/Executable
  )
ENDIF(PARAVIEW_ENABLE_PYTHON)

SET(Filters_SRCS
  vtkAnimationPlayer.cxx
  vtkAppendRectilinearGrid.cxx
  vtkAttributeDataReductionFilter.cxx
  vtkAttributeDataToTableFilter.cxx
  vtkBlockDeliveryPreprocessor.cxx
  vtkCameraInterpolator2.cxx
  vtkCameraManipulator.cxx
  vtkCameraManipulatorGUIHelper.cxx
  vtkCaveRenderManager.cxx
  vtkCleanArrays.cxx
  vtkCleanUnstructuredGrid.cxx
  vtkClientServerMoveData.cxx
  vtkCompleteArrays.cxx
  vtkCompositeAnimationPlayer.cxx
  vtkCompositeDataToUnstructuredGridFilter.cxx
  vtkCSVExporter.cxx
  vtkCSVWriter.cxx
  vtkDataSetToRectilinearGrid.cxx
  vtkDesktopDeliveryClient.cxx
  vtkDesktopDeliveryServer.cxx
  vtkExodusFileSeriesReader.cxx
  vtkExtractHistogram.cxx
  vtkExtractScatterPlot.cxx
  vtkFileSeriesReader.cxx
  vtkHierarchicalFractal.cxx
  vtkImageCompressor.cxx
  vtkImageSliceMapper.cxx
  vtkIntegrateAttributes.cxx
  vtkIntegrateFlowThroughSurface.cxx
  vtkInteractorStyleTransferFunctionEditor.cxx
  vtkKdTreeGenerator.cxx
  vtkKdTreeManager.cxx
  vtkMergeArrays.cxx
  vtkMinMax.cxx
  vtkMPICompositeManager.cxx
  vtkMPIMoveData.cxx
  vtkMultiViewManager.cxx
  vtkNetworkImageSource.cxx
  vtkOrderedCompositeDistributor.cxx
  vtkPConvertSelection.cxx
  vtkPExtractHistogram.cxx
  vtkParallelSerialWriter.cxx
  vtkPhastaReader.cxx
  vtkPointHandleRepresentationSphere.cxx
  vtkPolyLineToRectilinearGridFilter.cxx
  vtkPPhastaReader.cxx
  vtkPVAnimationScene.cxx
  vtkPVArrowSource.cxx
  vtkPVCacheKeeper.cxx
  vtkPVCacheKeeperPipeline.cxx
  vtkPVClientServerRenderManager.cxx
  vtkPVClipDataSet.cxx
  vtkPVConnectivityFilter.cxx
  vtkPVDesktopDeliveryClient.cxx
  vtkPVDesktopDeliveryServer.cxx
  vtkPVDReader.cxx
  vtkPVEnSightMasterServerReader.cxx
  vtkPVEnSightMasterServerTranslator.cxx
  vtkPVExtentTranslator.cxx
  vtkPVExtractSelection.cxx
  vtkPVExtractVOI.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVGeometryFilter.cxx
  vtkPVGeometryInformation.cxx
  vtkPVGlyphFilter.cxx
  vtkPVImageSlicer.cxx
  vtkPVInteractorStyle.cxx
  vtkPVJoystickFly.cxx
  vtkPVJoystickFlyIn.cxx
  vtkPVJoystickFlyOut.cxx
  vtkPVLinearExtrusionFilter.cxx
  vtkPVLODActor.cxx
  vtkPVLODVolume.cxx
  vtkPVLookupTable.cxx
  vtkPVMain.cxx
  vtkPVMergeTables.cxx
  vtkPVNullSource.cxx
  vtkPVRenderViewProxy.cxx
  vtkPVScalarBarActor.cxx
  vtkPVSelectionSource.cxx
  vtkPVServerArrayHelper.cxx
  vtkPVServerArraySelection.cxx
  vtkPVServerFileListing.cxx
  vtkPVServerObject.cxx
  vtkPVServerSelectTimeSet.cxx
  vtkPVServerTimeSteps.cxx
  vtkPVServerXDMFParameters.cxx
  vtkPVStringArrayHelper.cxx
  vtkPVSummaryHelper.cxx
  vtkPVTextSource.cxx
  vtkPVTrackballMoveActor.cxx
  vtkPVTrackballMultiRotate.cxx
  vtkPVTrackballPan.cxx
  vtkPVTrackballRoll.cxx
  vtkPVTrackballRotate.cxx
  vtkPVTrackballZoom.cxx
  vtkPVTrivialExtentTranslator.cxx
  vtkPVTrivialProducer.cxx
  vtkPVUpdateSuppressor.cxx
  vtkPVHardwareSelector.cxx
  vtkRealtimeAnimationPlayer.cxx
  vtkReductionFilter.cxx
  vtkSelectionStreamer.cxx
  vtkSequenceAnimationPlayer.cxx
  vtkSpyPlotBlock.cxx
  vtkSpyPlotBlockIterator.cxx
  vtkSpyPlotIStream.cxx
  vtkSpyPlotReader.cxx
  vtkSpyPlotReaderMap.cxx
  vtkSpyPlotUniReader.cxx
  vtkSquirtCompressor.cxx
  vtkSurfaceVectors.cxx
  vtkTableFFT.cxx
  vtkTableStreamer.cxx
#  vtkTemporalCacheFilter.cxx
#  vtkTemporalPickFilter.cxx
#  vtkTemporalProbeFilter.cxx
  vtkTexturePainter.cxx
  vtkTimestepsAnimationPlayer.cxx
  vtkTimeToTextConvertor.cxx
  vtkTrackballPan.cxx
  vtkTransferFunctionEditorRepresentation1D.cxx
  vtkTransferFunctionEditorRepresentation.cxx
  vtkTransferFunctionEditorRepresentationShapes1D.cxx
  vtkTransferFunctionEditorRepresentationShapes2D.cxx
  vtkTransferFunctionEditorRepresentationSimple1D.cxx
  vtkTransferFunctionEditorWidget1D.cxx
  vtkTransferFunctionEditorWidget.cxx
  vtkTransferFunctionEditorWidgetShapes1D.cxx
  vtkTransferFunctionEditorWidgetShapes2D.cxx
  vtkTransferFunctionEditorWidgetSimple1D.cxx
  vtkTransferFunctionViewer.cxx
  vtkUpdateSuppressorPipeline.cxx
  vtkVolumeRepresentationPreprocessor.cxx
  vtkVRMLSource.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXMLPVDWriter.cxx

# Backward-Compatibilty classes
  vtkMultiGroupDataGroupIdScalars.cxx
  vtkMultiGroupDataExtractGroup.cxx
  vtkMultiGroupDataExtractDataSets.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkPVMain.cxx
  vtkSpyPlotBlock.cxx
  vtkSpyPlotUniReader.cxx
  vtkSpyPlotIStream.cxx
  vtkSpyPlotBlockIterator.cxx
  vtkSpyPlotReaderMap.cxx
  WRAP_EXCLUDE)

SET_SOURCE_FILES_PROPERTIES(
  vtkAnimationPlayer.cxx
  vtkCameraManipulatorGUIHelper.cxx
  vtkImageCompressor.cxx
  vtkPVClientServerRenderManager.cxx
  vtkPVJoystickFly.cxx
  vtkPVRenderViewProxy.cxx
  vtkTransferFunctionEditorRepresentation1D.cxx
  vtkTransferFunctionEditorRepresentation.cxx
  vtkTransferFunctionEditorWidget1D.cxx
  vtkTransferFunctionEditorWidget.cxx
  ABSTRACT)

IF(PARAVIEW_ENABLE_PYTHON)
  SET(Filters_SRCS ${Filters_SRCS}
    vtkPythonCalculator.cxx
    vtkPythonProgrammableFilter.cxx
    )
ENDIF(PARAVIEW_ENABLE_PYTHON)

IF(PARAVIEW_USE_SILO)
  SET(Filters_SRCS ${Filters_SRCS}
    vtkSiloReader.cxx
    )
  INCLUDE_DIRECTORIES(${SILO_INCLUDE_DIR})
ENDIF(PARAVIEW_USE_SILO)

IF(VTK_USE_MPI)
  SET(Filters_SRCS ${Filters_SRCS}
    vtkBalancedRedistributePolyData.cxx
    vtkAllToNRedistributePolyData.cxx
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
VTK_EXPORT_KIT("PVFilters" "PVFILTERS" "${Filters_SRCS};${ExtraParaViewServer_SRCS};${ExtraParaViewCSWrapped_SRCS}")
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
SET (dynamicHeaders "${dynamicHeaders};${PVFilters_BINARY_DIR}/vtkPVFiltersConfig.h")
CONFIGURE_FILE(${PVFilters_SOURCE_DIR}/vtkPVFiltersConfig.h.in
  ${PVFilters_BINARY_DIR}/vtkPVFiltersConfig.h
  ESCAPE_QUOTES)

SET(VTK_WRAP_HINTS ${CMAKE_CURRENT_SOURCE_DIR}/hints)

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
  vtkPVServerCommonCS 
  vtkXdmfCS
  vtkHybridCS
  vtkParallelCS
  vtkVolumeRenderingCS
  vtkWidgetsCS
  KWCommon
  vtksys
  ${PARAVIEW_ADDITIONAL_LIBRARIES}
  )

IF(PARAVIEW_ENABLE_PYTHON)
  TARGET_LINK_LIBRARIES(vtkPVFilters 
    ${PYTHON_LIBRARIES}
    vtkPVPythonInterpretor
  ) 
ENDIF(PARAVIEW_ENABLE_PYTHON)

TARGET_LINK_LIBRARIES(vtkPVFilters ${PARAVIEW_HDF5_LIBRARIES} ${SPCTH_LIBRARIES})
TARGET_LINK_LIBRARIES(vtkPVFilters vtkClientServer)

IF(VTK_USE_MPI AND PARAVIEW_USE_ICE_T)
  TARGET_LINK_LIBRARIES(vtkPVFilters icet icet_mpi icet_strategies)
  INCLUDE_DIRECTORIES(${ICE_T_INCLUDE_DIR})
ENDIF(VTK_USE_MPI AND PARAVIEW_USE_ICE_T)

IF (PARAVIEW_ADAPTOR_LIBRARY)
  TARGET_LINK_LIBRARIES(vtkPVFilters ${PARAVIEW_ADAPTOR_LIBRARY})
ENDIF (PARAVIEW_ADAPTOR_LIBRARY)

IF (PARAVIEW_USE_SILO)
  TARGET_LINK_LIBRARIES(vtkPVFilters ${SILO_LIBRARIES})
ENDIF (PARAVIEW_USE_SILO)

# load the ClientServer command
INCLUDE(${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/vtkClientServer.cmake)
CS_INITIALIZE_WRAP()
INCLUDE_DIRECTORIES(${VTKCLIENTSERVER_INCLUDE_DIR})

# Wrap PVFilters
SET(KIT_WRAP_DEPS Parallel Hybrid Rendering VolumeRendering Widgets)
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/Servers/Filters/Utilities")
PV_WRAP_VTK_CS(PVFilters PVFILTERS "${KIT_WRAP_DEPS}")

#Install
#-------------------------------------------------------------------------------------------------
# Libraries
IF(NOT PV_INSTALL_NO_LIBRARIES)
    INSTALL(TARGETS vtkPVFiltersCS vtkPVFilters
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)
# Development
IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  GLOB_INSTALL_DEVELOPMENT(
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${PV_INSTALL_INCLUDE_DIR} 
      "*.h;*.hxx;*.txx")
  INSTALL(
      FILES ${dynamicHeaders}
      DESTINATION ${PV_INSTALL_INCLUDE_DIR} 
      COMPONENT Development)
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)

# Tests
#-------------------------------------------------------------------------------------------------
IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
