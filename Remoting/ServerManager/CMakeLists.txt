#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
set(classes
  vtkLoadStateOptions
  vtkRemoteWriterHelper
  vtkPVCatalystChannelInformation
  vtkPVCatalystSessionCore
  vtkPVFilePathEncodingHelper
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
  vtkSISILProperty
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
  vtkSMDataExtractWriterProxy
  vtkSMDataSourceProxy
  vtkSMDataTypeDomain
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
  vtkSMRemoteObject
  vtkSMRemoteObjectUpdateUndoElement
  vtkSMSelfGeneratingSourceProxy
  vtkSMServerStateLocator
  vtkSMSession
  vtkSMSessionClient
  vtkSMSessionObject
  vtkSMSessionProxyManager
  vtkSMSettings
  vtkSMSettingsProxy
  vtkSMSILDomain
  vtkSMSILModel
  vtkSMSourceProxy
  vtkSMStateLoader
  vtkSMStateLocator
  vtkSMStateVersionController
  vtkSMStringListDomain
  vtkSMStringVectorProperty
  vtkSMSubsetInclusionLatticeDomain
  vtkSMTimeKeeper
  vtkSMTimeKeeperProxy
  vtkSMTimeStepIndexDomain
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
