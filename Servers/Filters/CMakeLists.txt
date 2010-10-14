PROJECT(PVFilters)

# needed by vtkExportKit.cmake
SET(VTK_INSTALL_PACKAGE_DIR_CM24 ${PV_INSTALL_LIB_DIR})
SET(VTK_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})

# Any build configured headers should be added to this
# variable so that they also get installed.
SET (dynamicHeaders)

INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(KIT PVFilters)
SET(KIT_UPCASE "PVFILTERS")
IF(NOT WIN32)
  SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

IF(PARAVIEW_ENABLE_PYTHON)
  INCLUDE_DIRECTORIES(
    ${PYTHON_INCLUDE_PATH}
    ${VTKPythonWrapping_SOURCE_DIR}/Executable
  )
ENDIF(PARAVIEW_ENABLE_PYTHON)

SET(Filters_SRCS
  vtk3DWidgetRepresentation.cxx
  vtkAMRDualClip.cxx
  vtkAMRDualContour.cxx
  vtkAMRDualGridHelper.cxx
  vtkAnimationPlayer.cxx
  vtkAppendRectilinearGrid.cxx
  vtkAppendArcLength.cxx
  vtkAttributeDataReductionFilter.cxx
  vtkAttributeDataToTableFilter.cxx
  vtkBlockDeliveryPreprocessor.cxx
  vtkBSPCutsGenerator.cxx
  vtkCameraInterpolator2.cxx
  vtkCameraManipulator.cxx
  vtkCameraManipulatorGUIHelper.cxx
  vtkChartRepresentation.cxx
  vtkCleanArrays.cxx
  vtkCleanUnstructuredGrid.cxx
  vtkClientServerMoveData.cxx
  vtkCompleteArrays.cxx
  vtkCompositeAnimationPlayer.cxx
  vtkCompositeDataToUnstructuredGridFilter.cxx
  vtkCompositeRepresentation.cxx
  vtkContextNamedOptions.cxx
  vtkCSVExporter.cxx
  vtkCSVWriter.cxx
  vtkCubeAxesRepresentation.cxx
  vtkDataLabelRepresentation.cxx
  vtkDataSetToRectilinearGrid.cxx
  vtkEnzoReader.cxx
  vtkEquivalenceSet.cxx
  vtkExodusFileSeriesReader.cxx
  vtkExtractHistogram.cxx
  vtkExtractScatterPlot.cxx
  vtkFileSeriesReader.cxx
  vtkFileSeriesWriter.cxx
  vtkFlashContour.cxx
  vtkFlashReader.cxx
  vtkGlyph3DRepresentation.cxx
  vtkHierarchicalFractal.cxx
  vtkGeometryRepresentation.cxx
  vtkGeometryRepresentationWithFaces.cxx
  vtkGridConnectivity.cxx
  vtkImageCompressor.cxx
  vtkImageSliceDataDeliveryFilter.cxx
  vtkImageSliceMapper.cxx
  vtkImageSliceRepresentation.cxx
  vtkImageVolumeRepresentation.cxx
  vtkIntegrateAttributes.cxx
  vtkIntegrateFlowThroughSurface.cxx
  vtkInteractorStyleTransferFunctionEditor.cxx
  vtkIntersectFragments.cxx
  vtkIsoVolume.cxx
  vtkKdTreeGenerator.cxx
  vtkKdTreeManager.cxx
  vtkMarkSelectedRows.cxx
  vtkMaterialInterfaceCommBuffer.cxx
  vtkMaterialInterfaceFilter.cxx
  vtkMaterialInterfaceIdList.cxx
  vtkMaterialInterfacePieceLoading.cxx
  vtkMaterialInterfacePieceTransaction.cxx
  vtkMaterialInterfacePieceTransactionMatrix.cxx
  vtkMaterialInterfaceProcessLoading.cxx
  vtkMaterialInterfaceProcessRing.cxx
  vtkMaterialInterfaceToProcMap.cxx
  vtkMergeArrays.cxx
  vtkMergeCompositeDataSet.cxx
  vtkMinMax.cxx
  vtkMPICompositeManager.cxx
  vtkMPIMoveData.cxx
  vtkMultiViewManager.cxx
  vtkNetworkImageSource.cxx
  vtkOrderedCompositeDistributor.cxx
  vtkOutlineRepresentation.cxx
  vtkPConvertSelection.cxx
  vtkPExtractHistogram.cxx
  vtkParallelCoordinatesRepresentation.cxx
  vtkParallelSerialWriter.cxx
  vtkPEnSightGoldBinaryReader.cxx
  vtkPEnSightGoldReader.cxx
  vtkPEnSightReader.cxx
  vtkPGenericEnSightReader.cxx
  vtkPhastaReader.cxx
  vtkPlotEdges.cxx
  vtkPointHandleRepresentationSphere.cxx
  vtkPolyLineToRectilinearGridFilter.cxx
  vtkPPhastaReader.cxx
  vtkPSciVizContingencyStats.cxx
  vtkPSciVizDescriptiveStats.cxx
  vtkPSciVizMultiCorrelativeStats.cxx
  vtkPSciVizPCAStats.cxx
  vtkPSciVizKMeans.cxx
  vtkPVAMRDualClip.cxx
  vtkPVAnimationScene.cxx
  vtkPVArrayCalculator.cxx
  vtkPVArrowSource.cxx
  vtkPVAxesActor.cxx
  vtkPVAxesWidget.cxx
  vtkPVCacheKeeper.cxx
  vtkPVCacheKeeperPipeline.cxx
  vtkPVCenterAxesActor.cxx
  vtkPVClipClosedSurface.cxx
  vtkPVClipDataSet.cxx
  vtkPVConnectivityFilter.cxx
  vtkPVContextView.cxx
  vtkPVContourFilter.cxx
  vtkPVCompositeRepresentation.cxx
  vtkPVDataRepresentation.cxx
  vtkPVDataRepresentationPipeline.cxx
  vtkPVDefaultPass.cxx
  vtkPVDReader.cxx
  vtkPVEnSightMasterServerReader.cxx
  vtkPVEnSightMasterServerReader2.cxx
  vtkPVEnSightMasterServerTranslator.cxx
  vtkPVExtentTranslator.cxx
  vtkPVExtractSelection.cxx
  vtkPVExtractVOI.cxx
  vtkPVFrustumActor.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVGeometryFilter.cxx
  vtkPVGeometryInformation.cxx
  vtkPVGlyphFilter.cxx
  vtkPVInteractorStyle.cxx
  vtkPVJoystickFly.cxx
  vtkPVJoystickFlyIn.cxx
  vtkPVJoystickFlyOut.cxx
  vtkPVLastSelectionInformation.cxx
  vtkPVLinearExtrusionFilter.cxx
  vtkPVLODActor.cxx
  vtkPVLODVolume.cxx
  vtkPVMain.cxx
  vtkPVMergeTables.cxx
  vtkPVNullSource.cxx
  vtkPVRecoverGeometryWireframe.cxx
  vtkPV2DRenderView.cxx
  vtkPVRenderView.cxx
  vtkPVRenderViewWithShadowMap.cxx
  vtkPVRenderViewWithSobel.cxx
  vtkPVRenderViewProxy.cxx
  vtkPVRepresentedDataInformation.cxx
  vtkPVScalarBarActor.cxx
  vtkPVSelectionSource.cxx
  vtkPVServerArrayHelper.cxx
  vtkPVServerArraySelection.cxx
  vtkPVServerFileListing.cxx
  vtkPVServerObject.cxx
  vtkPVServerSelectTimeSet.cxx
  vtkPVServerTimeSteps.cxx
  vtkPVStringArrayHelper.cxx
  vtkPVSynchronizedRenderer.cxx
  vtkPVSynchronizedRenderWindows.cxx
  vtkPVTextSource.cxx
  vtkPVTrackballMoveActor.cxx
  vtkPVTrackballMultiRotate.cxx
  vtkPVTrackballPan.cxx
  vtkPVTrackballRoll.cxx
  vtkPVTrackballRotate.cxx
  vtkPVTrackballZoom.cxx
  vtkPVUpdateSuppressor.cxx
  vtkPVHardwareSelector.cxx
  vtkPVView.cxx
  vtkPVXYChartView.cxx
  vtkQuerySelectionSource.cxx
  vtkRealtimeAnimationPlayer.cxx
  vtkRectilinearGridConnectivity.cxx
  vtkReductionFilter.cxx
  vtkScatterPlotMapper.cxx
  vtkScatterPlotPainter.cxx
  vtkSciVizStatistics.cxx
  vtkSelectionConverter.cxx
  vtkSelectionDeliveryFilter.cxx
  vtkSelectionRepresentation.cxx
  vtkSequenceAnimationPlayer.cxx
  vtkSortedTableStreamer.cxx
  vtkSpreadSheetView.cxx
  vtkSpreadSheetRepresentation.cxx
  vtkSpyPlotBlock.cxx
  vtkSpyPlotBlockIterator.cxx
  vtkSpyPlotIStream.cxx
  vtkSpyPlotReader.cxx
  vtkSpyPlotReaderMap.cxx
  vtkSpyPlotUniReader.cxx
  vtkSquirtCompressor.cxx
  vtkZlibImageCompressor.cxx
  vtkSurfaceVectors.cxx
  vtkTextSourceRepresentation.cxx
  vtkTilesHelper.cxx
  vtkTableFFT.cxx
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
  vtkUnstructuredDataDeliveryFilter.cxx
  vtkUnstructuredGridVolumeRepresentation.cxx
  vtkUpdateSuppressorPipeline.cxx
  vtkVolumeRepresentationPreprocessor.cxx
  vtkVRMLSource.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXMLPVDWriter.cxx
  vtkXYChartRepresentation.cxx
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkAMRDualGridHelper.cxx
  vtkMaterialInterfaceCommBuffer.cxx
  vtkMaterialInterfaceIdList.cxx
  vtkMaterialInterfacePieceLoading.cxx
  vtkMaterialInterfacePieceTransaction.cxx
  vtkMaterialInterfacePieceTransactionMatrix.cxx
  vtkMaterialInterfaceProcessLoading.cxx
  vtkMaterialInterfaceProcessRing.cxx
  vtkMaterialInterfaceToProcMap.cxx
  vtkPVMain.cxx
  vtkSpyPlotBlock.cxx
  vtkSpyPlotBlockIterator.cxx
  vtkSpyPlotIStream.cxx
  vtkSpyPlotReaderMap.cxx
  vtkSpyPlotUniReader.cxx
  WRAP_EXCLUDE)

SET_SOURCE_FILES_PROPERTIES(
  vtkAnimationPlayer.cxx
  vtkCameraManipulatorGUIHelper.cxx
  vtkImageCompressor.cxx
  vtkPEnSightReader.cxx
  vtkPVContextView.cxx
  vtkPVDataRepresentation.cxx
  vtkPVJoystickFly.cxx
  vtkPVRenderViewProxy.cxx
  vtkPVView.cxx
  vtkSciVizStatistics.cxx
  vtkTransferFunctionEditorRepresentation1D.cxx
  vtkTransferFunctionEditorRepresentation.cxx
  vtkTransferFunctionEditorWidget1D.cxx
  vtkTransferFunctionEditorWidget.cxx
  ABSTRACT)

# The set of source files to be encoded.
SET(glsl_files
  vtkIceTCompositeZPassShader_fs
  )

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
    vtkAllToNRedistributeCompositePolyData.cxx
    vtkRedistributePolyData.cxx
    vtkWeightedRedistributePolyData.cxx
    )
ENDIF(VTK_USE_MPI)

IF (PARAVIEW_USE_ICE_T)
  IF(VTK_USE_MPI)
    SET(Filters_SRCS
      ${Filters_SRCS}
      #vtkCaveRenderManager.cxx
      vtkIceTCompositePass.cxx
      vtkIceTContext.cxx
      vtkIceTSynchronizedRenderers.cxx
      )
  ENDIF(VTK_USE_MPI)
ENDIF (PARAVIEW_USE_ICE_T)

IF (VTK_USE_QVTK)
  FIND_PACKAGE(Qt4)
  INCLUDE(${QT_USE_FILE})
  INCLUDE_DIRECTORIES(
    ${QT_INCLUDE_DIR}
    ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
    ${ParaView_BINARY_DIR}/VTK/GUISupport/Qt)
ENDIF (VTK_USE_QVTK)

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


IF(VTK_USE_RENDERING)
# Create custom commands to encode each assembly file into a C string
# literal in a header file.

SET(glsl_h_files)
FOREACH(file ${glsl_files})
  SET(src ${PVFilters_SOURCE_DIR}/${file}.glsl)
  SET(res ${PVFilters_BINARY_DIR}/${file}.cxx)
  SET(resh ${PVFilters_BINARY_DIR}/${file}.h)
  SET(glsl_h_files ${glsl_h_files} "${resh}")
  ADD_CUSTOM_COMMAND(
    OUTPUT ${res} ${resh}
    DEPENDS ${src} vtkEncodeString
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file} --build-header VTK_EXPORT vtkSystemIncludes.h
    )
  SET(Filters_SRCS ${Filters_SRCS} ${res})
  SET_SOURCE_FILES_PROPERTIES( ${res} WRAP_EXCLUDE)
ENDFOREACH(file)

ENDIF(VTK_USE_RENDERING)



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
  vtkViews
  vtkViewsCS
  vtkCharts
  vtkChartsCS
  KWCommon
  vtksys
  ${OPENGL_gl_LIBRARY}
  ${PARAVIEW_ADDITIONAL_LIBRARIES}
  )

IF(PARAVIEW_ENABLE_PYTHON)
  TARGET_LINK_LIBRARIES(vtkPVFilters
    ${PYTHON_LIBRARIES}
    vtkPVPythonInterpretor
  )
ENDIF(PARAVIEW_ENABLE_PYTHON)

IF (VTK_USE_QVTK)
  TARGET_LINK_LIBRARIES(vtkPVFilters QVTK)
ENDIF (VTK_USE_QVTK)

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

# Wrap for python
IF (PARAVIEW_ENABLE_PYTHON)
  SET(Kit_SRCS ${Filters_SRCS})
  SET(Kit_PYTHON_EXTRA_SRCS)
  SET(KIT_PYTHON_LIBS
    vtkPVFilters vtkPVServerCommonPythonD vtkPVFiltersPythonD vtkPVFiltersCS vtkRenderingPythonD vtkWidgetsPythonD vtkParallelPythonD
    vtkViewsPythonD
    )
  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  SET(VTK_WRAP_INCLUDE_DIRS ${VTK_INCLUDE_DIR})
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(KIT_PYTHON_DEPS)
  SET(VTK_INSTALL_NO_LIBRARIES 1)
  IF(VTKPythonWrapping_INSTALL_BIN_DIR)
    SET(VTK_INSTALL_NO_LIBRARIES)
  ENDIF(VTKPythonWrapping_INSTALL_BIN_DIR)

  SET(VTK_INSTALL_LIB_DIR "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR "${VTKPythonWrapping_INSTALL_BIN_DIR}")
  SET(VTK_INSTALL_LIB_DIR_CM24 "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR_CM24 "${VTKPythonWrapping_INSTALL_BIN_DIR}")

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_ENABLE_PYTHON)


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
