# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkPVEnsembleDataReaderInformation
  vtkPVMultiServerDataSource
  vtkSMCameraConfigurationReader
  vtkSMCameraConfigurationWriter
  vtkSMEnsembleDataReaderProxy
  vtkSMExtractSelectionProxyInitializationHelper
  vtkSMInteractionUndoStackBuilder
  vtkSMInteractiveSelectionPipeline
  vtkSMMultiServerSourceProxy
  vtkSMOMETIFFChannelCalculatorInitializationHelper
  vtkSMPLYWriterProxyInitializationHelper
  vtkSMPreselectionPipeline
  vtkSMProxyClipboard
  vtkSMProxyConfigurationReader
  vtkSMProxyConfigurationWriter
  vtkSMTesting
  vtkSMTooltipSelectionPipeline)

set(private_headers
  vtkSMCameraConfigurationFileInfo.h
  vtkSMProxyConfigurationFileInfo.h)

vtk_module_add_module(ParaView::RemotingMisc
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})

paraview_add_server_manager_xmls(
  XMLS Resources/sources_remotingmisc.xml)
