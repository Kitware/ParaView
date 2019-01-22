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
  vtkLiveInsituLink
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
  vtkSMDataSourceProxy
  vtkSMDataTypeDomain
  vtkSMDeserializer
  vtkSMDeserializerProtobuf
  vtkSMDeserializerXML
  vtkSMDeserializerXMLCache
  vtkSMDimensionsDomain
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
  vtkSMExportProxyDepot
  vtkSMFieldDataDomain
  vtkSMFileListDomain
  vtkSMFixedTypeDomain
  vtkSMGlobalPropertiesLinkUndoElement
  vtkSMGlobalPropertiesProxy
  vtkSMIdTypeVectorProperty
  vtkSMIndexSelectionDomain
  vtkSMInputArrayDomain
  vtkSMInputFileNameDomain
  vtkSMInputProperty
  vtkSMInsituStateLoader
  vtkSMIntRangeDomain
  vtkSMIntVectorProperty
  vtkSMLink
  vtkSMLiveInsituLinkProxy
  vtkSMMapProperty
  vtkSMNamedPropertyIterator
  vtkSMNullProxy
  vtkSMNumberOfComponentsDomain
  vtkSMObject
  vtkSMOrderedPropertyIterator
  vtkSMOutputPort
  vtkSMPWriterProxy
  vtkSMParaViewPipelineController
  vtkSMPipelineState
  vtkSMPluginLoaderProxy
  vtkSMPluginManager
  vtkSMProperty
  vtkSMPropertyGroup
  vtkSMPropertyHelper
  vtkSMPropertyIterator
  vtkSMPropertyLink
  vtkSMProxy
  vtkSMProxyDefinitionManager
  vtkSMProxyGroupDomain
  vtkSMProxyInitializationHelper
  vtkSMProxyIterator
  vtkSMProxyLink
  vtkSMProxyListDomain
  vtkSMProxyLocator
  vtkSMProxyManager
  vtkSMProxyProperty
  vtkSMProxySelectionModel
  vtkSMReaderFactory
  vtkSMRemoteObject
  vtkSMRemoteObjectUpdateUndoElement
  vtkSMSILDomain
  vtkSMSILModel
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
set(template_classes
  vtkSMRangeDomainTemplate)

vtk_module_add_module(ParaView::ServerManagerCore
  CLASSES ${classes}
  TEMPLATE_CLASSES ${template_classes})
