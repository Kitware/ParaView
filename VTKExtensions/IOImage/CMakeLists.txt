# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkImageFileSeriesReader
  vtkRawImageFileSeriesReader)

vtk_module_add_module(ParaView::VTKExtensionsIOImage
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/readers_pv_ioimage.xml
       Resources/writers_pv_ioimage.xml)
