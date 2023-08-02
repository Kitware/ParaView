# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVWebApplication)

vtk_module_add_module(ParaView::ClientsWeb
  CLASSES ${classes})
vtk_module_client_server_exclude()
