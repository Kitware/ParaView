# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkExtractsDeliveryHelper
  vtkLiveInsituLink
  vtkSMInsituStateLoader
  vtkSMLiveInsituLinkProxy
  vtkSteeringDataGenerator
)

vtk_module_add_module(ParaView::RemotingLive
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/live_utilities.xml)
