# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkSpyPlotBlock
  vtkSpyPlotFileSeriesReader
  vtkSpyPlotHistoryReader
  vtkSpyPlotIStream
  vtkSpyPlotReader
  vtkSpyPlotReaderMap
  vtkSpyPlotUniReader)

set(nowrap_classes
  vtkSpyPlotBlockIterator)

set(private_headers
  vtkSpyPlotHistoryReaderPrivate.h)

vtk_module_add_module(ParaView::VTKExtensionsIOSPCTH
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes}
  PRIVATE_HEADERS ${private_headers})

paraview_add_server_manager_xmls(
  XMLS Resources/readers_pv_iospcth.xml)
