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
set(Module_SRCS
  vtkAttributeDataToTableFilter.cxx
  vtkBlockDeliveryPreprocessor.cxx
  vtkCameraInterpolator2.cxx
  vtkCameraManipulator.cxx
  vtkCameraManipulatorGUIHelper.cxx
  vtkCleanArrays.cxx
  vtkCompositeDataToUnstructuredGridFilter.cxx
  vtkContext2DScalarBarActor.cxx
  vtkCSVExporter.cxx
  vtkImageCompressor.cxx
  vtkImageTransparencyFilter.cxx
  vtkKdTreeGenerator.cxx
  vtkKdTreeManager.cxx
  vtkLZ4Compressor.cxx
  vtkMarkSelectedRows.cxx
  vtkMultiSliceContextItem.cxx
  vtkOrderedCompositeDistributor.cxx
  vtkPartitionOrdering.cxx
  vtkPartitionOrderingInterface.cxx
  vtkPVAxesActor.cxx
  vtkPVAxesWidget.cxx
  vtkPVCameraCollection.cxx
  vtkPVCenterAxesActor.cxx
  vtkBoundingRectContextDevice2D.cxx
  vtkPVDefaultPass.cxx
  vtkPVDiscretizableColorTransferFunction.cxx
  vtkPVGeometryFilter.cxx
  vtkPVGL2PSExporter.cxx
  vtkPVInteractiveViewLinkRepresentation.cxx
  vtkPVInteractorStyle.cxx
  vtkPVJoystickFly.cxx
  vtkPVJoystickFlyIn.cxx
  vtkPVJoystickFlyOut.cxx
  vtkPVLODActor.cxx
  vtkPVLODVolume.cxx
  vtkPVMergeTables.cxx
  vtkPVMergeTablesMultiBlock.cxx
  vtkPVPlotTime.cxx
  vtkPVRecoverGeometryWireframe.cxx
  vtkPVScalarBarActor.cxx
  vtkPVScalarBarRepresentation.cxx
  vtkPVTrackballMoveActor.cxx
  vtkPVTrackballMultiRotate.cxx
  vtkPVTrackballPan.cxx
  vtkPVTrackballRoll.cxx
  vtkPVTrackballRotate.cxx
  vtkPVTrackballZoom.cxx
  vtkPVTrackballZoomToMouse.cxx
  vtkPVUpdateSuppressor.cxx
  vtkResampledAMRImageSource.cxx
  vtkSelectionConverter.cxx
  vtkSortedTableStreamer.cxx
  vtkSquirtCompressor.cxx
  vtkTileDisplayHelper.cxx
  vtkTilesHelper.cxx
  vtkTrackballPan.cxx
  vtkUpdateSuppressorPipeline.cxx
  vtkViewLayout.cxx
  vtkVolumeRepresentationPreprocessor.cxx
  vtkZlibImageCompressor.cxx
)

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND Module_SRCS
    vtkCinemaExporter.cxx
    )
endif()

if (PARAVIEW_USE_MPI)
  list(APPEND Module_SRCS
    vtkAllToNRedistributeCompositePolyData.cxx
    vtkAllToNRedistributePolyData.cxx
    vtkBalancedRedistributePolyData.cxx
    vtkRedistributePolyData.cxx
    vtkWeightedRedistributePolyData.cxx
    vtkMPICompositeManager.cxx # deprecate?
    )

  if (PARAVIEW_USE_ICE_T)
    list(APPEND Module_SRCS
      vtkIceTCompositePass.cxx
      vtkIceTContext.cxx)
  endif()
endif()

if (PARAVIEW_ENABLE_NVPIPE)
  find_package(nvpipe REQUIRED)
  list(APPEND Module_SRCS
    vtkNvPipeCompressor.cxx
    )
  include_directories(${NVPIPE_INCLUDE_DIRS})
endif()

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
  list(APPEND Module_SRCS ${source})
endforeach()
#---------------------------------------------------------

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
endif()
vtk_module_library(vtkPVVTKExtensionsRendering ${Module_SRCS})
if(PARAVIEW_USE_MPI)
  vtk_mpi_link(${vtk-module})
endif()
include(vtkOpenGL)
vtk_opengl_link(vtkPVVTKExtensionsRendering)

if(PARAVIEW_ENABLE_NVPIPE)
  target_link_libraries(vtkPVVTKExtensionsRendering LINK_PRIVATE
    ${NVPIPE_LIBRARIES})
endif()
