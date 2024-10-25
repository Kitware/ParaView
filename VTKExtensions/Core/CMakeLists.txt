# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkCLIOptions
  vtkCommunicationErrorCatcher
  vtkDistributedTrivialProducer
  vtkEmulatedTimeAlgorithm
  vtkFileSequenceParser
  vtkLogRecorder
  vtkPVCellCenters
  vtkPVCompositeDataPipeline
  vtkPVDataUtilities
  vtkPVFeatureEdges
  vtkPVInformationKeys
  vtkPVLogger
  vtkPVNullSource
  vtkPVPostFilter
  vtkPVPostFilterExecutive
  vtkPVTestUtilities
  vtkPVTrivialProducer
  vtkPVXMLElement
  vtkPVXMLParser
  vtkStringList
  vtkUndoElement
  vtkUndoSet
  vtkUndoStack)

set(headers
  vtkMemberFunctionCommand.h)

set(private_headers
  vtkUndoStackInternal.h)

set(nowrap_classes
  vtkPVStringFormatter)

vtk_module_add_module(ParaView::VTKExtensionsCore
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers})

paraview_add_server_manager_xmls(
  XMLS  Resources/core_sources.xml)
