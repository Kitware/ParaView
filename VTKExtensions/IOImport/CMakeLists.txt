# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkMetaImporter)

vtk_module_add_module(ParaView::VTKExtensionsIOImport
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/importers_pv_ioimport.xml)
