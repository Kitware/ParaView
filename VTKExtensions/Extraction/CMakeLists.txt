# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkExtractSelectionRange
  vtkPConvertSelection
  vtkExtractCellsAlongLine
  vtkPVExtractCellsByType
  vtkPVExtractSelection
  vtkPVSelectionSource
  vtkPVSingleOutputExtractSelection
  vtkSelectArraysExtractBlocks)

vtk_module_add_module(ParaView::VTKExtensionsExtraction
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/extraction_filters.xml)
