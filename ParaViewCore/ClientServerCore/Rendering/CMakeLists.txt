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
  vtkAMROutlineRepresentation.cxx
  vtkAMRStreamingPriorityQueue.cxx
  vtkAMRStreamingVolumeRepresentation.cxx
  vtkCacheSizeKeeper.cxx
  vtkCaveSynchronizedRenderers.cxx
  vtkChartRepresentation.cxx
  vtkChartSelectionRepresentation.cxx
  vtkChartWarning.cxx
  vtkClientServerMoveData.cxx
  vtkCompositeRepresentation.cxx
  vtkDataLabelRepresentation.cxx
  vtkGeometryRepresentation.cxx
  vtkGeometryRepresentationWithFaces.cxx
  vtkGeometrySliceRepresentation.cxx
  vtkGlyph3DRepresentation.cxx
  vtkImageSliceRepresentation.cxx
  vtkImageVolumeRepresentation.cxx
  vtkMoleculeRepresentation.cxx
  vtkMPIMoveData.cxx
  vtkOutlineRepresentation.cxx
  vtkPExtentTranslator.cxx
  vtkPolarAxesRepresentation.cxx
  vtkProgressBarSourceRepresentation.cxx
  vtkPVBagChartRepresentation.cxx
  vtkPVBoxChartRepresentation.cxx
  vtkPVCacheKeeper.cxx
  vtkPVCacheKeeperPipeline.cxx
  vtkPVCacheSizeInformation.cxx
  vtkPVClientServerSynchronizedRenderers.cxx
  vtkPVCompositeOrthographicSliceRepresentation.cxx
  vtkPVCompositeRepresentation.cxx
  vtkPVContextInteractorStyle.cxx
  vtkPVContextView.cxx
  vtkPVDataDeliveryManager.cxx
  vtkPVDataRepresentation.cxx
  vtkPVDataRepresentationPipeline.cxx
  vtkPVDisplayInformation.cxx
  vtkPVGridAxes3DRepresentation.cxx
  vtkPVHardwareSelector.cxx
  vtkPVHistogramChartRepresentation.cxx
  vtkPVImageSliceMapper.cxx
  vtkPVImplicitPlaneRepresentation.cxx
  vtkPVImplicitCylinderRepresentation.cxx
  vtkPVLastSelectionInformation.cxx
  vtkPVMultiSliceView.cxx
  vtkPVOpenGLInformation.cxx
  vtkPVOpenGLExtensionsInformation.cxx
  vtkPVOrthographicSliceView.cxx
  vtkPVParallelCoordinatesRepresentation.cxx
  vtkPVPlotMatrixRepresentation.cxx
  vtkPVPlotMatrixView.cxx
  vtkPVProminentValuesInformation.cxx
  vtkPVRayCastPickingHelper.cxx
  vtkPVRenderView.cxx
  vtkPVRenderViewSettings.cxx
  vtkPVRepresentedDataInformation.cxx
  vtkPVSelectionInformation.cxx
  vtkPVStreamingPiecesInformation.cxx
  vtkPVSynchronizedRenderer.cxx
  vtkPVSynchronizedRenderWindows.cxx
  vtkPVView.cxx
  vtkPVXYChartView.cxx
  vtkQuartileChartRepresentation.cxx
  vtkRulerSourceRepresentation.cxx
  vtkSelectionDeliveryFilter.cxx
  vtkSelectionRepresentation.cxx
  vtkSpreadSheetRepresentation.cxx
  vtkSpreadSheetView.cxx
  vtkStructuredGridVolumeRepresentation.cxx
  vtkTableExtentTranslator.cxx
  vtkTextSourceRepresentation.cxx
  vtkThreeSliceFilter.cxx
  vtkUnstructuredGridVolumeRepresentation.cxx
  vtkXYChartRepresentation.cxx
)

if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  add_definitions(-DVTKGL2)
  list(APPEND Module_SRCS
    vtkPointGaussianRepresentation.cxx)
endif()

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND Module_SRCS
    vtkPythonRepresentation.cxx
    vtkPythonView.cxx)
endif()


if (PARAVIEW_USE_ICE_T)
  list(APPEND Module_SRCS
    vtkIceTSynchronizedRenderers.cxx)
endif()

set_source_files_properties(
  vtkCacheSizeKeeper

  # No need to wrap vtkPExtentTranslator, its an internal class.
  vtkPExtentTranslator
  WRAP_EXCLUDE
)

set_source_files_properties(
  vtkCacheSizeKeeper
  vtkPVContextView
  vtkPVDataRepresentation
  vtkPVSynchronizedRenderWindows
  vtkPVView
  ABSTRACT)

set (${vtk-module}_HDRS vtkStreamingPriorityQueue.h)

# Use a custom hints file for this module.
set(${vtk-module}_WRAP_HINTS "${CMAKE_CURRENT_SOURCE_DIR}/hints")
vtk_module_library(vtkPVClientServerCoreRendering ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(vtkPVClientServerCoreRendering)
