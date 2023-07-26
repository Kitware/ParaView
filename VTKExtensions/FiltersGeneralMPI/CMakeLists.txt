# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
# SPDX-License-Identifier: BSD-3-CLAUSE
set(classes
  vtkInSituPParticlePathFilter)

vtk_module_add_module(ParaView::VTKExtensionsFiltersGeneralMPI
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/general_mpi_filters.xml)
