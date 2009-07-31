/*=========================================================================

  Program:   ParaView
  Module:    ServersServerManagerPrintSelf.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVBatchOptions.h"

#include "vtkSMAnimationCueProxy.h"
#include "vtkSMAnimationSceneGeometryWriter.h"
#include "vtkSMAnimationSceneImageWriter.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMApplication.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMArrayListInformationHelper.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMArraySelectionInformationHelper.h"
#include "vtkSMBooleanKeyFrameProxy.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMBoxProxy.h"
#include "vtkSMCameraKeyFrameProxy.h"
#include "vtkSMCameraLink.h"
#include "vtkSMCameraManipulatorProxy.h"
#include "vtkSMCameraProxy.h"
#include "vtkSMClientDeliveryStrategyProxy.h"
#include "vtkSMCompositeKeyFrameProxy.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMCompoundProxyDefinitionLoader.h"
#include "vtkSMConnectionCleanerProxy.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleArrayInformationHelper.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMExponentialKeyFrameProxy.h"
#include "vtkSMExtentDomain.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMIntArrayInformationHelper.h"
#include "vtkSMFileListDomain.h"
#include "vtkSMFixedTypeDomain.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMImplicitPlaneProxy.h"
#include "vtkSMImplicitPlaneRepresentationProxy.h"
#include "vtkSMInformationHelper.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMInteractionUndoStackBuilder.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMKeyFrameAnimationCueManipulatorProxy.h"
#include "vtkSMKeyFrameProxy.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMMaterialLoaderProxy.h"
#include "vtkSMNullProxy.h"
#include "vtkSMNumberOfGroupsDomain.h"
#include "vtkSMObject.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPQStateLoader.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionIterator.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyRegisterUndoElement.h"
#include "vtkSMProxyUnRegisterUndoElement.h"
#include "vtkSMPVLookupTableProxy.h"
#include "vtkSMPWriterProxy.h"
#include "vtkSMRampKeyFrameProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMServerFileListingProxy.h"
#include "vtkSMServerProxyManagerReviver.h"
#include "vtkSMServerSideAnimationPlayer.h"
#include "vtkSMSimpleDoubleInformationHelper.h"
#include "vtkSMSimpleIntInformationHelper.h"
#include "vtkSMSimpleParallelStrategy.h"
#include "vtkSMSimpleStrategy.h"
#include "vtkSMSimpleStringInformationHelper.h"
#include "vtkSMSinusoidKeyFrameProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringListRangeDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMSubPropertyIterator.h"
#include "vtkSMTesting.h"
#include "vtkSMTimeRangeInformationHelper.h"
#include "vtkSMTimeStepsInformationHelper.h"
#include "vtkSMTransformProxy.h"
#include "vtkSMUndoRedoStateLoader.h"
#include "vtkSMUndoStack.h"
#include "vtkSMUndoStackBuilder.h"
#include "vtkSMUpdateInformationUndoElement.h"
#include "vtkSMUpdateSuppressorProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMWidgetRepresentationProxy.h"
#include "vtkSMWriterProxy.h"
#include "vtkSMXDMFInformationHelper.h"
#include "vtkSMXDMFPropertyDomain.h"
#include "vtkSMXMLParser.h"
#include "vtkSMXMLPVAnimationWriterProxy.h"

// Display Proxies
#include "vtkSMScalarBarActorProxy.h"

// View Proxies
#include "vtkSMRenderViewProxy.h"
#include "vtkSMViewProxy.h"

// Representation Proxies
#include "vtkSMAxesRepresentationProxy.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMOutlineRepresentationProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSurfaceRepresentationProxy.h"
#include "vtkSMUniformGridVolumeRepresentationProxy.h"
#include "vtkSMUnstructuredGridVolumeRepresentationProxy.h"

#ifdef PARAVIEW_USE_ICE_T
#include "vtkSMProcessModule.h"
#include "vtkSMPropertyInternals.h"
#include "vtkSMProxyInternals.h"
#include "vtkSMProxyManagerInternals.h"
#include "vtkSMIceTCompositeViewProxy.h"
#include "vtkSMIceTDesktopRenderViewProxy.h"

#endif  //PARAVIEW_USE_ICE_T

int main(int , char *[])
{
  vtkObject *c;
  c = vtkPVBatchOptions::New(); c->Print( cout ); c->Delete();
  c = vtkSMAnimationCueProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMAnimationSceneGeometryWriter::New(); c->Print( cout ); c->Delete();
  c = vtkSMAnimationSceneImageWriter::New(); c->Print( cout ); c->Delete();
  c = vtkSMAnimationSceneProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMApplication::New(); c->Print( cout ); c->Delete();
  c = vtkSMArrayListDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMArrayListInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMArrayRangeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMArraySelectionDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMArraySelectionInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMBooleanDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMBooleanKeyFrameProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMBoundsDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMBoxProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMCameraKeyFrameProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMCameraLink::New(); c->Print( cout ); c->Delete();
  c = vtkSMCameraManipulatorProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMCameraProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMCompoundSourceProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMCompoundProxyDefinitionLoader::New(); c->Print( cout ); c->Delete();
  c = vtkSMCompositeKeyFrameProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMClientDeliveryStrategyProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMConnectionCleanerProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMDataTypeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMDocumentation::New(); c->Print( cout ); c->Delete();
  c = vtkSMDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMDomainIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMDoubleRangeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMDoubleVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMEnumerationDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMExponentialKeyFrameProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMExtentDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMFieldDataDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMFileListDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMFixedTypeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMIdTypeVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMImplicitPlaneProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMImplicitPlaneRepresentationProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMInputArrayDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMInputProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMInteractionUndoStackBuilder::New(); c->Print( cout ); c->Delete();
  c = vtkSMIntArrayInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMIntRangeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMIntVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMKeyFrameAnimationCueManipulatorProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMKeyFrameProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMLookupTableProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMMaterialLoaderProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMNullProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMNumberOfGroupsDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMObject::New(); c->Print( cout ); c->Delete();
  c = vtkSMOrderedPropertyIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMOutputPort::New(); c->Print( cout ); c->Delete();
  c = vtkSMPQStateLoader::New(); c->Print( cout ); c->Delete();
  c = vtkSMPropertyAdaptor::New(); c->Print( cout ); c->Delete();
  c = vtkSMProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMPropertyIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMPropertyLink::New(); c->Print( cout ); c->Delete();
  c = vtkSMPropertyModificationUndoElement::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyGroupDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyDefinitionIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyLink::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyListDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyManager::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyRegisterUndoElement::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyUnRegisterUndoElement::New(); c->Print( cout ); c->Delete();
  c = vtkSMPVLookupTableProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMPWriterProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMRampKeyFrameProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMSelectionHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMServerFileListingProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMServerProxyManagerReviver::New(); c->Print( cout ); c->Delete();
  c = vtkSMServerSideAnimationPlayer::New(); c->Print( cout ); c->Delete();
  c = vtkSMDoubleArrayInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMSimpleDoubleInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMSimpleIntInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMSimpleParallelStrategy::New(); c->Print( cout ); c->Delete();
  c = vtkSMSimpleStrategy::New(); c->Print( cout ); c->Delete();
  c = vtkSMSimpleStringInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMSinusoidKeyFrameProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMSourceProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMStringListDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMStringListRangeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMStringVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMSubPropertyIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMTesting::New(); c->Print( cout ); c->Delete();
  c = vtkSMTimeRangeInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMTimeStepsInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMTransformProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMUndoRedoStateLoader::New(); c->Print( cout ); c->Delete();
  c = vtkSMUndoStack::New(); c->Print( cout ); c->Delete();
  c = vtkSMUndoStackBuilder::New(); c->Print( cout ); c->Delete();
  c = vtkSMUpdateInformationUndoElement::New(); c->Print( cout ); c->Delete();
  c = vtkSMUpdateSuppressorProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMWidgetRepresentationProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMWriterProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMXDMFInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMXDMFPropertyDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMXMLParser::New(); c->Print( cout ); c->Delete();
  c = vtkSMXMLPVAnimationWriterProxy::New(); c->Print( cout ); c->Delete();

  c = vtkSMScalarBarActorProxy::New(); c->Print( cout ); c->Delete();

  // View Proxies
  c = vtkSMRenderViewProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMViewProxy::New(); c->Print( cout ); c->Delete();

  // Representation Proxies
  c = vtkSMAxesRepresentationProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMClientDeliveryRepresentationProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMNewWidgetRepresentationProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMOutlineRepresentationProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMScalarBarWidgetRepresentationProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMSurfaceRepresentationProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMUniformGridVolumeRepresentationProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMUnstructuredGridVolumeRepresentationProxy::New(); c->Print( cout ); c->Delete();

#ifdef PARAVIEW_USE_ICE_T
  c = vtkSMProcessModule::New(); c->Print( cout ); c->Delete();
  c = vtkSMPropertyInternals::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyInternals::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyManagerInternals::New(); c->Print( cout ); c->Delete();
  c = vtkSMIceTCompositeViewProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMIceTDesktopRenderViewProxy::New(); c->Print( cout ); c->Delete();
#endif  //PARAVIEW_USE_ICE_T

  return 0;
}
