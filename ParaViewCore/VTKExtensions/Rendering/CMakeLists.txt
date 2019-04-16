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
  vtkAttributeDataToTableFilter
  vtkBlockDeliveryPreprocessor
  vtkBoundingRectContextDevice2D
  vtkCSVExporter
  vtkCameraInterpolator2
  vtkCameraManipulator
  vtkCameraManipulatorGUIHelper
  vtkCleanArrays
  vtkCompositeDataToUnstructuredGridFilter
  vtkContext2DScalarBarActor
  vtkDataObjectTreeToPointSetFilter
  vtkImageCompressor
  vtkImageTransparencyFilter
  vtkKdTreeGenerator
  vtkKdTreeManager
  vtkLZ4Compressor
  vtkMarkSelectedRows
  vtkMultiSliceContextItem
  vtkOrderedCompositeDistributor
  vtkPVAxesActor
  vtkPVAxesWidget
  vtkPVCameraCollection
  vtkPVCenterAxesActor
  vtkPVDefaultPass
  vtkPVDiscretizableColorTransferFunction
  vtkPVGL2PSExporter
  vtkPVGeometryFilter
  vtkPVInteractiveViewLinkRepresentation
  vtkPVInteractorStyle
  vtkPVJoystickFly
  vtkPVJoystickFlyIn
  vtkPVJoystickFlyOut
  vtkPVLODActor
  vtkPVLODVolume
  vtkPVMergeTables
  vtkPVMergeTablesMultiBlock
  vtkPVPlotTime
  vtkPVRecoverGeometryWireframe
  vtkPVScalarBarActor
  vtkPVScalarBarRepresentation
  vtkPVTrackballMoveActor
  vtkPVTrackballMultiRotate
  vtkPVTrackballPan
  vtkPVTrackballRoll
  vtkPVTrackballRotate
  vtkPVTrackballZoom
  vtkPVTrackballZoomToMouse
  vtkPVUpdateSuppressor
  vtkPartitionOrdering
  vtkPartitionOrderingInterface
  vtkResampledAMRImageSource
  vtkSelectionConverter
  vtkSortedTableStreamer
  vtkSquirtCompressor
  vtkTilesHelper
  vtkTrackballPan
  vtkUpdateSuppressorPipeline
  vtkVolumeRepresentationPreprocessor
  vtkZlibImageCompressor)

if (TARGET VTK::ParallelMPI)
  # TODO: Split these into a separate module so that there isn't a public
  # optional dependency.
  list(APPEND classes
    vtkAllToNRedistributeCompositePolyData
    vtkAllToNRedistributePolyData
    vtkBalancedRedistributePolyData
    vtkMPICompositeManager # deprecate?
    vtkRedistributePolyData
    vtkWeightedRedistributePolyData)

  if (TARGET ParaView::icet)
    # TODO: Split these into a separate module so that there isn't a public
    # optional dependency.
    list(APPEND classes
      vtkIceTCompositePass
      vtkIceTContext)
  endif ()
endif ()

if (PARAVIEW_ENABLE_NVPIPE)
  list(APPEND classes
    vtkNvPipeCompressor)
endif ()

set(sources)
set(private_headers)
#---------------------------------------------------------
# Encode glsl files.
foreach (file vtkIceTCompositeZPassShader_fs.glsl)
  get_filename_component(file_we ${file} NAME_WE)
  vtk_encode_string(
    INPUT         "${file}"
    NAME          "${file_we}"
    EXPORT_SYMBOL "VTKPVVTKEXTENSIONSRENDERING_EXPORT"
    EXPORT_HEADER "vtkPVVTKExtensionsRenderingModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources ${source})
  list(APPEND private_headers ${header})
endforeach ()

vtk_module_add_module(ParaView::VTKExtensionsRendering
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
