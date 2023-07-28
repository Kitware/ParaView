# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVGeneralSettings)

vtk_module_add_module(ParaView::RemotingSettings
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/utilities_settings.xml)
