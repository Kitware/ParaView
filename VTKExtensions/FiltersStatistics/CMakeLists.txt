# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPSciVizContingencyStats
  vtkPSciVizDescriptiveStats
  vtkPSciVizKMeans
  vtkPSciVizMultiCorrelativeStats
  vtkPSciVizPCAStats
  vtkSciVizStatistics)

set(private_headers
  vtkSciVizStatisticsPrivate.h)

vtk_module_add_module(ParaView::VTKExtensionsFiltersStatistics
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})

paraview_add_server_manager_xmls(
  XMLS  Resources/statistics_filters.xml)
