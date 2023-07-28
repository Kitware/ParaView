# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkIntersectFragments
  vtkMaterialInterfaceCommBuffer
  vtkMaterialInterfaceFilter
  vtkMaterialInterfaceIdList
  vtkMaterialInterfacePieceLoading
  vtkMaterialInterfacePieceTransaction
  vtkMaterialInterfacePieceTransactionMatrix
  vtkMaterialInterfaceProcessLoading
  vtkMaterialInterfaceProcessRing
  vtkMaterialInterfaceToProcMap
  )

set(headers
  vtkMaterialInterfaceIdListItem.h
  vtkMaterialInterfaceUtilities.h)

vtk_module_add_module(ParaView::VTKExtensionsFiltersMaterialInterface
  CLASSES ${classes}
  HEADERS ${headers})

paraview_add_server_manager_xmls(
  XMLS  Resources/material_interface_filters.xml)
