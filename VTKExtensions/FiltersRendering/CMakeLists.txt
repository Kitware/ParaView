# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkAbstractChartExporter
  vtkAllToNRedistributeCompositePolyData
  vtkAllToNRedistributePolyData
  vtkBalancedRedistributePolyData
  vtkBlockDeliveryPreprocessor
  vtkClientServerMoveData
  vtkCSVExporter
  vtkDataTabulator
  vtkImageCompressor
  vtkImageTransparencyFilter
  vtkLZ4Compressor
  vtkMarkSelectedRows
  vtkMPIMoveData
  vtkNetworkImageSource
  vtkOrderedCompositeDistributor
  vtkPlotlyJsonExporter
  vtkPVGeometryFilter
  vtkRedistributePolyData
  vtkResampledAMRImageSource
  vtkSelectionDeliveryFilter
  vtkSortedTableStreamer
  vtkSquirtCompressor
  vtkVolumeRepresentationPreprocessor
  vtkWeightedRedistributePolyData
  vtkZlibImageCompressor
  )

if (PARAVIEW_ENABLE_NVPIPE)
  list(APPEND classes
    vtkNvPipeCompressor)
endif ()

vtk_module_add_module(ParaView::VTKExtensionsFiltersRendering
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/rendering_sources.xml
        Resources/filters_filtersrendering.xml)
