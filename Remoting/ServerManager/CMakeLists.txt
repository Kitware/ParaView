# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
# SPDX-License-Identifier: BSD-3-Clause
set(classes
  vtkLoadStateOptions
  vtkRemoteWriterHelper
  vtkPVCatalystChannelInformation
  vtkPVCatalystSessionCore
  vtkPVDataMover
  vtkPVFilePathEncodingHelper
  vtkPVMultiClientsInformation
  vtkPVProxyDefinitionIterator
  vtkPVSessionBase
  vtkPVSessionCore
  vtkPVSessionCoreInterpreterHelper
  vtkPVSessionServer
  vtkSIArraySelectionProperty
  vtkSICollaborationManager
  vtkSICompoundSourceProxy
  vtkSIDataArrayProperty
  vtkSIDataArraySelectionProperty
  vtkSIDirectoryProxy
  vtkSIDoubleMapProperty
  vtkSIDoubleVectorProperty
  vtkSIIdTypeVectorProperty
  vtkSIIndexSelectionProperty
  vtkSIInputProperty
  vtkSIIntVectorProperty
  vtkSIMetaReaderProxy
  vtkSIMultiplexerSourceProxy
  vtkSIObject
  vtkSIProperty
  vtkSIProxy
  vtkSIProxyDefinitionManager
  vtkSIProxyProperty
  vtkSISourceProxy
  vtkSIStringVectorProperty
  vtkSITimeLabelProperty
  vtkSITimeRangeProperty
  vtkSITimeStepsProperty
  vtkSIVectorProperty
  vtkSIWriterProxy
  vtkSMAMRLevelsDomain
  vtkSMArrayListDomain
  vtkSMArrayRangeDomain
  vtkSMArraySelectionDomain
  vtkSMBooleanDomain
  vtkSMBoundsDomain
  vtkSMCollaborationManager
  vtkSMCompositeTreeDomain
  vtkSMCompoundProxyDefinitionLoader
  vtkSMCompoundSourceProxy
  vtkSMCompoundSourceProxyDefinitionBuilder
  vtkSMCoreUtilities
  vtkSMDataAssemblyDomain
  vtkSMDataAssemblyListDomain
  vtkSMDataExtractWriterProxy
  vtkSMDataSourceProxy
  vtkSMDataTypeDomain
  vtkSMDataTypeQueryDomain
  vtkSMDeserializer
  vtkSMDeserializerProtobuf
  vtkSMDeserializerXML
  vtkSMDeserializerXMLCache
  vtkSMDimensionsDomain
  vtkSMDirectoryProxy
  vtkSMDiscreteDoubleDomain
  vtkSMDocumentation
  vtkSMDomain
  vtkSMDomainIterator
  vtkSMDoubleMapProperty
  vtkSMDoubleMapPropertyIterator
  vtkSMDoubleRangeDomain
  vtkSMDoubleVectorProperty
  vtkSMEnumerationDomain
  vtkSMExtentDomain
  vtkSMExtractsController
  vtkSMExtractTriggerProxy
  vtkSMExtractWriterProxy
  vtkSMFieldDataDomain
  vtkSMFileListDomain
  vtkSMFileUtilities
  vtkSMFixedTypeDomain
  vtkSMIdTypeVectorProperty
  vtkSMIndexSelectionDomain
  vtkSMInputArrayDomain
  vtkSMInputFileNameDomain
  vtkSMInputProperty
  vtkSMIntRangeDomain
  vtkSMIntVectorProperty
  vtkSMLink
  vtkSMLoadStateOptionsProxy
  vtkSMMapProperty
  vtkSMMessage
  vtkSMMultiplexerSourceProxy
  vtkSMMultiplexerInputDomain
  vtkSMNamedPropertyIterator
  vtkSMNullProxy
  vtkSMNumberOfComponentsDomain
  vtkSMObject
  vtkSMOrderedPropertyIterator
  vtkSMOutputPort
  vtkSMParaViewPipelineController
  vtkSMPipelineState
  vtkSMPluginLoaderProxy
  vtkSMPluginManager
  vtkSMProperty
  vtkSMPropertyGroup
  vtkSMPropertyHelper
  vtkSMPropertyIterator
  vtkSMPropertyLink
  vtkSMPropertyModificationUndoElement
  vtkSMProxy
  vtkSMProxyDefinitionManager
  vtkSMProxyGroupDomain
  vtkSMProxyInitializationHelper
  vtkSMProxyIterator
  vtkSMProxyLink
  vtkSMProxyListDomain
  vtkSMProxyLocator
  vtkSMProxyManager
  vtkSMProxyManagerUtilities
  vtkSMProxyProperty
  vtkSMProxySelectionModel
  vtkSMPSWriterProxy
  vtkSMPWriterProxy
  vtkSMReaderFactory
  vtkSMReaderReloadHelper
  vtkSMRegistrationNamesDomain
  vtkSMRemoteObject
  vtkSMRemoteObjectUpdateUndoElement
  vtkSMSelectionQueryDomain
  vtkSMSelfGeneratingSourceProxy
  vtkSMServerStateLocator
  vtkSMSession
  vtkSMSessionClient
  vtkSMSessionObject
  vtkSMSessionProxyManager
  vtkSMSettings
  vtkSMSettingsProxy
  vtkSMSourceProxy
  vtkSMStateLoader
  vtkSMStateLocator
  vtkSMStateVersionController
  vtkSMStringListDomain
  vtkSMStringVectorProperty
  vtkSMTimeKeeper
  vtkSMTimeKeeperProxy
  vtkSMTimeStepIndexDomain
  vtkSMTimeStepsDomain
  vtkSMTrace
  vtkSMUncheckedPropertyHelper
  vtkSMUndoElement
  vtkSMUndoStack
  vtkSMUndoStackBuilder
  vtkSMVectorProperty
  vtkSMWriterFactory
  vtkSMWriterProxy)

set(headers
  vtkSMMessageMinimal.h)

set(private_headers
  vtkSMPropertyInternals.h
  vtkSMProxyInternals.h
  vtkSMProxyPropertyInternals.h
  vtkSMSessionProxyManagerInternals.h)


set(template_classes
  vtkSIVectorPropertyTemplate
  vtkSMRangeDomainTemplate)

vtk_module_add_module(ParaView::RemotingServerManager
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  TEMPLATE_CLASSES ${template_classes})

paraview_protobuf_generate(
  PROTOS    vtkPVMessage.proto
  OUT_VAR   vtkPVMessage_sources
  LANGUAGE  cpp
  EXPORT_MACRO VTKREMOTINGSERVERMANAGER_EXPORT)
# Install this way to hide it from the wrappers.
vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.h")
add_custom_target(vtkPVMessage_protobuf_compile
  DEPENDS
    ${vtkPVMessage_sources})
vtk_module_depend(ParaView::RemotingServerManager
  vtkPVMessage_protobuf_compile)
