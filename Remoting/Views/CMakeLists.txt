# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtk2DWidgetRepresentation
  vtk3DWidgetRepresentation
  vtkProtractorRepresentation
  vtkAMROutlineRepresentation
  vtkAMRStreamingPriorityQueue
  vtkAMRStreamingVolumeRepresentation
  vtkBoundingRectContextDevice2D
  vtkCaveSynchronizedRenderers
  vtkCellGridRepresentation
  vtkChartRepresentation
  vtkChartSelectionRepresentation
  vtkChartTextRepresentation
  vtkChartLogoRepresentation
  vtkChartWarning
  vtkCompositeRepresentation
  vtkContext2DScalarBarActor
  vtkDataLabelRepresentation
  vtkFeatureEdgesRepresentation
  vtkGeometryRepresentation
  vtkGeometryRepresentationWithFaces
  vtkGeometrySliceRepresentation
  vtkGlyph3DRepresentation
  vtkHyperTreeGridRepresentation
  vtkImageSliceRepresentation
  vtkImageVolumeRepresentation
  vtkLogoSourceRepresentation
  vtkMoleculeRepresentation
  vtkMultiSliceContextItem
  vtkOrderedCompositingHelper
  vtkOutlineRepresentation
  vtkPVAxesActor
  vtkPVAxesWidget
  vtkPVBoxChartRepresentation
  vtkPVCameraCollection
  vtkPVCenterAxesActor
  vtkPVClientServerSynchronizedRenderers
  vtkPVComparativeAnimationCue
  vtkPVComparativeView
  vtkPVCompositeOrthographicSliceRepresentation
  vtkPVCompositeRepresentation
  vtkPVContextInteractorStyle
  vtkPVContextView
  vtkPVContextViewDataDeliveryManager
  vtkPVDataDeliveryManager
  vtkPVDataRepresentation
  vtkPVDataRepresentationPipeline
  vtkPVDefaultPass
  vtkPVDiscretizableColorTransferFunction
  vtkPVDisplaySizedImplicitPlaneRepresentation
  vtkPVEncodeSelectionForServer
  vtkPVFrustumActor
  vtkPVGridAxes3DActor
  vtkPVGridAxes3DRepresentation
  vtkPVHardwareSelector
  vtkPVHistogramChartRepresentation
  vtkPVImageChartRepresentation
  vtkPVImageSliceMapper
  vtkPVImplicitAnnulusRepresentation
  vtkPVImplicitConeRepresentation
  vtkPVImplicitCylinderRepresentation
  vtkPVImplicitFrustumRepresentation
  vtkPVImplicitPlaneRepresentation
  vtkPVInteractiveViewLinkRepresentation
  vtkPVIOSettings
  vtkPVLODActor
  vtkPVLODVolume
  vtkPVLastSelectionInformation
  vtkPVLight
  vtkPVLogoSource
  vtkPVMaterialLibrary
  vtkPVMultiSliceView
  vtkPVOpenGLInformation
  vtkPVOrthographicSliceView
  vtkPVParallelCoordinatesRepresentation
  vtkPVPlotMatrixRepresentation
  vtkPVPlotMatrixView
  vtkPVPlotTime
  vtkPVProcessWindow
  vtkPVProminentValuesInformation
  vtkPVRayCastPickingHelper
  vtkPVRenderView
  vtkPVRenderViewDataDeliveryManager
  vtkPVRenderViewSettings
  vtkPVRenderViewWithEDL
  vtkPVRenderingCapabilitiesInformation
  vtkPVRepresentedArrayListSettings
  vtkPVRepresentedDataInformation
  vtkPVScalarBarActor
  vtkPVScalarBarRepresentation
  vtkPVSelectionInformation
  vtkPVStreamingPiecesInformation
  vtkPVSynchronizedRenderer
  vtkPVTransferFunction2D
  vtkPVTransferFunction2DBox
  vtkPVView
  vtkPVXYChartView
  vtkPointGaussianRepresentation
  vtkPolarAxesRepresentation
  vtkProgressBarSourceRepresentation
  vtkQuartileChartRepresentation
  vtkRulerSourceRepresentation
  vtkSIImageTextureProxy
  vtkSIPVRepresentationProxy
  vtkSIUnstructuredGridVolumeRepresentationProxy
  vtkSMBoxRepresentationProxy
  vtkSMCameraLink
  vtkSMCameraProxy
  vtkSMChartRepresentationProxy
  vtkSMChartSeriesListDomain
  vtkSMChartSeriesSelectionDomain
  vtkSMChartUseIndexForAxisDomain
  vtkSMCinemaVolumetricImageExtractWriterProxy
  vtkSMColorMapEditorHelper
  vtkSMComparativeAnimationCueProxy
  vtkSMComparativeAnimationCueUndoElement
  vtkSMComparativeViewProxy
  vtkSMContextViewProxy
  vtkSMContextItemWidgetProxy
  vtkSMDataDeliveryManagerProxy
  vtkSMDisplaySizedImplicitPlaneRepresentationProxy
  vtkSMImageExtractWriterProxy
  vtkSMImplicitPlaneRepresentationProxy
  vtkSMLightProxy
  vtkSMMaterialDomain
  vtkSMMaterialLibraryProxy
  vtkSMMultiSliceViewProxy
  vtkSMNew2DWidgetRepresentationProxy
  vtkSMNewWidgetRepresentationProxy
  vtkSMNewWidgetRepresentationProxyAbstract
  vtkSMOrthographicSliceViewProxy
  vtkSMPVMoleculeRepresentationProxy
  vtkSMPVRepresentationProxy
  vtkSMParaViewPipelineControllerWithRendering
  vtkSMPlotMatrixViewProxy
  vtkSMPlotlyJsonExtractWriterProxy
  vtkSMRangedTransferFunctionDomain
  vtkSMRecolorableImageExtractWriterProxy
  vtkSMRenderViewProxy
  vtkSMRendererDomain
  vtkSMRepresentationProxy
  vtkSMRepresentationTypeDomain
  vtkSMRepresentedArrayListDomain
  vtkSMSaveScreenshotProxy
  vtkSMScalarBarWidgetRepresentationProxy
  vtkSMSelectionHelper
  vtkSMSelectionLink
  vtkSMSpreadSheetRepresentationProxy
  vtkSMSpreadSheetViewProxy
  vtkSMTextWidgetRepresentationProxy
  vtkSMTransferFunction2DProxy
  vtkSMTransferFunctionManager
  vtkSMTransferFunctionPresets
  vtkSMTransferFunctionProxy
  vtkSMUtilities
  vtkSMViewLayoutProxy
  vtkSMViewLink
  vtkSMViewProxy
  vtkSMViewProxyInteractorHelper
  vtkSMViewResolutionDomain
  vtkSMWidgetRepresentationProxy
  vtkSMXYChartRepresentationInitializationHelper
  vtkSelectionRepresentation
  vtkSpreadSheetRepresentation
  vtkSpreadSheetView
  vtkStructuredGridVolumeRepresentation
  vtkSurfaceLICRepresentation
  vtkTextSourceRepresentation
  vtkThreeSliceFilter
  vtkTilesHelper
  vtkTransferFunctionBoxItem
  vtkTransferFunctionChartHistogram2D
  vtkUnstructuredGridVolumeRepresentation
  vtkViewLayout
  vtkVolumeRepresentation
  vtkXYChartRepresentation)

set(private_headers
  vtkPVDataDeliveryManagerInternals.h
  vtkGeometryRepresentationInternal.h
  vtkXYChartRepresentationInternals.h)
set(headers
  vtkStreamingPriorityQueue.h)
set(sources)

if (TARGET ParaView::icet)
  list(APPEND classes
    vtkIceTCompositePass
    vtkIceTContext
    vtkIceTSynchronizedRenderers)

  # Encode glsl files.
  foreach (file vtkIceTCompositeZPassShader_fs.glsl)
    get_filename_component(file_we ${file} NAME_WE)
    vtk_encode_string(
      INPUT         "${file}"
      NAME          "${file_we}"
      EXPORT_SYMBOL "VTKREMOTINGVIEWS_EXPORT"
      EXPORT_HEADER "vtkRemotingViewsModule.h"
      HEADER_OUTPUT header
      SOURCE_OUTPUT source)
    list(APPEND sources ${source})
    list(APPEND private_headers ${header})
  endforeach ()
endif()

# Compile ColorMaps.json
set(_colormaps_depends_args)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
  list(APPEND _colormaps_depends_args
    DEPENDS_EXPLICIT_ONLY)
endif ()

add_custom_command(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json"
          "$<TARGET_FILE:ParaView::ProcessXML>"
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
          "$<TARGET_FILE:ParaView::ProcessXML>"
          "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h"
          vtkSMTransferFunctionPresets JSON JSON
          "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json"
  COMMENT "Processing builtin color maps"
  ${_colormaps_depends_args})
list(APPEND private_headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h")

vtk_object_factory_declare(
  BASE vtkSMParaViewPipelineController
  OVERRIDE vtkSMParaViewPipelineControllerWithRendering)
vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKREMOTINGVIEWS_EXPORT")

vtk_module_add_module(ParaView::RemotingViews
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${vtk_object_factory_source} ${sources}
  PRIVATE_HEADERS ${vtk_object_factory_header} ${private_headers}
  # This is code generated from ColorMaps.json. Exclude until issue 22351 is fixed.
  SPDX_SKIP_REGEX "vtkSMTransferFunctionPresetsBuiltin"
)
  

paraview_add_server_manager_xmls(
  XMLS Resources/2dwidgets_remotingviews.xml
       Resources/3dwidgets_remotingviews.xml
       Resources/annotations_remotingviews.xml
       Resources/extractors_remotingviews.xml
       Resources/extractors_json.xml
       Resources/representationsinternal_remotingviews.xml
       Resources/representations_remotingviews.xml
       Resources/rendering.xml
       Resources/sources_remotingviews.xml
       Resources/utilities_remotingviews.xml
       Resources/viewsinternal_remotingviews.xml
       Resources/views_remotingviews.xml)
