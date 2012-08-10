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
set (Module_SRCS
  vtk3DWidgetRepresentation.cxx
  vtkAMRVolumeRepresentation.cxx
  vtkCaveSynchronizedRenderers.cxx
  vtkChartRepresentation.cxx
  vtkClientServerMoveData.cxx
  vtkCompositeRepresentation.cxx
  vtkCubeAxesRepresentation.cxx
  vtkDataLabelRepresentation.cxx
  vtkGeometryRepresentation.cxx
  vtkGeometryRepresentationWithFaces.cxx
  vtkGlyph3DRepresentation.cxx
  vtkImageSliceRepresentation.cxx
  vtkImageVolumeRepresentation.cxx
  vtkMoleculeRepresentation.cxx
  vtkMPIMoveData.cxx
  vtkOutlineRepresentation.cxx
  vtkPVBarChartView.cxx
  vtkPVCacheKeeper.cxx
  vtkPVCacheKeeperPipeline.cxx
  vtkPVCacheSizeInformation.cxx
  vtkPVCameraAnimationCue.cxx
  vtkPVCameraCueManipulator.cxx
  vtkPVCameraKeyFrame.cxx
  vtkPVClientServerSynchronizedRenderers.cxx
  vtkPVCompositeRepresentation.cxx
  vtkPVContextView.cxx
  vtkPVDataDeliveryManager.cxx
  vtkPVDataRepresentation.cxx
  vtkPVDataRepresentationPipeline.cxx
  vtkPVDisplayInformation.cxx
  vtkPVImageSliceMapper.cxx
  vtkPVImplicitPlaneRepresentation.cxx
  vtkPVLastSelectionInformation.cxx
  vtkPVLineChartView.cxx
  vtkPVOpenGLExtensionsInformation.cxx
  vtkPVParallelCoordinatesChartView.cxx
  vtkPVParallelCoordinatesRepresentation.cxx
  vtkPVPlotMatrixRepresentation.cxx
  vtkPVPlotMatrixView.cxx
  vtkPVRayCastPickingHelper.cxx
  vtkPVRenderView.cxx
  vtkPVRepresentedDataInformation.cxx
  vtkPVSelectionInformation.cxx
  vtkPVSynchronizedRenderer.cxx
  vtkPVSynchronizedRenderWindows.cxx
  vtkPVView.cxx
  vtkPVXYChartView.cxx
  vtkSelectionDeliveryFilter.cxx
  vtkSelectionRepresentation.cxx
  vtkSpreadSheetRepresentation.cxx
  vtkSpreadSheetView.cxx
  vtkTextSourceRepresentation.cxx
  vtkUnstructuredGridVolumeRepresentation.cxx
  vtkXYChartRepresentation.cxx
)

if (PARAVIEW_USE_ICE_T)
  list(APPEND Module_SRCS
    vtkIceTSynchronizedRenderers.cxx)
endif()

#set_source_files_properties(
#  WRAP_EXCLUDE
#  )

set_source_files_properties(
  vtkPVContextView
  vtkPVDataRepresentation
  vtkPVSynchronizedRenderWindows
  vtkPVView
  ABSTRACT)

vtk_module_library(vtkPVClientServerCoreRendering ${Module_SRCS})
