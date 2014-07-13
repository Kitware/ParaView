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
set (Module_SRCS
  vtkLiveInsituLink.cxx
  vtkSMArrayListDomain.cxx
  vtkSMArrayRangeDomain.cxx
  vtkSMArraySelectionDomain.cxx
  vtkSMBooleanDomain.cxx
  vtkSMBoundsDomain.cxx
  vtkSMCollaborationManager.cxx
  vtkSMCompositeTreeDomain.cxx
  vtkSMCompoundProxyDefinitionLoader.cxx
  vtkSMCompoundSourceProxy.cxx
  vtkSMCompoundSourceProxyDefinitionBuilder.cxx
  vtkSMCoreUtilities.cxx
  vtkSMDataSourceProxy.cxx
  vtkSMDataTypeDomain.cxx
  vtkSMDeserializer.cxx
  vtkSMDeserializerProtobuf.cxx
  vtkSMDeserializerXMLCache.cxx
  vtkSMDeserializerXML.cxx
  vtkSMDimensionsDomain.cxx
  vtkSMDocumentation.cxx
  vtkSMDomain.cxx
  vtkSMDomainIterator.cxx
  vtkSMDoubleMapProperty.cxx
  vtkSMDoubleMapPropertyIterator.cxx
  vtkSMDoubleRangeDomain.cxx
  vtkSMDoubleVectorProperty.cxx
  vtkSMEnumerationDomain.cxx
  vtkSMExtentDomain.cxx
  vtkSMFieldDataDomain.cxx
  vtkSMFileListDomain.cxx
  vtkSMFixedTypeDomain.cxx
  vtkSMGlobalPropertiesLinkUndoElement.cxx
  vtkSMGlobalPropertiesProxy.cxx
  vtkSMIdTypeVectorProperty.cxx
  vtkSMInputArrayDomain.cxx
  vtkSMInputProperty.cxx
  vtkSMInsituStateLoader.cxx
  vtkSMIntRangeDomain.cxx
  vtkSMIntVectorProperty.cxx
  vtkSMLink.cxx
  vtkSMLiveInsituLinkProxy.cxx
  vtkSMMapProperty.cxx
  vtkSMNamedPropertyIterator.cxx
  vtkSMNumberOfComponentsDomain.cxx
  vtkSMObject.cxx
  vtkSMOrderedPropertyIterator.cxx
  vtkSMOutputPort.cxx
  vtkSMParaViewPipelineController.cxx
  vtkSMPWriterProxy.cxx
  vtkSMPipelineState.cxx
  vtkSMPluginLoaderProxy.cxx
  vtkSMPluginManager.cxx
  vtkSMProperty.cxx
  vtkSMPropertyGroup.cxx
  vtkSMPropertyHelper.cxx
  vtkSMPropertyIterator.cxx
  vtkSMPropertyLink.cxx
  vtkSMProxy.cxx
  vtkSMProxyDefinitionManager.cxx
  vtkSMProxyGroupDomain.cxx
  vtkSMProxyIterator.cxx
  vtkSMProxyLink.cxx
  vtkSMProxyListDomain.cxx
  vtkSMProxyLocator.cxx
  vtkSMProxyManager.cxx
  vtkSMProxyProperty.cxx
  vtkSMProxySelectionModel.cxx
  vtkSMRangeDomainTemplate.txx
  vtkSMReaderFactory.cxx
  vtkSMRemoteObject.cxx
  vtkSMRemoteObjectUpdateUndoElement.cxx
  vtkSMServerStateLocator.cxx
  vtkSMSessionClient.cxx
  vtkSMSession.cxx
  vtkSMSessionObject.cxx
  vtkSMSessionProxyManager.cxx
  vtkSMSettings.cxx
  vtkSMSettingsProxy.cxx
  vtkSMSILDomain.cxx
  vtkSMSILModel.cxx
  vtkSMSourceProxy.cxx
  vtkSMStateLoader.cxx
  vtkSMStateLocator.cxx
  vtkSMStateVersionController.cxx
  vtkSMStringListDomain.cxx
  vtkSMStringVectorProperty.cxx
  vtkSMUncheckedPropertyHelper.cxx
  vtkSMUndoElement.cxx
  vtkSMUndoStackBuilder.cxx
  vtkSMUndoStack.cxx
  vtkSMNullProxy.cxx
  vtkSMTimeKeeper.cxx
  vtkSMTimeKeeperProxy.cxx
  vtkSMTrace.cxx
  vtkSMVectorProperty.cxx
  vtkSMWriterFactory.cxx
  vtkSMWriterProxy.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkSMDeserializer
  vtkSMDomain
  vtkSMLink
  vtkSMPropertyHelper
  vtkSMProxyManager  # since New() is protected.
  vtkSMRangeDomainTemplate
  vtkSMRemoteObject
  vtkSMSession
  vtkSMSessionProxyManager # since New() is non-standard.
  vtkSMUncheckedPropertyHelper
  vtkSMUndoElement
  vtkSMVectorProperty
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkSMPropertyHelper
  vtkSMRangeDomainTemplate
  vtkSMUncheckedPropertyHelper
  WRAP_EXCLUDE)

vtk_module_library(vtkPVServerManagerCore ${Module_SRCS})
