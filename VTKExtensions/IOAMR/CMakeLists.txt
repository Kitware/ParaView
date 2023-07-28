# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkAMRFileSeriesReader)

vtk_module_add_module(ParaView::VTKExtensionsIOAMR
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/readers_pv_ioamr.xml)
