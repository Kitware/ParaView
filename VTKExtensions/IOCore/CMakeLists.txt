# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkAdditionalFieldReader
  vtkCSVWriter
  vtkFileSeriesReader
  vtkFileSeriesWriter
  vtkMetaReader
  vtkParallelSerialWriter
  vtkPVDReader
  vtkStringReader
  vtkStringWriter
  vtkXMLCollectionReader
  vtkXMLPVDWriter)

vtk_module_add_module(ParaView::VTKExtensionsIOCore
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/filters_pv_iocore.xml
       Resources/readers_pv_iocore.xml
       Resources/writers_pv_iocore.xml)
