# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPropertyDecorator
  vtkBoolPropertyDecorator
  vtkInputDataTypeDecorator
  vtkEnableDecorator
  vtkGenericPropertyDecorator
  vtkCompositePropertyDecorator
  vtkShowDecorator)

vtk_module_add_module(ParaView::RemotingApplicationComponents
  CLASSES ${classes})


vtk_module_client_server_exclude()
