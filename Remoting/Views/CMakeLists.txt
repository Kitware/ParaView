#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
set(classes
  vtk3DWidgetRepresentation
  vtkAMROutlineRepresentation
  vtkAMRStreamingPriorityQueue
  vtkAMRStreamingVolumeRepresentation
  vtkBoundingRectContextDevice2D
  vtkCaveSynchronizedRenderers
  vtkChartRepresentation
  vtkChartSelectionRepresentation
  vtkChartTextRepresentation
  vtkChartWarning
  vtkCompositeRepresentation
  vtkContext2DScalarBarActor
  vtkDataLabelRepresentation
  vtkFeatureEdgesRepresentation
  vtkGeometryRepresentation
  vtkGeometryRepresentationWithFaces
  vtkGeometrySliceRepresentation
  vtkGlyph3DRepresentation
  vtkGridAxes2DActor
  vtkGridAxes3DActor
  vtkGridAxesHelper
  vtkGridAxesPlane2DActor
  vtkImageSliceRepresentation
  vtkImageVolumeRepresentation
  vtkLogoSourceRepresentation
  vtkMoleculeRepresentation
  vtkMultiSliceContextItem
  vtkOrderedCompositingHelper
  vtkOutlineRepresentation
  vtkPointGaussianRepresentation
  vtkPolarAxesRepresentation
  vtkProgressBarSourceRepresentation
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
  vtkPVEncodeSelectionForServer
  vtkPVFrustumActor
  vtkPVGridAxes3DActor
  vtkPVGridAxes3DRepresentation
  vtkPVHardwareSelector
  vtkPVHistogramChartRepresentation
  vtkPVImageSliceMapper
  vtkPVImplicitCylinderRepresentation
  vtkPVImplicitPlaneRepresentation
  vtkPVInteractiveViewLinkRepresentation
  vtkPVLastSelectionInformation
  vtkPVLight
  vtkPVLODActor
  vtkPVLODVolume
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
  vtkPVRenderingCapabilitiesInformation
  vtkPVRenderView
  vtkPVRenderViewDataDeliveryManager
  vtkPVRenderViewSettings
  vtkPVRenderViewWithEDL
  vtkPVRepresentedArrayListSettings
  vtkPVRepresentedDataInformation
  vtkPVScalarBarActor
  vtkPVScalarBarRepresentation
  vtkPVSelectionInformation
  vtkPVStreamingPiecesInformation
  vtkPVSynchronizedRenderer
  vtkPVView
  vtkPVXYChartView
  vtkQuartileChartRepresentation
  vtkRulerSourceRepresentation
  vtkSelectionRepresentation
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
  vtkSMComparativeAnimationCueProxy
  vtkSMComparativeAnimationCueUndoElement
  vtkSMComparativeViewProxy
  vtkSMContextViewProxy
  vtkSMDataDeliveryManagerProxy
  vtkSMImageExtractWriterProxy
  vtkSMImplicitPlaneRepresentationProxy
  vtkSMLightProxy
  vtkSMMaterialDomain
  vtkSMMaterialLibraryProxy
  vtkSMMultiSliceViewProxy
  vtkSMNewWidgetRepresentationProxy
  vtkSMOrthographicSliceViewProxy
  vtkSMParaViewPipelineControllerWithRendering
  vtkSMPlotMatrixViewProxy
  vtkSMPVMoleculeRepresentationProxy
  vtkSMPVRepresentationProxy
  vtkSMRangedTransferFunctionDomain
  vtkSMRecolorableImageExtractWriterProxy
  vtkSMRendererDomain
  vtkSMRenderViewProxy
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
  vtkSMTransferFunctionManager
  vtkSMTransferFunctionPresets
  vtkSMTransferFunctionProxy
  vtkSMUtilities
  vtkSMViewLayoutProxy
  vtkSMViewProxy
  vtkSMViewProxyInteractorHelper
  vtkSMViewResolutionDomain
  vtkSMWidgetRepresentationProxy
  vtkSMXYChartRepresentationInitializationHelper
  vtkSpreadSheetRepresentation
  vtkSpreadSheetView
  vtkStructuredGridVolumeRepresentation
  vtkTextSourceRepresentation
  vtkThreeSliceFilter
  vtkTilesHelper
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
add_custom_command(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json"
          ParaView::ProcessXML
  COMMAND ParaView::ProcessXML
          "${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h"
          vtkSMTransferFunctionPresets JSON JSON
          "${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json"
  COMMENT "Processing builtin color maps")
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
  PRIVATE_HEADERS ${vtk_object_factory_header} ${private_headers})

paraview_add_server_manager_xmls(
  XMLS Resources/3dwidgets_remotingviews.xml
       Resources/extractors_remotingviews.xml
       Resources/rendering.xml
       Resources/sources_remotingviews.xml
       Resources/utilities_remotingviews.xml
       Resources/views_and_representations.xml)
