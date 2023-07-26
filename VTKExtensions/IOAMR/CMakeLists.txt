# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
# SPDX-License-Identifier: BSD-3-CLAUSE
set(classes
  vtkAMRFileSeriesReader)

vtk_module_add_module(ParaView::VTKExtensionsIOAMR
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/readers_pv_ioamr.xml)
