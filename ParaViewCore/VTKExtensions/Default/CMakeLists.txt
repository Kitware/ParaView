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
  vtkAdditionalFieldReader.cxx
  vtkAMRConnectivity.cxx
  vtkAMRDualClip.cxx
  vtkAMRDualContour.cxx
  vtkAMRDualGridHelper.cxx
  vtkAMRFileSeriesReader.cxx
  vtkAMRFragmentIntegration.cxx
  vtkAMRFragmentsFilter.cxx
  vtkAppendRectilinearGrid.cxx
  vtkCellIntegrator.cxx
  vtkCleanUnstructuredGridCells.cxx
  vtkCleanUnstructuredGrid.cxx
  vtkCSVWriter.cxx
  vtkEnsembleDataReader.cxx
  vtkEquivalenceSet.cxx
  vtkExodusFileSeriesReader.cxx
  vtkExtractScatterPlot.cxx
  vtkExtractSelectionRange.cxx
  vtkFileSequenceParser.cxx
  vtkFlashContour.cxx
  vtkGridAxes2DActor.cxx
  vtkGridAxes3DActor.cxx
  vtkGridAxesHelper.cxx
  vtkGridAxesPlane2DActor.cxx
  vtkGridConnectivity.cxx
  vtkHierarchicalFractal.cxx
  vtkHybridProbeFilter.cxx
  vtkIntegrateFlowThroughSurface.cxx
  vtkIntersectFragments.cxx
  vtkIsoVolume.cxx
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
  vtkPEnSightGoldBinaryReader.cxx
  vtkPEnSightGoldReader.cxx
  vtkPEnSightReader.cxx
  vtkPEquivalenceSet.cxx
  vtkPExtractTemporalFieldData.cxx
  vtkPGenericEnSightReader.cxx
  vtkPhastaReader.cxx
  vtkPlotEdges.cxx
  vtkPointHandleRepresentationSphere.cxx
  vtkPolyLineToRectilinearGridFilter.cxx
  vtkPPhastaReader.cxx
  vtkPSciVizContingencyStats.cxx
  vtkPSciVizDescriptiveStats.cxx
  vtkPSciVizKMeans.cxx
  vtkPSciVizMultiCorrelativeStats.cxx
  vtkPSciVizPCAStats.cxx
  vtkPVAMRDualClip.cxx
  vtkPVAMRDualContour.cxx
  vtkPVAMRFragmentIntegration.cxx
  vtkPVArrayCalculator.cxx
  vtkPVBox.cxx
  vtkPVClipClosedSurface.cxx
  vtkPVClipDataSet.cxx
  vtkPVConnectivityFilter.cxx
  vtkPVContourFilter.cxx
  vtkPVCylinder.cxx
  vtkPVDataSetAlgorithmSelectorFilter.cxx
  vtkPVDReader.cxx
  vtkPVEnSightMasterServerReader2.cxx
  vtkPVEnSightMasterServerReader.cxx
  vtkPVEnSightMasterServerTranslator.cxx
  vtkPVExtractBagPlots.cxx
  vtkPVExtractComponent.cxx
  vtkPVExtractVOI.cxx
  vtkPVFrustumActor.cxx
  vtkPVGlyphFilter.cxx
  vtkPVGridAxes3DActor.cxx
  vtkPVImageReader.cxx
  vtkPVLegacyGlyphFilter.cxx
  vtkPVLinearExtrusionFilter.cxx
  vtkPVMetaClipDataSet.cxx
  vtkPVMetaSliceDataSet.cxx
  vtkPVPlane.cxx
  vtkPVPLYWriter.cxx
  vtkPVSelectionSource.cxx
  vtkPVTextSource.cxx
  vtkPVTransposeTable.cxx
  vtkRulerLineForInput.cxx
  vtkQuerySelectionSource.cxx
  vtkRectilinearGridConnectivity.cxx
  vtkSciVizStatistics.cxx
  vtkSpyPlotBlock.cxx
  vtkSpyPlotBlockIterator.cxx
  vtkSpyPlotFileSeriesReader.cxx
  vtkSpyPlotHistoryReader.cxx
  vtkSpyPlotIStream.cxx
  vtkSpyPlotReader.cxx
  vtkSpyPlotReaderMap.cxx
  vtkSpyPlotUniReader.cxx
  vtkSurfaceVectors.cxx
  vtkTimeStepProgressFilter.cxx
  vtkTimeToTextConvertor.cxx
  vtkUnstructuredPOPReader.cxx
  vtkVRMLSource.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVDWriter.cxx
)
if (PARAVIEW_USE_MPI)
  list(APPEND Module_SRCS vtkInSituPParticlePathFilter.cxx)
endif()

# Disabled unless using the OpenGL backend.
if ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL")
  list(APPEND Module_SRCS
    vtkScatterPlotPainter.cxx
    vtkScatterPlotMapper.cxx
    )
endif()

# Add header-only files so they get installed correctly.
set (${vtk-module}_HDRS
  vtkMaterialInterfaceIdListItem.h
)

set_source_files_properties(
  vtkAMRDualGridHelper
  vtkGridAxesHelper
  vtkMaterialInterfaceCommBuffer
  vtkMaterialInterfaceIdList
  vtkMaterialInterfacePieceLoading
  vtkMaterialInterfacePieceTransaction
  vtkMaterialInterfacePieceTransactionMatrix
  vtkMaterialInterfaceProcessLoading
  vtkMaterialInterfaceProcessRing
  vtkMaterialInterfaceToProcMap
  vtkPVPlotTime
  vtkSpyPlotBlock
  vtkSpyPlotBlockIterator
  vtkSpyPlotIStream
  vtkSpyPlotReaderMap
  vtkSpyPlotUniReader
  WRAP_EXCLUDE
  )

set_source_files_properties(
  vtkSpyPlotBlockIterator
  PROPERTIES WRAP_EXCLUDE_PYTHON 1
  )

set_source_files_properties(
  vtkCellIntegrator
  vtkPEnSightReader
  vtkSciVizStatistics
  ABSTRACT)

# Use a custom hints file for this module.
set(${vtk-module}_WRAP_HINTS "${CMAKE_CURRENT_SOURCE_DIR}/hints")
vtk_module_library(vtkPVVTKExtensionsDefault ${Module_SRCS})
target_link_libraries(vtkPVVTKExtensionsDefault LINK_PRIVATE
  ${vtkIOInfovis_LIBRARIES})
include(vtkOpenGL)
vtk_opengl_link(vtkPVVTKExtensionsDefault)

OPTION(PARAVIEW_ENABLE_SPYPLOT_MARKERS
       "Enables markers in the spyplot reader"
       ON)
MARK_AS_ADVANCED(PARAVIEW_ENABLE_SPYPLOT_MARKERS)

IF(${PARAVIEW_ENABLE_SPYPLOT_MARKERS})
  ADD_DEFINITIONS(-DPARAVIEW_ENABLE_SPYPLOT_MARKERS)
ENDIF()
