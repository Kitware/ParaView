# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkLegacyStreaklineFilter
  vtkLegacyParticlePathFilter
  )

vtk_module_add_module(ParaView::VTKExtensionsFiltersFlowPaths
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/flowpaths_filters.xml
  )
