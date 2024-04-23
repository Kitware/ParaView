# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVGenerateGlobalIds
  vtkPVGhostCellsGenerator
  vtkPVProbeLineFilter)

vtk_module_add_module(ParaView::VTKExtensionsFiltersParallelDIY2
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/parallel_diy_filters.xml)
