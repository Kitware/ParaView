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
  vtkCompleteArrays.cxx
  vtkCompositeRepresentation.cxx
  vtkCubeAxesRepresentation.cxx
  vtkDataLabelRepresentation.cxx
  vtkGeometryRepresentation.cxx
  vtkGeometryRepresentationWithFaces.cxx
  vtkGlyph3DRepresentation.cxx
  vtkPVImageSliceMapper.cxx
  vtkImageSliceRepresentation.cxx
  vtkImageVolumeRepresentation.cxx
  vtkMoleculeRepresentation.cxx
  vtkMPIMoveData.cxx
  vtkMPIMToNSocketConnection.cxx
  vtkMPIMToNSocketConnectionPortInformation.cxx
  vtkNetworkAccessManager.cxx
  vtkNetworkImageSource.cxx
  vtkOutlineRepresentation.cxx
  vtkPVAlgorithmPortsInformation.cxx
  vtkPVArrayInformation.cxx
  vtkPVBarChartView.cxx
  vtkPVCacheKeeper.cxx
  vtkPVCacheKeeperPipeline.cxx
  vtkPVCacheSizeInformation.cxx
  vtkPVCameraAnimationCue.cxx
  vtkPVCameraCueManipulator.cxx
  vtkPVCameraKeyFrame.cxx
  vtkPVClassNameInformation.cxx
  vtkPVClientServerSynchronizedRenderers.cxx
  vtkPVCompositeDataInformation.cxx
  vtkPVCompositeDataInformationIterator.cxx
  vtkPVCompositeRepresentation.cxx
  vtkPVContextView.cxx
  vtkPVDataInformation.cxx
  vtkPVDataRepresentation.cxx
  vtkPVDataRepresentationPipeline.cxx
  vtkProcessModuleAutoMPI.cxx
  vtkProcessModule.cxx
  vtkPVDataDeliveryManager.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataSizeInformation.cxx
  vtkPVDisplayInformation.cxx
  vtkPVEnvironmentInformation.cxx
  vtkPVEnvironmentInformationHelper.cxx
  vtkPVExtractSelection.cxx
  vtkPVFileInformation.cxx
  vtkPVFileInformationHelper.cxx
  vtkPVGenericAttributeInformation.cxx
  vtkPVImplicitPlaneRepresentation.cxx
  vtkPVInformation.cxx
  vtkPVLastSelectionInformation.cxx
  vtkPVLineChartView.cxx
  vtkPVMultiClientsInformation.cxx
  vtkPVOpenGLExtensionsInformation.cxx
  vtkPVOptions.cxx
  vtkPVOptionsXMLParser.cxx
  vtkPVParallelCoordinatesChartView.cxx
  vtkPVParallelCoordinatesRepresentation.cxx
  vtkPVPlotMatrixRepresentation.cxx
  vtkPVPlotMatrixView.cxx
  vtkPVPlugin.cxx
  vtkPVPluginLoader.cxx
  vtkPVPluginsInformation.cxx
  vtkPVPluginTracker.cxx
  vtkPVProgressHandler.cxx
  vtkPVPythonModule.cxx
  vtkPVPythonPluginInterface.cxx
  vtkPVRayCastPickingHelper.cxx
  vtkPVRenderView.cxx
  vtkPVRepresentedDataInformation.cxx
  vtkPVSelectionInformation.cxx
  vtkPVServerInformation.cxx
  vtkPVServerManagerPluginInterface.cxx
  vtkPVServerOptions.cxx
  vtkPVSession.cxx
  vtkPVSILInformation.cxx
  vtkPVSingleOutputExtractSelection.cxx
  vtkPVSynchronizedRenderer.cxx
  vtkPVSynchronizedRenderWindows.cxx
  vtkPVSystemInformation.cxx
  vtkPVTemporalDataInformation.cxx
  vtkPVTimerInformation.cxx
  vtkPVView.cxx
  vtkPVXYChartView.cxx
  vtkSelectionDeliveryFilter.cxx
  vtkSelectionRepresentation.cxx
  vtkSession.cxx
  vtkSessionIterator.cxx
  vtkSpreadSheetRepresentation.cxx
  vtkSpreadSheetView.cxx
  vtkTCPNetworkAccessManager.cxx
  vtkTextSourceRepresentation.cxx
  vtkUnstructuredGridVolumeRepresentation.cxx
  vtkXMLPVAnimationWriter.cxx
  vtkXYChartRepresentation.cxx
)

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND Module_SRCS
    vtkAnnotateGlobalDataFilter.cxx
    vtkPythonAnimationCue.cxx
    vtkPythonCalculator.cxx
    vtkPythonProgrammableFilter.cxx
    vtkPythonExtractSelection.cxx
    vtkPythonAnnotationFilter.cxx
    )
endif()

if (PARAVIEW_USE_ICE_T)
  list(APPEND Module_SRCS
    vtkIceTSynchronizedRenderers.cxx)
endif()

if (PARAVIEW_USE_MPI)
  list(APPEND Module_SRCS
    vtkPVMPICommunicator.cxx)
  set_source_files_properties(
    vtkPVMPICommunicator
    WRAP_EXCLUDE)

endif()

set_source_files_properties(
  vtkProcessModuleAutoMPI
  vtkPVOptionsXMLParser
  vtkPVPlugin
  vtkPVPythonPluginInterface
  vtkPVServerManagerPluginInterface
  vtkPVServerOptions

  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkNetworkAccessManager
  vtkPVContextView
  vtkPVDataRepresentation
  vtkPVInformation
  vtkPVPlugin
  vtkPVPluginTracker
  vtkPVPythonPluginInterface
  vtkPVServerManagerPluginInterface
  vtkPVSession
  vtkPVSynchronizedRenderWindows
  vtkPVView
  vtkSession
  vtkTileDisplayHelper
  ABSTRACT)

vtk_module_library(vtkPVClientServerCore ${Module_SRCS})
