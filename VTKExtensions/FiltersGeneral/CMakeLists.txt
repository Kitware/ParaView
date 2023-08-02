# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkAnnotateGlobalDataFilter
  vtkAnnotateSelectionFilter
  vtkCellIntegrator
  vtkConvertPolyhedraFilter
  vtkEquivalenceSet
  vtkExtractScatterPlot
  vtkFlashContour
  vtkHierarchicalFractal
  vtkHybridProbeFilter
  vtkImageSimpleSource
  vtkIntegrateFlowThroughSurface
  vtkIsoVolume
  vtkOMETIFFChannelCalculator
  vtkPEquivalenceSet
  vtkPlotEdges
  vtkPVArrayCalculator
  vtkPVClipClosedSurface
  vtkPVClipDataSet
  vtkPVConnectivityFilter
  vtkPVContourFilter
  vtkPVCutter
  vtkPVDataSetAlgorithmSelectorFilter
  vtkPVExtractComponent
  vtkPVExtractVOI
  vtkPVGlyphFilter
  vtkPVGradientFilter
  vtkPVLinearExtrusionFilter
  vtkPVMetaClipDataSet
  vtkPVMetaSliceDataSet
  vtkPVPlaneCutter
  vtkPVTextSource
  vtkPVThreshold
  vtkPVTransposeTable
  vtkRectilinearGridConnectivity
  vtkRulerLineForInput
  vtkSliceAlongPolyPlane
  vtkSurfaceVectors
  vtkTimeStepProgressFilter
  vtkTimeToTextConvertor)

vtk_module_add_module(ParaView::VTKExtensionsFiltersGeneral
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/general_filters.xml
        Resources/general_sources.xml)
