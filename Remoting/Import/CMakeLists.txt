# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkSIImporterProxy
  vtkSMImporterProxy
  vtkSMImporterFactory)

vtk_module_add_module(ParaView::RemotingImport
  CLASSES ${classes})
