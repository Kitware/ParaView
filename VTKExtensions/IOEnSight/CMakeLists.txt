# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPEnSightGoldBinaryReader
  vtkPEnSightGoldReader
  vtkPEnSightReader
  vtkPGenericEnSightReader
  vtkPVEnSightMasterServerReader
  vtkPVEnSightMasterServerReader2
  vtkPVEnSightMasterServerTranslator)

if (NOT VTK_LEGACY_REMOVE)
  option(VTK_ENSIGHT_SPARSE_MODE "Enable \"Sparse Mode\" support for Parallel EnSight Reader (DEPRECATED)." OFF)
  mark_as_advanced(VTK_ENSIGHT_SPARSE_MODE)
endif ()

if (VTK_ENSIGHT_SPARSE_MODE)
  message(WARNING "Warning: EnSight \"Sparse Mode\" is activated (legacy behavior when the number of"
  " MPI processes > 12). This mode can leads to crashes in some cases, we don't recommend to use it.")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPEnSightSparseMode.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPEnSightSparseMode.h")

vtk_module_add_module(ParaView::VTKExtensionsIOEnSight
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/readers_pv_ioensight.xml)
