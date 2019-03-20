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
  vtkAMRConnectivity
  vtkAMRDualClip
  vtkAMRDualContour
  vtkAMRDualGridHelper
  vtkAMRFileSeriesReader
  vtkAMRFragmentIntegration
  vtkAMRFragmentsFilter
  vtkAdditionalFieldReader
  vtkAppendRectilinearGrid
  vtkCSVWriter
  vtkCellIntegrator
  vtkCleanUnstructuredGrid
  vtkCleanUnstructuredGridCells
  vtkEnsembleDataReader
  vtkEquivalenceSet
  vtkExodusFileSeriesReader
  vtkExtractScatterPlot
  vtkExtractSelectionRange
  vtkFileSequenceParser
  vtkFlashContour
  vtkGridAxes2DActor
  vtkGridAxes3DActor
  vtkGridAxesHelper
  vtkGridAxesPlane2DActor
  vtkGridConnectivity
  vtkHierarchicalFractal
  vtkHybridProbeFilter
  vtkImageSimpleSource
  vtkIntegrateFlowThroughSurface
  vtkIntersectFragments
  vtkIsoVolume
  vtkMaterialInterfaceCommBuffer
  vtkMaterialInterfaceFilter
  vtkMaterialInterfaceIdList
  vtkMaterialInterfacePieceLoading
  vtkMaterialInterfacePieceTransaction
  vtkMaterialInterfacePieceTransactionMatrix
  vtkMaterialInterfaceProcessLoading
  vtkMaterialInterfaceProcessRing
  vtkMaterialInterfaceToProcMap
  vtkMergeArrays
  vtkMergeCompositeDataSet
  vtkMinMax
  vtkPEnSightGoldBinaryReader
  vtkPEnSightGoldReader
  vtkPEnSightReader
  vtkPEquivalenceSet
  vtkPGenericEnSightReader
  vtkPPhastaReader
  vtkPSciVizContingencyStats
  vtkPSciVizDescriptiveStats
  vtkPSciVizKMeans
  vtkPSciVizMultiCorrelativeStats
  vtkPSciVizPCAStats
  vtkPVAMRDualClip
  vtkPVAMRDualContour
  vtkPVAMRFragmentIntegration
  vtkPVArrayCalculator
  vtkPVBox
  vtkPVClipClosedSurface
  vtkPVClipDataSet
  vtkPVConnectivityFilter
  vtkPVContourFilter
  vtkPVCylinder
  vtkPVDReader
  vtkPVDataSetAlgorithmSelectorFilter
  vtkPVEnSightMasterServerReader
  vtkPVEnSightMasterServerReader2
  vtkPVEnSightMasterServerTranslator
  vtkPVExtractComponent
  vtkPVExtractVOI
  vtkPVFrustumActor
  vtkPVGlyphFilter
  vtkPVGlyphFilterLegacy
  vtkPVGridAxes3DActor
  vtkPVLinearExtrusionFilter
  vtkPVMetaClipDataSet
  vtkPVMetaSliceDataSet
  vtkPVPLYWriter
  vtkPVPlane
  vtkPVSelectionSource
  vtkPVTextSource
  vtkPVTransposeTable
  vtkPhastaReader
  vtkPlotEdges
  vtkPointHandleRepresentationSphere
  vtkPolyLineToRectilinearGridFilter
  vtkQuerySelectionSource
  vtkRectilinearGridConnectivity
  vtkRulerLineForInput
  vtkSciVizStatistics
  vtkSpyPlotBlock
  vtkSpyPlotBlockIterator
  vtkSpyPlotFileSeriesReader
  vtkSpyPlotHistoryReader
  vtkSpyPlotIStream
  vtkSpyPlotReader
  vtkSpyPlotReaderMap
  vtkSpyPlotUniReader
  vtkSurfaceVectors
  vtkTimeStepProgressFilter
  vtkTimeToTextConvertor
  vtkUnstructuredPOPReader
  vtkVRMLSource
  vtkXMLCollectionReader
  vtkXMLPVDWriter)

set(headers
  vtkMaterialInterfaceIdListItem.h)

vtk_module_add_module(ParaView::VTKExtensionsDefault
  CLASSES ${classes}
  HEADERS ${headers})

option(PARAVIEW_ENABLE_SPYPLOT_MARKERS "Enables markers in the spyplot reader" ON)
mark_as_advanced(PARAVIEW_ENABLE_SPYPLOT_MARKERS)

if (PARAVIEW_ENABLE_SPYPLOT_MARKERS)
  set_source_files_properties(vtkSpyPlotReader.cxx vtkSpyPlotFileSeriesReader.cxx
    PROPERTIES
      COMPILE_DEFINITIONS "PARAVIEW_ENABLE_SPYPLOT_MARKERS")
endif ()
