# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkAMRConnectivity
  vtkAMRDualClip
  vtkAMRDualContour
  vtkAMRDualGridHelper
  vtkAMRFragmentIntegration
  vtkAMRFragmentsFilter
  vtkPVAMRDualClip
  vtkPVAMRDualContour
  vtkPVAMRFragmentIntegration)

vtk_module_add_module(ParaView::VTKExtensionsAMR
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  "Resources/amr_filters.xml")
