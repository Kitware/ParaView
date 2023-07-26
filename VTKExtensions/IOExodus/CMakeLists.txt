# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
# SPDX-License-Identifier: BSD-3-CLAUSE
set(classes
  vtkExodusFileSeriesReader
)

vtk_module_add_module(ParaView::VTKExtensionsIOExodus
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/readers_ioexodus.xml
       Resources/writers_ioexodus.xml)
