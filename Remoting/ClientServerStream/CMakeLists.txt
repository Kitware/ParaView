# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkClientServerID
  vtkClientServerInterpreter
  vtkClientServerInterpreterInitializer
  vtkClientServerStream
  vtkClientServerStreamInstantiator)

vtk_module_add_module(ParaView::RemotingClientServerStream
  CLASSES ${classes})
