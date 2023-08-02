# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkEnsembleDataReader
  vtkNastranBDFReader
  vtkPhastaReader
  vtkPPhastaReader
  vtkPVPLYWriter
  vtkUnstructuredPOPReader
  vtkVRMLSource)

vtk_module_add_module(ParaView::VTKExtensionsIOGeneral
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/general_readers.xml
        Resources/general_writers.xml)
