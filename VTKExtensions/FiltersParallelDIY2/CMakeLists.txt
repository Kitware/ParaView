# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVGhostCellsGenerator
  vtkPVProbeLineFilter)

vtk_module_add_module(ParaView::VTKExtensionsFiltersParallelDIY2
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/parallel_diy_filters.xml)
