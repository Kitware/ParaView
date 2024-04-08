# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVGenerateProcessIds)

vtk_module_add_module(ParaView::VTKExtensionsFiltersParallel
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/parallel_filters.xml)
