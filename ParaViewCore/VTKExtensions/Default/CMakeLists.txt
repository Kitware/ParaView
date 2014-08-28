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
  vtkAMRConnectivity.cxx
  vtkAMRDualClip.cxx
  vtkAMRDualContour.cxx
  vtkAMRDualGridHelper.cxx
  vtkAMRFragmentIntegration.cxx
  vtkAMRFragmentsFilter.cxx
  vtkAMRFileSeriesReader.cxx
  vtkAppendArcLength.cxx
  vtkAppendRectilinearGrid.cxx
  vtkAttributeDataReductionFilter.cxx
  vtkCellIntegrator.cxx
  vtkCleanUnstructuredGrid.cxx
  vtkCleanUnstructuredGridCells.cxx
  vtkCSVWriter.cxx
  vtkEnsembleReader.cxx
  vtkEquivalenceSet.cxx
  vtkExodusFileSeriesReader.cxx
  vtkExtractHistogram.cxx
  vtkExtractScatterPlot.cxx
  vtkFileSequenceParser.cxx
  vtkFileSeriesReader.cxx
  vtkFileSeriesWriter.cxx
  vtkFlashContour.cxx
  vtkGridConnectivity.cxx
  vtkHierarchicalFractal.cxx
  vtkHybridProbeFilter.cxx
  vtkIntegrateAttributes.cxx
  vtkIntegrateFlowThroughSurface.cxx
#  vtkInteractorStyleTransferFunctionEditor.cxx
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
  vtkMetaReader.cxx
  vtkMinMax.cxx
  vtkParallelSerialWriter.cxx
  vtkPEnSightGoldBinaryReader.cxx
  vtkPEnSightGoldReader.cxx
  vtkPEnSightReader.cxx
  vtkPEquivalenceSet.cxx
  vtkPExtractHistogram.cxx
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
  vtkPVDReader.cxx
  vtkPVDataSetAlgorithmSelectorFilter.cxx
  vtkPVEnSightMasterServerReader.cxx
  vtkPVEnSightMasterServerReader2.cxx
  vtkPVEnSightMasterServerTranslator.cxx
  vtkPVExtractBagPlots.cxx
  vtkPVExtractVOI.cxx
  vtkPVFrustumActor.cxx
  vtkPVGlyphFilter.cxx
  vtkPVLegacyGlyphFilter.cxx
  vtkPVImageReader.cxx
  vtkPVLinearExtrusionFilter.cxx
  vtkPVMetaClipDataSet.cxx
  vtkPVMetaSliceDataSet.cxx
  vtkPVNullSource.cxx
  vtkPVPlane.cxx
  vtkPVSelectionSource.cxx
  vtkPVTextSource.cxx
  vtkPVTransform.cxx
  vtkPVTransposeTable.cxx
  vtkQuerySelectionSource.cxx
  vtkRectilinearGridConnectivity.cxx
  vtkScatterPlotMapper.cxx
  vtkScatterPlotPainter.cxx
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
  vtkTimeToTextConvertor.cxx
  vtkUnstructuredPOPReader.cxx
  vtkVRMLSource.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVDWriter.cxx
)

set_source_files_properties(
  vtkAMRDualGridHelper
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
  vtkCellIntegrator
  vtkPEnSightReader
  vtkSciVizStatistics
  ABSTRACT)

# Use a custom hints file for this module.
set(${vtk-module}_WRAP_HINTS "${CMAKE_CURRENT_SOURCE_DIR}/hints")
vtk_module_library(vtkPVVTKExtensionsDefault ${Module_SRCS})
target_link_libraries(vtkPVVTKExtensionsDefault LINK_PRIVATE
  ${vtkIOInfovis_LIBRARIES})

OPTION(PARAVIEW_ENABLE_SPYPLOT_MARKERS
       "Enables markers in the spyplot reader"
       ON)
MARK_AS_ADVANCED(PARAVIEW_ENABLE_SPYPLOT_MARKERS)

IF(${PARAVIEW_ENABLE_SPYPLOT_MARKERS})
  ADD_DEFINITIONS(-DPARAVIEW_ENABLE_SPYPLOT_MARKERS)
ENDIF()
