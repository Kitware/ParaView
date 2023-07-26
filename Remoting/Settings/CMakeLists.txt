# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
# SPDX-License-Identifier: BSD-3-CLAUSE
set(classes
  vtkPVGeneralSettings)

vtk_module_add_module(ParaView::RemotingSettings
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/utilities_settings.xml)
