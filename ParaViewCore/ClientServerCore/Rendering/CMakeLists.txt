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
  vtkCacheSizeKeeper
  vtkCaveSynchronizedRenderers
  vtkChartRepresentation
  vtkChartSelectionRepresentation
  vtkChartWarning
  vtkClientServerMoveData
  vtkCompositeRepresentation
  vtkDataLabelRepresentation
  vtkFeatureEdgesRepresentation
  vtkGeometryRepresentation
  vtkGeometryRepresentationWithFaces
  vtkGeometrySliceRepresentation
  vtkGlyph3DRepresentation
  vtkImageSliceRepresentation
  vtkImageVolumeRepresentation
  vtkMoleculeRepresentation
  vtkMPIMoveData
  vtkOutlineRepresentation
  vtkPExtentTranslator
  vtkPointGaussianRepresentation
  vtkPolarAxesRepresentation
  vtkProgressBarSourceRepresentation
  vtkPVBoxChartRepresentation
  vtkPVCacheKeeper
  vtkPVCacheKeeperPipeline
  vtkPVCacheSizeInformation
  vtkPVClientServerSynchronizedRenderers
  vtkPVCompositeOrthographicSliceRepresentation
  vtkPVCompositeRepresentation
  vtkPVContextInteractorStyle
  vtkPVContextView
  vtkPVDataDeliveryManager
  vtkPVDataRepresentation
  vtkPVDataRepresentationPipeline
  vtkPVGridAxes3DRepresentation
  vtkPVHardwareSelector
  vtkPVHistogramChartRepresentation
  vtkPVImageSliceMapper
  vtkPVImplicitCylinderRepresentation
  vtkPVImplicitPlaneRepresentation
  vtkPVLastSelectionInformation
  vtkPVLight
  vtkPVMaterialLibrary
  vtkPVMultiSliceView
  vtkPVOpenGLInformation
  vtkPVOrthographicSliceView
  vtkPVParallelCoordinatesRepresentation
  vtkPVPlotMatrixRepresentation
  vtkPVPlotMatrixView
  vtkPVProcessWindow
  vtkPVProminentValuesInformation
  vtkPVRayCastPickingHelper
  vtkPVRenderingCapabilitiesInformation
  vtkPVRenderView
  vtkPVRenderViewSettings
  vtkPVRepresentedDataInformation
  vtkPVSelectionInformation
  vtkPVStreamingPiecesInformation
  vtkPVSynchronizedRenderer
  vtkPVView
  vtkPVXYChartView
  vtkQuartileChartRepresentation
  vtkRulerSourceRepresentation
  vtkSelectionDeliveryFilter
  vtkSelectionRepresentation
  vtkSpreadSheetRepresentation
  vtkSpreadSheetView
  vtkStructuredGridVolumeRepresentation
  vtkTableExtentTranslator
  vtkTextSourceRepresentation
  vtkThreeSliceFilter
  vtkUnstructuredGridVolumeRepresentation
  vtkViewLayout
  vtkXYChartRepresentation)

if (TARGET VTK::ParallelMPI AND TARGET ParaView::icet)
  # TODO: Split these into a separate module so that there isn't a public
  # optional dependency.
  list(APPEND classes
    vtkIceTSynchronizedRenderers)
endif ()

set(headers
  vtkStreamingPriorityQueue.h)

set(private_headers
  vtkGeometryRepresentationInternal.h)

vtk_module_add_module(ParaView::ClientServerCoreRendering
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers})
