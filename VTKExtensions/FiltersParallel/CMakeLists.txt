# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
vtk_object_factory_declare(
  BASE      vtkUnstructuredGridGhostCellsGenerator
  OVERRIDE  vtkPUnstructuredGridGhostCellsGenerator)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKPVVTKEXTENSIONSFILTERSPARALLEL_EXPORT")

set(classes
  vtkPUnstructuredGridGhostCellsGenerator)

vtk_module_add_module(ParaView::VTKExtensionsFiltersParallel
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
