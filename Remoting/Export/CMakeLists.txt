# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVGL2PSExporter
  vtkPVWebExporter
  vtkPVX3DExporter
  vtkSMCSVExporterProxy
  vtkSMCSVProxiesInitializationHelper
  vtkSMExporterProxy
  vtkSMGL2PSExporterProxy
  vtkSMPropArrayListDomain
  vtkSMRenderViewExporterProxy
  vtkSMViewExportHelper)

vtk_module_add_module(ParaView::RemotingExport
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/exporters.xml)
